list(APPEND UTILS_SRCS  
	datamap_wrapper.cpp 
	game_detection.cpp
	interfaces.cpp
	lua.cpp 
	math.cpp
	signals.cpp
)

add_library(utilslib STATIC ${UTILS_SRCS})
target_link_libraries(utilslib PUBLIC ssdk lua Signal SPTLib)
target_include_directories(utilslib PUBLIC include)
