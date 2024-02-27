#include "plugin.h"
#include <cstring>
#include <chrono>
#include <Windows.h>
#include <libloaderapi.h>
#include "SPTLib/MemUtils.hpp"
#include "SPTLib/Hooks.hpp"
#include "feature.hpp"
#include "icvar.h"
#include "cdll_int.h"
#include "eiface.h"
#include "filesystem.h"
#include "icliententitylist.h"
#include "engine/iserverplugin.h"
#include "interfaces.hpp"
#include "game_detection.hpp"
#include "signals.hpp"

static Plugin plugin;
static ConVar* sv_cheats;

static void Unload_Cvars();
static void Load_Cvars();

extern "C" __declspec(dllexport) const void* CreateInterface(const char* pName, int* pReturnCode) {
	if (pReturnCode) {
		*pReturnCode = 1; // not sure if 0 is success or failure but it appears this is not checked anyway
	}

	return &plugin;
}

static void SetFuncIfFound(void** pTarget, void* func) {
	if (func) {
		*pTarget = func;
	}
}

static void GrabTier0Funcs() {
	HMODULE moduleHandle = GetModuleHandleA("tier0.dll");
	if (moduleHandle != NULL) {
		SetFuncIfFound((void**)&ConColorMsg, GetProcAddress(moduleHandle, "ConColorMsg"));
		SetFuncIfFound((void**)&ConMsg, GetProcAddress(moduleHandle, "?ConMsg@@YAXPBDZZ"));
		SetFuncIfFound((void**)&DevMsg, GetProcAddress(moduleHandle, "?DevMsg@@YAXPBDZZ"));
		SetFuncIfFound((void**)&DevWarning, GetProcAddress(moduleHandle, "?DevWarning@@YAXPBDZZ"));
		SetFuncIfFound((void**)&Msg, GetProcAddress(moduleHandle, "Msg"));
		SetFuncIfFound((void**)&Warning, GetProcAddress(moduleHandle, "Warning"));
	}
}

static bool pluginLoaded = false;
static bool skipUnload = false;
static patterns::PatternCache cache;

static void* Sys_GetProcAddress(const char* pModuleName, const char* pName) {
	HMODULE hModule = GetModuleHandle(pModuleName);
	return GetProcAddress(hModule, pName);
}

bool Plugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
	auto start = std::chrono::steady_clock::now();
	GrabTier0Funcs();

	if (pluginLoaded) {
		Warning("Trying to load snufkin when snufkin is already loaded.\n");
		// Failure to load causes immediate call to Unload, make sure we do nothing there
		skipUnload = true;
		return false;
	}

	pluginLoaded = true;

	g_pCVar = (ICvar*)interfaceFactory(CVAR_INTERFACE_VERSION, NULL);
	interfaces::g_pFileSystem = (IFileSystem*)interfaceFactory(FILESYSTEM_INTERFACE_VERSION, NULL);
	interfaces::g_pEngine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	interfaces::g_pEngineClient = (IVEngineClient*)interfaceFactory(VENGINE_CLIENT_INTERFACE_VERSION, NULL);
	interfaces::pluginHelpers =
	    (IServerPluginHelpers*)interfaceFactory(INTERFACEVERSION_ISERVERPLUGINHELPERS, NULL);

	auto clientFactory = (CreateInterfaceFn)Sys_GetProcAddress("client", "CreateInterface");
	interfaces::entList = (IClientEntityList*)clientFactory(VCLIENTENTITYLIST_INTERFACE_VERSION, NULL);

	std::string cachePath = interfaces::g_pEngineClient->GetGameDirectory();
	cachePath += "/pattern_cache.json";

	cache.LoadFromFile(cachePath);
	utils::SearchBuildNumber(&cache);
	Feature::LoadFeatures(&cache);
	cache.SaveIfChanged(cachePath);
	Load_Cvars();
	sv_cheats = g_pCVar->FindVar("sv_cheats");

	auto end = std::chrono::steady_clock::now();
	Msg("snufkin loaded in %f milliseconds\n", (end - start).count() / 1e6);

	return true;
}

void Plugin::Unload(void) {
	if (!skipUnload) {
		Unload_Cvars();
		Feature::UnloadFeatures();
		Hooks::Free();
	} else {
		skipUnload = false;
	}
}

const char* Plugin::GetPluginDescription(void) { return "snufkin"; }

PLUGIN_RESULT Plugin::ClientConnect(bool* bAllowConnect,
                                    edict_t* pEntity,
                                    const char* pszName,
                                    const char* pszAddress,
                                    char* reject,
                                    int maxrejectlen) {
	signals::ClientConnect(bAllowConnect, pEntity, pszName, pszAddress, reject, maxrejectlen);
	return PLUGIN_CONTINUE;
}

PLUGIN_RESULT Plugin::ClientCommand(edict_t* pEntity, const CCommand& args) {
	signals::ClientCommand(pEntity, args);
	return PLUGIN_CONTINUE;
}

PLUGIN_RESULT Plugin::NetworkIDValidated(const char* pszUserName, const char* pszNetworkID) {
	signals::NetworkIDValidated(pszUserName, pszNetworkID);
	return PLUGIN_CONTINUE;
}

void Plugin::LevelInit(char const* pMapName) { signals::LevelInit(pMapName); }
void Plugin::ServerActivate(edict_t* pEdictList, int edictCount, int clientMax) {
	signals::ServerActivate(pEdictList, edictCount, clientMax);
}
void Plugin::GameFrame(bool simulating) { signals::GameFrame(simulating); }
void Plugin::LevelShutdown(void) { signals::LevelShutdown(); }
void Plugin::ClientActive(edict_t* pEntity) { signals::ClientActive(pEntity); }
void Plugin::ClientDisconnect(edict_t* pEntity) { signals::ClientDisconnect(pEntity); }
void Plugin::ClientPutInServer(edict_t* pEntity, char const* playername) {
	signals::ClientPutInServer(pEntity, playername);
}
void Plugin::SetCommandClient(int index) { signals::SetCommandClient(index); }
void Plugin::ClientSettingsChanged(edict_t* pEdict) { signals::ClientSettingsChanged(pEdict); }
void Plugin::OnQueryCvarValueFinished(QueryCvarCookie_t iCookie,
                                      edict_t* pPlayerEntity,
                                      EQueryCvarValueStatus eStatus,
                                      const char* pCvarName,
                                      const char* pCvarValue) {
	signals::OnQueryCvarValueFinished(iCookie, pPlayerEntity, eStatus, pCvarName, pCvarValue);
}
void Plugin::OnEdictAllocated(edict_t* edict) { signals::OnEdictAllocated(edict); }
void Plugin::OnEdictFreed(const edict_t* edict) { signals::OnEdictFreed(edict); }

static std::vector<ConCommandBase*> m_vecInitializedCommands;

static void Load_Cvars() {
	for (auto* command : m_vecInitializedCommands) {
		g_pCVar->RegisterConCommand(command);
	}
}

static void Unload_Cvars() {
	for (auto* command : m_vecInitializedCommands) {
		g_pCVar->UnregisterConCommand(command);
	}
	m_vecInitializedCommands.clear();
}

void InitConcommandBase(ConCommandBase& base) { m_vecInitializedCommands.push_back(&base); }

bool CheckCheatVarBool(const ConVar& base) { return base.GetBool() && sv_cheats->GetBool(); }
bool CheatsEnabled() { return sv_cheats->GetBool(); }

int GetPluginIndex() {
	auto& plugins = *(CUtlVector<uintptr_t>*)((uint32_t)interfaces::pluginHelpers + 4);

	for (int i = 0; i < plugins.Count(); i++)
		if (*(IServerPluginCallbacks**)(plugins[i] + 132) == &plugin)
			return i;
	return -1;
}

void EngineConCmd(const char* command) {
	if (interfaces::g_pEngineClient)
		interfaces::g_pEngineClient->ClientCmd(command);
}
