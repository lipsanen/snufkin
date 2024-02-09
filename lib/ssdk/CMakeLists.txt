add_library(ssdk STATIC dbg.cpp cvar.cpp mathlib_base.cpp strtools.cpp utlbuffer.cpp utlstring.cpp vector.cpp vmatrix.cpp)
target_include_directories(ssdk PUBLIC include/ include/tier1 include/tier0)