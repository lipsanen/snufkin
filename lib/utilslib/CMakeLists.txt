list(APPEND UTILS_SRCS  
	datamap_wrapper.cpp 
	lua.cpp 
	math.cpp
)

add_library(utilslib STATIC ${UTILS_SRCS})
target_link_libraries(utilslib PUBLIC ssdk lua Signal SPTLib)
target_include_directories(utilslib PUBLIC include)
