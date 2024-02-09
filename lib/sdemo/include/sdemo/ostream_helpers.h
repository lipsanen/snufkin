#pragma once

#include "sdemo/sdemo.h"
#include <iostream>

namespace sdemo
{
    inline std::ostream& operator<<(std::ostream& os, const header_t& header) {
        #define PRINT(x) os << "\t" #x ": " << header.  x << '\n'

        PRINT(ID);
        PRINT(demo_protocol);
        PRINT(net_protocol);
        PRINT(server_name);
        PRINT(client_name);
        PRINT(map_name);
        PRINT(game_directory);
        PRINT(seconds);
        PRINT(tick_count);
        PRINT(frame_count);
        PRINT(signon_length);

        #undef PRINT

        return os;
    }
}