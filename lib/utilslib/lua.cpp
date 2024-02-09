#include "utils/lua.hpp"
#include "Color.h"
#include <cstdlib>
#include <cstring>

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

#define SSDK_COLOR "ssdk.color"

using namespace utils;

static void* alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
	(void)ud;
	(void)osize; // Not used
	if (nsize == 0) {
		free(ptr);
		return NULL;
	} else {
		void* p = realloc(ptr, nsize);
		return p;
	}
}

static int newcolor(lua_State* L) {
	if (lua_gettop(L) != 4) {
		return luaL_error(L, "color.new takes 4 arguments: red, green, blue and alpha");
	}

	Color* c = (Color*)lua_newuserdata(L, sizeof(Color));
	for (int i = 1; i <= 4; ++i) {
		c->_color[i - 1] = lua_tointeger(L, i);
	}

	luaL_getmetatable(L, SSDK_COLOR);
	lua_setmetatable(L, -2);

	return 1;
}

#define checkcolor(L) \
(Color*)luaL_checkudata(L, 1, SSDK_COLOR)

static int getcolor(lua_State* L) {
	Color* c = checkcolor(L);
	int index = lua_tointeger(L, 2);

	if (index > 4 || index < 1) {
		return luaL_error(L, "index should be between 1 and 4");
	}
	lua_pushnumber(L, c->_color[index-1]);
	return 1;
}

static int setcolor(lua_State* L) {
	Color* c = checkcolor(L);
	int index = lua_tointeger(L, 2);
	int value = lua_tointeger(L, 3);

	if (index > 4 || index < 1) {
		return luaL_error(L, "index should be between 1 and 4");
	}

	c->_color[index-1] = value;
	return 0;
}

static const struct luaL_Reg colorlib[] = {
    {"new", newcolor},
	{"set", setcolor},
    {NULL, NULL},
};


static const struct luaL_Reg colorlib_m[] = {
	{"__index", getcolor},
	{"__newindex", setcolor},
	{NULL, NULL},
};

static void luaopen_color(lua_State* L) {
	luaL_newmetatable(L, SSDK_COLOR);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, colorlib_m, 0);
	luaL_newlib(L, colorlib);
	lua_setglobal(L, "color");
}

void LuaState::init() {
	this->L = lua_newstate(&alloc, NULL); /* opens Lua */
	luaopen_base(L);
	luaopen_table(L);
	luaopen_io(L);
	luaopen_string(L);
	luaopen_math(L);
	luaopen_color(L);
}

utils::LuaState::~LuaState() {
	if (L) {
		lua_close(L);
	}
}

int utils::LuaState::run_inline_script(const char* script) {
	int error = luaL_loadbuffer(L, script, strlen(script), "line") || lua_pcall(L, 0, 0, 0);
	if (error) {
		lua_fprintf(stderr, "%s\n", lua_tostring(L, -1));
		lua_pop(L, 1); /* pop error message from the stack */
		return 1;
	}

	return 0;
}

int utils::LuaState::run_script(const char* filepath) {
	int error = luaL_dofile(L, filepath);
	if (error) {
		lua_fprintf(stderr, "%s\n", lua_tostring(L, -1));
		lua_pop(L, 1); /* pop error message from the stack */
		return 1;
	}

	return 0;
}

std::optional<int> utils::LuaState::get_int(const char* name) {
	int ltype = lua_getglobal(L, name);
	if (ltype != LUA_TNUMBER) {
		lua_pop(L, 1);
		return {};
	}

	int result = lua_tointeger(L, -1);
	lua_pop(L, 1);
	return result;
}
