cmake_minimum_required(VERSION 3.15)
project(b VERSION 1.0.0)

set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(DEBUG_BUILD "$<CONFIG:Debug>")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

if(!WIN32)
	add_compile_options(-fdata-sections -ffunction-sections)
	add_link_options(-Wl,--gc-sections)
	add_compile_options(-m32)
	add_link_options(-m32)
endif()

set(BUILD_SHARED_LIBS False)
add_subdirectory(lib)
add_subdirectory(tools)

if (WIN32)
	add_subdirectory(src)
endif()

enable_testing()
add_subdirectory(test/sdemo_tests)
add_subdirectory(test/sptlib_tests)
add_subdirectory(test/utilslib_tests)
