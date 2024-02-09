#include "ent_props.hpp"
#include "convar.h"
#include "../plugin.h"
#include "SPTLib/feature.hpp"
#include "SPTLib/patterns.hpp"
#include "utils/game_detection.hpp"
#include "utils/interfaces.hpp"
#include "client_class.h"
#include "eiface.h"
#include "icliententitylist.h"
#include "icliententity.h"
#include "iserverunknown.h"
#include "dt_recv.h"
#include <string>
#include <unordered_map>
#include "cdll_int.h"

EntProps feat_entprops;

namespace patterns
{
	PATTERNS(Datamap,
	         "pattern1",
	         "C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C7 05 ?? ?? ?? ?? ?? ?? ?? ?? B8",
	         "pattern2",
	         "C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C3",
	         "pattern3",
	         "C7 05 ?? ?? ?? ?? ?? ?? ?? ?? B8 ?? ?? ?? ?? C7 05");
}

void EntProps::InitHooks()
{
	AddMatchAllPattern(patterns::Datamap, "client", "Datamap", &clientPatterns);
	AddMatchAllPattern(patterns::Datamap, "server", "Datamap", &serverPatterns);
	tablesProcessed = false;
}

void EntProps::PreHook()
{
	ProcessTablesLazy();
}

void EntProps::UnloadFeature()
{
	for (auto* map : wrappers)
		delete map;
}

void EntProps::WalkDatamap(std::string key)
{
	ProcessTablesLazy();
	auto* result = GetDatamapWrapper(key);
	if (result)
		result->ExploreOffsets();
	else
		Msg("No datamap found with name \"%s\"\n", key.c_str());
}

void EntProps::PrintDatamaps()
{
	Msg("Printing all datamaps:\n");
	std::vector<std::string> names;
	names.reserve(wrappers.size());

	for (auto& pair : nameToMapWrapper)
		names.push_back(pair.first);

	std::sort(names.begin(), names.end());

	for (auto& name : names)
	{
		auto* map = nameToMapWrapper[name];

		Msg("\tDatamap: %s, server map %x, client map %x\n", name.c_str(), map->_serverMap, map->_clientMap);
	}
}

int EntProps::GetPlayerOffset(const std::string& key, bool server)
{
	ProcessTablesLazy();
	auto* playermap = GetPlayerDatamapWrapper();

	if (!playermap)
		return utils::INVALID_DATAMAP_OFFSET;

	if (server)
		return playermap->GetServerOffset(key);
	else
		return playermap->GetClientOffset(key);
}

void* EntProps::GetPlayer(bool server)
{
	if (server)
	{
		if (!interfaces::g_pEngine)
			return nullptr;

		auto edict = interfaces::g_pEngine->PEntityOfEntIndex(1);
		if (!edict || (uint32_t)edict == 0xFFFFFFFF)
			return nullptr;

		return edict->GetUnknown();
	}
	else
	{
		return interfaces::entList->GetClientEntity(1);
	}
}

int EntProps::GetFieldOffset(const std::string& mapKey, const std::string& key, bool server)
{
	auto map = GetDatamapWrapper(mapKey);
	if (!map)
	{
		return utils::INVALID_DATAMAP_OFFSET;
	}

	if (server)
		return map->GetServerOffset(key);
	else
		return map->GetClientOffset(key);
}

_InternalPlayerField EntProps::_GetPlayerField(const std::string& key, PropMode mode)
{
	_InternalPlayerField out;
	// Do nothing if we didn't load
	if (!startedLoading)
		return out;

	bool getServer = mode == PropMode::Server || mode == PropMode::PreferClient || mode == PropMode::PreferServer;
	bool getClient = mode == PropMode::Client || mode == PropMode::PreferClient || mode == PropMode::PreferServer;

	if (getServer)
	{
		out.serverOffset = GetPlayerOffset(key, true);
	}

	if (getClient)
	{
		out.clientOffset = GetPlayerOffset(key, false);
	}

	if (out.serverOffset == utils::INVALID_DATAMAP_OFFSET && out.clientOffset == utils::INVALID_DATAMAP_OFFSET)
	{
		DevWarning("Was unable to find field %s offsets for server or client!\n", key.c_str());
	}

	return out;
}

static bool IsAddressLegal(uint8_t* address, uint8_t* start, std::size_t length)
{
	return address >= start && address <= start + length;
}

static bool DoesMapLookValid(datamap_t* map, uint8_t* moduleStart, std::size_t length)
{
	if (!IsAddressLegal(reinterpret_cast<uint8_t*>(map->dataDesc), moduleStart, length))
		return false;

	int MAX_LEN = 64;
	char* name = const_cast<char*>(map->dataClassName);

	if (!IsAddressLegal(reinterpret_cast<uint8_t*>(name), moduleStart, length))
		return false;

	for (int i = 0; i < MAX_LEN; ++i)
	{
		if (name[i] == '\0')
		{
			return i > 0;
		}
	}

	return false;
}

PropMode EntProps::ResolveMode(PropMode mode)
{
	auto svplayer = GetPlayer(true);
	auto clplayer = GetPlayer(false);

	switch (mode)
	{
	case PropMode::Client:
		if (clplayer)
			return PropMode::Client;
		else
			return PropMode::None;
	case PropMode::Server:
		if (svplayer)
			return PropMode::Server;
		else
			return PropMode::None;
	case PropMode::PreferServer:
		if (svplayer)
			return PropMode::Server;
		else if (clplayer)
			return PropMode::Client;
		else
			return PropMode::None;
	case PropMode::PreferClient:
		if (clplayer)
			return PropMode::Client;
		else if (svplayer)
			return PropMode::Server;
		else
			return PropMode::None;
	default:
		return PropMode::None;
	}
}

void EntProps::AddMap(datamap_t* map, bool server)
{
	std::string name = map->dataClassName;

	// if this is a client map, get the associated server name
	if (!server)
	{
		if (!strcmp(map->dataClassName, "C_BaseHLPlayer"))
			name = "CHL2_Player";
		else if (strstr(map->dataClassName, "C_") == map->dataClassName)
			name = name.erase(1, 1);
	}

	auto result = nameToMapWrapper.find(name);
	utils::DatamapWrapper* ptr;

	if (result == nameToMapWrapper.end())
	{
		ptr = new utils::DatamapWrapper();
		wrappers.push_back(ptr);
		nameToMapWrapper[name] = ptr;
	}
	else
	{
		ptr = result->second;
	}

	if (server)
		ptr->_serverMap = map;
	else
		ptr->_clientMap = map;
}

utils::DatamapWrapper* EntProps::GetDatamapWrapper(const std::string& key)
{
	auto result = nameToMapWrapper.find(key);
	if (result != nameToMapWrapper.end())
		return result->second;
	else
		return nullptr;
}

utils::DatamapWrapper* EntProps::GetPlayerDatamapWrapper()
{
	// Grab cached result if exists, can also be NULL!
	if (playerDatamapSearched)
		return __playerdatamap;

	if (utils::DoesGameLookLikePortal())
	{
		__playerdatamap = GetDatamapWrapper("CPortal_Player");
	}

	// Add any other game specific class names here
	if (!__playerdatamap)
	{
		__playerdatamap = GetDatamapWrapper("CHL2_Player");
	}

	if (!__playerdatamap)
	{
		__playerdatamap = GetDatamapWrapper("CBasePlayer");
	}

	if (!__playerdatamap)
	{
		__playerdatamap = GetDatamapWrapper("CBaseCombatCharacter");
	}

	if (!__playerdatamap)
	{
		__playerdatamap = GetDatamapWrapper("CBaseEntity");
	}

	playerDatamapSearched = true;
	return __playerdatamap;
}

static void GetDatamapInfo(patterns::MatchedPattern pattern, int& numfields, datamap_t**& pmap)
{
	switch (pattern.ptnIndex)
	{
	case 0:
		numfields = *reinterpret_cast<int*>(pattern.ptr + 6);
		pmap = reinterpret_cast<datamap_t**>(pattern.ptr + 12);
		break;
	case 1:
		numfields = *reinterpret_cast<int*>(pattern.ptr + 6);
		pmap = reinterpret_cast<datamap_t**>(pattern.ptr + 12);
		break;
	case 2:
		numfields = *reinterpret_cast<int*>(pattern.ptr + 6);
		pmap = reinterpret_cast<datamap_t**>(pattern.ptr + 17);
		break;
	}
}

void EntProps::ProcessTablesLazy()
{
	if (tablesProcessed)
		return;

	void* clhandle;
	void* clmoduleStart;
	size_t clmoduleSize;
	void* svhandle;
	void* svmoduleStart;
	size_t svmoduleSize;

	if (MemUtils::GetModuleInfo(L"server.dll", &svhandle, &svmoduleStart, &svmoduleSize))
	{
		for (auto serverPattern : serverPatterns)
		{
			int numfields;
			datamap_t** pmap;
			GetDatamapInfo(serverPattern, numfields, pmap);
			if (numfields > 0
			    && IsAddressLegal(reinterpret_cast<uint8_t*>(pmap),
			                      reinterpret_cast<uint8_t*>(svmoduleStart),
			                      svmoduleSize))
			{
				datamap_t* map = *pmap;
				if (DoesMapLookValid(map, reinterpret_cast<uint8_t*>(svmoduleStart), svmoduleSize))
					AddMap(map, true);
			}
		}
	}

	if (MemUtils::GetModuleInfo(L"client.dll", &clhandle, &clmoduleStart, &clmoduleSize))
	{
		for (auto& clientPattern : clientPatterns)
		{
			int numfields;
			datamap_t** pmap;
			GetDatamapInfo(clientPattern, numfields, pmap);
			if (numfields > 0
			    && IsAddressLegal(reinterpret_cast<uint8_t*>(pmap),
			                      reinterpret_cast<uint8_t*>(clmoduleStart),
			                      clmoduleSize))
			{
				datamap_t* map = *pmap;
				if (DoesMapLookValid(map, reinterpret_cast<uint8_t*>(clmoduleStart), clmoduleSize))
					AddMap(map, false);
			}
		}
	}

	clientPatterns.clear();
	serverPatterns.clear();
	tablesProcessed = true;
}

CON_COMMAND(sn_datamap_print, "Prints all datamaps.")
{
	feat_entprops.PrintDatamaps();
}

CON_COMMAND(sn_datamap_walk, "Walk through a datamap and print all offsets.")
{
	feat_entprops.WalkDatamap(args.Arg(1));
}

void EntProps::LoadFeature()
{
	m_vecAbsOrigin = GetPlayerField<Vector>("m_vecAbsOrigin");
	m_vecViewOffset = GetPlayerField<Vector>("m_vecViewOffset");
	InitCommand(sn_datamap_print);
	InitCommand(sn_datamap_walk);
}

void** _InternalPlayerField::GetServerPtr() const
{
	auto serverplayer = reinterpret_cast<uintptr_t>(feat_entprops.GetPlayer(true));
	if (serverplayer && serverOffset != utils::INVALID_DATAMAP_OFFSET)
		return reinterpret_cast<void**>(serverplayer + serverOffset);
	else
		return nullptr;
}

void** _InternalPlayerField::GetClientPtr() const
{
	auto clientPlayer = reinterpret_cast<uintptr_t>(feat_entprops.GetPlayer(false));
	if (clientPlayer && clientOffset != utils::INVALID_DATAMAP_OFFSET)
		return reinterpret_cast<void**>(clientPlayer + clientOffset);
	else
		return nullptr;
}

bool _InternalPlayerField::ClientOffsetFound() const
{
	return clientOffset != utils::INVALID_DATAMAP_OFFSET;
}

bool _InternalPlayerField::ServerOffsetFound() const
{
	return serverOffset != utils::INVALID_DATAMAP_OFFSET;
}

IClientEntity* GetClientEntity(int index)
{
	return interfaces::entList->GetClientEntity(index + 1);
}

void PrintAllClientEntities()
{
	int maxIndex = interfaces::entList->GetHighestEntityIndex();

	for (int i = 0; i <= maxIndex; ++i)
	{
		auto ent = GetClientEntity(i);
		if (ent)
		{
			Msg("%d : %s\n", i, ent->GetClientClass()->m_pNetworkName);
		}
	}
}

IClientEntity* GetPlayer()
{
	return GetClientEntity(0);
}

Vector EntProps::GetPlayerEyePosition()
{
	return feat_entprops.m_vecAbsOrigin.GetValue() + feat_entprops.m_vecViewOffset.GetValue();
}

QAngle EntProps::GetPlayerEyeAngles()
{
	QAngle ang(0.0f, 0.0f, 0.0f);
	interfaces::g_pEngineClient->GetViewAngles(ang);
	return ang;
}

IClientEntity* EntProps::GetClientEntity(int index) {
	return interfaces::entList->GetClientEntity(index+1);
}

void GetPropValue(RecvProp* prop, void* ptr, char* buffer, size_t size)
{
	void* value = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) + prop->GetOffset());
	Vector v;
	int ehandle;

	switch (prop->m_RecvType)
	{
	case DPT_Int:
		if (strstr(prop->GetName(), "m_h") != NULL)
		{
			ehandle = *reinterpret_cast<int*>(value);
			sprintf_s(buffer,
				size,
				"(index %d, serial %d)",
				ehandle & ENT_ENTRY_MASK,
				(ehandle & (~ENT_ENTRY_MASK)) >> MAX_EDICT_BITS);
		}
		else
		{
			sprintf_s(buffer, size, "%d", *reinterpret_cast<int*>(value));
		}
		break;
	case DPT_Float:
		sprintf_s(buffer, size, "%f", *reinterpret_cast<float*>(value));
		break;
	case DPT_Vector:
		v = *reinterpret_cast<Vector*>(value);
		sprintf_s(buffer, size, "(%.3f, %.3f, %.3f)", v.x, v.y, v.z);
		break;
#ifdef SSDK2007
	case DPT_String:
		sprintf_s(buffer, size, "%s", *reinterpret_cast<const char**>(value));
		break;
#endif
	default:
		sprintf_s(buffer, size, "unable to parse");
		break;
	}
}

const size_t BUFFER_SIZE = 256;
static char BUFFER[BUFFER_SIZE];

void AddProp(std::vector<propValue>& props, const char* name, RecvProp* prop)
{
	props.push_back(propValue(name, BUFFER, prop));
}

void GetAllProps(RecvTable* table, void* ptr, std::vector<propValue>& props)
{
	int numProps = table->m_nProps;

	for (int i = 0; i < numProps; ++i)
	{
		auto prop = table->GetProp(i);

		if (strcmp(prop->GetName(), "baseclass") == 0)
		{
			RecvTable* base = prop->GetDataTable();

			if (base)
				GetAllProps(base, ptr, props);
		}
		else if (
			prop->GetOffset()
			!= 0) // There's various garbage attributes at offset 0 for a thusfar unbeknownst reason
		{
			GetPropValue(prop, ptr, BUFFER, BUFFER_SIZE);
			AddProp(props, prop->GetName(), prop);
		}
	}
}

void GetAllProps(IClientEntity* ent, std::vector<propValue>& props)
{
	props.clear();

	if (ent)
	{
		snprintf(BUFFER,
			BUFFER_SIZE,
			"(%.3f, %.3f, %.3f)",
			ent->GetAbsOrigin().x,
			ent->GetAbsOrigin().y,
			ent->GetAbsOrigin().z);
		AddProp(props, "AbsOrigin", nullptr);
		snprintf(BUFFER,
			BUFFER_SIZE,
			"(%.3f, %.3f, %.3f)",
			ent->GetAbsAngles().x,
			ent->GetAbsAngles().y,
			ent->GetAbsAngles().z);
		AddProp(props, "AbsAngles", nullptr);
		GetAllProps(ent->GetClientClass()->m_pRecvTable, ent, props);
	}
}

void PrintAllProps(int index)
{
	IClientEntity* ent = GetClientEntity(index);

	if (ent)
	{
		std::vector<propValue> props;
		GetAllProps(ent, props);

		Msg("Class %s props:\n\n", ent->GetClientClass()->m_pNetworkName);

		for (auto& prop : props)
			Msg("Name: %s, offset %d, value %s\n",
				prop.name.c_str(),
				prop.GetOffset(),
				prop.value.c_str());
	}
	else
		Msg("Entity doesn't exist!");
}

PropMap EntProps::FindOffsets(IClientEntity* ent)
{
	PropMap out;
	if (!ent)
	{
		out.foundOffsets = false;
		return out;
	}

	auto clientClass = ent->GetClientClass();
	std::vector<propValue> props;
	GetAllProps(clientClass->m_pRecvTable, ent, props);

	for (auto prop1 : props)
	{
		out.props[prop1.name] = prop1.prop;
	}

	out.foundOffsets = true;
	return out;
}

int EntProps::GetOffset(int entindex, const std::string& key)
{
	auto prop = GetRecvProp(entindex, key);
	if (prop)
		return prop->GetOffset();
	else
		return INVALID_OFFSET;
}

RecvProp* EntProps::GetRecvProp(int entindex, const std::string& key)
{
	auto ent = GetClientEntity(entindex);
	std::string className = ent->GetClientClass()->m_pNetworkName;

	if (classToOffsetsMap.find(className) == classToOffsetsMap.end())
	{
		auto map = FindOffsets(ent);
		if (!map.foundOffsets)
			return nullptr;
		classToOffsetsMap[className] = map;
	}

	auto& classMap = classToOffsetsMap[className].props;

	if (classMap.find(key) != classMap.end())
		return classMap[key];
	else
		return nullptr;
}


int propValue::GetOffset()
{
	if (prop)
	{
		return prop->GetOffset();
	}
	else
	{
		return INVALID_OFFSET;
	}
}

IServerUnknown* GetServerPlayer()
{
	if (!interfaces::g_pEngine)
		return nullptr;

	auto edict = interfaces::g_pEngine->PEntityOfEntIndex(1);
	if (!edict)
		return nullptr;

	return edict->GetUnknown();
}

bool playerEntityAvailable()
{
	return GetClientEntity(0) != nullptr;
}

static CBaseEntity* GetServerEntity(int index)
{
	if (!interfaces::g_pEngine)
		return nullptr;

	auto edict = interfaces::g_pEngine->PEntityOfEntIndex(index);
	if (!edict)
		return nullptr;

	auto unknown = edict->GetUnknown();
	if (!unknown)
		return nullptr;

	return unknown->GetBaseEntity();
}
