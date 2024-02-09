#include "utils/signals.hpp"

namespace signals {
	Gallant::Signal1<const char*> LevelInit;
	Gallant::Signal3<edict_t*, int, int> ServerActivate;
	Gallant::Signal1<bool> GameFrame;
	Gallant::Signal0<void> LevelShutdown;
	Gallant::Signal1<edict_t*> ClientActive;
	Gallant::Signal1<edict_t*> ClientDisconnect;
	Gallant::Signal2<edict_t*, const char*> ClientPutInServer;
	Gallant::Signal1<int> SetCommandClient;
	Gallant::Signal1<edict_t*> ClientSettingsChanged;
	Gallant::Signal6<bool*, edict_t*, const char*, const char*, char*, int> ClientConnect;
	Gallant::Signal2<edict_t*, const CCommand&> ClientCommand;
	Gallant::Signal2<const char*, const char*> NetworkIDValidated;
	Gallant::Signal5<QueryCvarCookie_t, edict_t*, EQueryCvarValueStatus, const char*, const char*>
	    OnQueryCvarValueFinished;
	Gallant::Signal1<edict_t*> OnEdictAllocated;
	Gallant::Signal1<const edict_t*> OnEdictFreed;
	Gallant::Signal2<IDirect3D9*, IDirect3DDevice9*> DX9_EndScene;
} // namespace signals
