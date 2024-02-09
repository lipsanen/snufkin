#pragma once

#include "engine/iserverplugin.h"

#define SNUFKIN_VERSION __DATE__ " " __TIME__

class Plugin : public IServerPluginCallbacks {
public:
	virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) override;
	virtual void Unload(void) override;
	virtual void Pause(void) override {}
	virtual void UnPause(void) override {}
	virtual const char* GetPluginDescription(void) override;
	virtual void LevelInit(char const* pMapName) override;
	virtual void ServerActivate(edict_t* pEdictList, int edictCount, int clientMax) override;
	virtual void GameFrame(bool simulating) override;
	virtual void LevelShutdown(void) override;
	virtual void ClientActive(edict_t* pEntity) override;
	virtual void ClientDisconnect(edict_t* pEntity) override;
	virtual void ClientPutInServer(edict_t* pEntity, char const* playername) override;
	virtual void SetCommandClient(int index) override;
	virtual void ClientSettingsChanged(edict_t* pEdict) override;
	virtual PLUGIN_RESULT ClientConnect(bool* bAllowConnect,
	                                    edict_t* pEntity,
	                                    const char* pszName,
	                                    const char* pszAddress,
	                                    char* reject,
	                                    int maxrejectlen) override;
	virtual PLUGIN_RESULT ClientCommand(edict_t* pEntity, const CCommand& args) override;
	virtual PLUGIN_RESULT NetworkIDValidated(const char* pszUserName, const char* pszNetworkID) override;
	virtual void OnQueryCvarValueFinished(QueryCvarCookie_t iCookie,
	                                      edict_t* pPlayerEntity,
	                                      EQueryCvarValueStatus eStatus,
	                                      const char* pCvarName,
	                                      const char* pCvarValue) override;
	virtual void OnEdictAllocated(edict_t* edict) override;
	virtual void OnEdictFreed(const edict_t* edict) override;
};

int GetPluginIndex();
void InitConcommandBase(ConCommandBase& base);
bool CheckCheatVarBool(const ConVar& base); // Check if cheats enabled and cvar enabled
void EngineConCmd(const char* command);
bool CheatsEnabled();
