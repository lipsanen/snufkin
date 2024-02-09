add_executable(slistdemo demo/slistdemo.cpp)
target_link_libraries(slistdemo sdemo)

add_executable(sdemodump demo/sdemodump.cpp)
target_link_libraries(sdemodump sdemo)

add_executable(luatest luatest.cpp)
target_link_libraries(luatest PUBLIC utilslib)
