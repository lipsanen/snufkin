set(SRCS main_tests.cpp)
add_executable(sdemo_tests ${SRCS})
target_link_libraries(sdemo_tests PRIVATE gtest gtest_main sdemo)
