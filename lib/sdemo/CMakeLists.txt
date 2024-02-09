set(SRCS parser.cpp version_utils.cpp)
add_library(sdemo STATIC ${SRCS})
target_include_directories(sdemo PUBLIC include)