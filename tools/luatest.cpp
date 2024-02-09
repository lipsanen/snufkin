#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "utils/lua.hpp"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

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


int main(void) {
    char buff[256];
    int error;
    utils::LuaState ls;
	ls.init();

    while (fgets(buff, sizeof(buff), stdin) != NULL) {
        ls.run_inline_script(buff);
    }

    return 0;
}
