#pragma once

class IFileSystem;
class IVEngineClient;
class IVEngineServer;
class IServerPluginHelpers;
class IClientEntityList;

namespace interfaces
{
	extern IFileSystem* g_pFileSystem;
	extern IVEngineClient* g_pEngineClient;
	extern IVEngineServer* g_pEngine;
	extern IServerPluginHelpers* pluginHelpers;
	extern IClientEntityList* entList;
}
