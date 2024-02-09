#pragma once

#include <optional>

extern "C"
{
#include <lua.h>
}

namespace utils
{
	class LuaState
	{
	public:
		void init();
		~LuaState();
		int run_inline_script(const char* script);
		int run_script(const char* filepath);
		std::optional<int> get_int(const char* name);

		lua_State* L = nullptr;
	};
}
