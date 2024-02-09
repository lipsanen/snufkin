list(APPEND PLUGIN_SOURCES
	"plugin.cpp"
	"features/autojump.cpp"
	"features/bigcheattext.cpp"
	"features/dx9.cpp"
	"features/ent_props.cpp"
	"features/input.cpp"
	"features/lua.cpp"
	"features/overlay.cpp"
	"features/portal_utils.cpp"
	"features/portalcolors.cpp"
	"features/shadow.cpp"
	"features/speedrun_demorecord.cpp"
	"features/updater.cpp"
)

add_library(snufkin SHARED ${PLUGIN_SOURCES})
target_link_libraries(snufkin PUBLIC utilslib SPTLib dx9 libcurl nlohmann Dbghelp Ws2_32 Wldap32 Crypt32 Normaliz)
