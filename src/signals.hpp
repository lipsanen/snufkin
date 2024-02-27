#pragma once

#include "thirdparty/Signal.h"
#include "engine/iserverplugin.h"

class IDirect3D9;
class IDirect3DDevice9;

namespace signals {
	// plugin interface signals
	extern Gallant::Signal1<const char*> LevelInit;
	extern Gallant::Signal3<edict_t*, int, int> ServerActivate;
	extern Gallant::Signal1<bool> GameFrame;
	extern Gallant::Signal0<void> LevelShutdown;
	extern Gallant::Signal1<edict_t*> ClientActive;
	extern Gallant::Signal1<edict_t*> ClientDisconnect;
	extern Gallant::Signal2<edict_t*, const char*> ClientPutInServer;
	extern Gallant::Signal1<int> SetCommandClient;
	extern Gallant::Signal1<edict_t*> ClientSettingsChanged;
	extern Gallant::Signal6<bool*, edict_t*, const char*, const char*, char*, int> ClientConnect;
	extern Gallant::Signal2<edict_t*, const CCommand&> ClientCommand;
	extern Gallant::Signal2<const char*, const char*> NetworkIDValidated;
	extern Gallant::Signal5<QueryCvarCookie_t, edict_t*, EQueryCvarValueStatus, const char*, const char*>
	    OnQueryCvarValueFinished;
	extern Gallant::Signal1<edict_t*> OnEdictAllocated;
	extern Gallant::Signal1<const edict_t*> OnEdictFreed;
	extern Gallant::Signal2<IDirect3D9*, IDirect3DDevice9*> DX9_EndScene;
} // namespace signals
