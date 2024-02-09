#include "sdemo/sdemo.h"
#include "sdemo/ostream_helpers.h"
#include <cstdio>
#include <iostream>

int main(int argc, char** argv)
{
    if(argc <= 1) {
        printf("Usage: sdemodump <filepath>\n");
        return 0;
    }

    sdemo::demo_t demo;
    auto result = sdemo::demo_t::parse_from(argv[1], &demo);

    if(result != sdemo::error_code::success) {
        std::cout << "Parsing failed with error " << (int)result << std::endl;
        return 1;
    } else {
        std::cout << demo.m_header;
        std::cout << demo.m_vec_messages.size() << " messages" << std::endl;
    }

    return 0;
}
