set(SRCS lua_tests.cpp)
add_executable(utilslib_tests ${SRCS})
target_link_libraries(utilslib_tests PRIVATE gtest gtest_main utilslib)
