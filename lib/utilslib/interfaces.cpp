#include "utils/interfaces.hpp"

namespace interfaces
{
	IFileSystem* g_pFileSystem;
	IVEngineClient* g_pEngineClient;
	IVEngineServer* g_pEngine;
	IServerPluginHelpers* pluginHelpers;
	IClientEntityList* entList;
}
