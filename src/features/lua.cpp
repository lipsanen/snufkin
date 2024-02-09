#include "feature.hpp"
#include "../plugin.h"
#include "utils/interfaces.hpp"
#include "utils/lua.hpp"
#include "cdll_int.h"
#include <stdlib.h>

// Running lua scripts and various hooks for lua scripts
class LuaFeature : public FeatureWrapper<LuaFeature>
{
public:
	utils::LuaState state;
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;
};

static LuaFeature feat_lua;

CON_COMMAND(sn_lua, "Run inline lua script")
{
	if (!CheatsEnabled()) {
		Msg("Cheats required for sn_lua\n");
		return;
	}

	if (args.ArgC() == 1) {
		Msg("Usage: sn_lua <insert lua script here>\n");
		return;
	}
	
	feat_lua.state.run_inline_script(args.ArgS());
}

CON_COMMAND(sn_lua_script, "Run lua script from file")
{
	if (!CheatsEnabled()) {
		Msg("Cheats required for sn_lua_script\n");
		return;
	}

	if (args.ArgC() == 1) {
		Msg("Usage: sn_lua_script <lua script>\n");
		return;
	}

	static char BUFFER[320];
	snprintf(BUFFER, sizeof(BUFFER), "%s\\lua\\%s.lua", interfaces::g_pEngineClient->GetGameDirectory(), args.Arg(1));
	feat_lua.state.run_script(BUFFER);
}

extern "C"
{
	extern size_t user_fwrite(void const* _Buffer, size_t _ElementSize, size_t _ElementCount, FILE* _Stream) {
		if (_Stream == stdout)
		{
			Msg((const char*)_Buffer);
			return _ElementSize * _ElementCount;
		}
		else if (_Stream == stderr) 
		{
			Warning((const char*)_Buffer);
			return _ElementSize * _ElementCount;
		}
		else
		{
			return fwrite(_Buffer, _ElementSize, _ElementCount, _Stream);
		}
	}

	int user_fflush(FILE* _Stream) {
		if (_Stream != stdout && _Stream != stderr) {
			return fflush(_Stream);
		}
		else {
			return 0;
		}
	}

	const char* user_luapath() {
		static bool path_set = false;
		static char BUFFER[320];
		if (!path_set) {
			snprintf(BUFFER, sizeof(BUFFER), "%s\\lua\\?.lua", interfaces::g_pEngineClient->GetGameDirectory());
			path_set = true;
		}
		return BUFFER;
	}
}

bool LuaFeature::ShouldLoadFeature()
{
	return true;
}

void LuaFeature::InitHooks() {}

void LuaFeature::LoadFeature() {
	state.init();
	InitCommand(sn_lua);
	InitCommand(sn_lua_script);
}

void LuaFeature::UnloadFeature() {
}
