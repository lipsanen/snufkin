set(SRCS main_tests.cpp)
add_executable(SPTLib_tests ${SRCS})
target_link_libraries(SPTLib_tests PRIVATE gtest gtest_main SPTLib)
add_test(NAME SPTLib_tests COMMAND SPTLib_tests)
