list(APPEND SPTLIB_SOURCES
    "Hooks.cpp"
    "IHookableDirFilter.cpp"
    "IHookableModule.cpp"
    "IHookableNameFilter.cpp"
    "IHookableNameFilterOrdered.cpp"
    "MemUtils.cpp"
    "feature.cpp"
    "PatternCacher.cpp"
    "PatternSearcher.cpp"
    "sptlib.cpp"
)

IF (WIN32)
    list(APPEND SPTLIB_SOURCES
        "Windows/DetoursUtils.cpp"
        "Windows/Hooks_win.cpp"
        "Windows/MemUtils_win.cpp"
        )

    list(APPEND MINHOOK_SOURCES
        "Windows/minhook/src/buffer.c"
        "Windows/minhook/src/hook.c"
        "Windows/minhook/src/trampoline.c"
        "Windows/minhook/src/hde/hde32.c"
        "Windows/minhook/src/hde/hde64.c"
        )
        
    add_library(SPTLib STATIC ${SPTLIB_SOURCES})
    add_library(minhook STATIC ${MINHOOK_SOURCES})
    target_include_directories(minhook PUBLIC Windows/minhook/include)
    target_link_libraries(SPTLib PRIVATE minhook)
ELSE()
    add_library(SPTLib STATIC ${SPTLIB_SOURCES})
ENDIF()

target_link_libraries(SPTLib PUBLIC ssdk nlohmann)
target_include_directories(SPTLib PUBLIC include/SPTLib include)

