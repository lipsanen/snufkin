#include "gtest/gtest.h"
#include "utils/lua.hpp"

extern "C"
{
	extern size_t user_fwrite(void const* _Buffer, size_t _ElementSize, size_t _ElementCount, FILE* _Stream) {
		return fwrite(_Buffer, _ElementSize, _ElementCount, _Stream);
	}

	int user_fflush(FILE* _Stream) {
		return fflush(_Stream);
	}

	const char* user_luapath() {
		return "?.lua";
	}
}


TEST(lua, Works)
{
	utils::LuaState state;
	state.init();
	state.run_inline_script("a = 42");

	int result = lua_getglobal(state.L, "a");
	EXPECT_EQ(result, LUA_TNUMBER);
	result = lua_tonumber(state.L, -1);
	EXPECT_EQ(result, 42);
}

TEST(lua, BrokenCodeErrors)
{
	utils::LuaState state;
	state.init();
	int error = state.run_inline_script("a =");
	EXPECT_NE(error, 0);
}


TEST(lua, ColorsWork)
{
	utils::LuaState state;
	state.init();
	int error = state.run_inline_script(
		"a = color.new(1, 2, 3, 4)\n"
		"b = a[1]\n"
		"a[2] = 42\n"
		"c = a[2]\n"
	);
	EXPECT_EQ(state.get_int("b").value(), 1);
	EXPECT_EQ(state.get_int("c").value(), 42);
	EXPECT_EQ(error, 0);
}
