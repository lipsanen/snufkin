#include "portalcolors.hpp"
#include <algorithm>

// Portal colors
class PortalColorsFeature : public FeatureWrapper<PortalColorsFeature>
{
public:
	struct Color colors[3] = {
		Color(242, 202, 167, 255), Color(64, 160, 255, 255), Color(255, 160, 32, 255) };
protected:
	virtual bool ShouldLoadFeature() override { return true;  }

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override {}

	DECL_HOOK_CDECL(Color, UTIL_Portal_Color, int portal);
};

PortalColorsFeature feat_portalcolors;

static void GravBeamChanged(IConVar* var, const char* pOldValue, float flOldValue);
static void BlueChanged(IConVar* var, const char* pOldValue, float flOldValue);
static void OrangeChanged(IConVar* var, const char* pOldValue, float flOldValue);

ConVar sn_color_grav("sn_color_grav", "F2CAA7FF", FCVAR_ARCHIVE | FCVAR_DEMO, "Hex color for the grav beam", GravBeamChanged);
ConVar sn_color_blue("sn_color_blue", "40A0FFFF", FCVAR_ARCHIVE | FCVAR_DEMO, "Hex color for the blue portal", BlueChanged);
ConVar sn_color_orange("sn_color_orange", "FFA020FF", FCVAR_ARCHIVE | FCVAR_DEMO, "Hex color for the orange portal", OrangeChanged);

static int hexToInt(char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	}

	char lowercased = tolower(c);
	if (lowercased >= 'a' && lowercased <= 'f') {
		return lowercased - 'a' + 10;
	}
	else {
		return 0;
	}
}

static void ChangeColor(Color* p, const char* value) {
	constexpr size_t colorstring_size = 8;
	char value_zeroed[colorstring_size];
	auto len = strlen(value);
	memset(value_zeroed, 0, colorstring_size);
	if (len > colorstring_size) {
		Warning("Your color was more than 8 characters long! Extra characters will be ignored\n");
	}
	else if (len < colorstring_size) {
		Warning("Your color was less than 8 characters long! Extra characters will be set to 0\n");
	}

	len = std::min(colorstring_size, len);
	memcpy(value_zeroed, value, len);

	for (size_t i = 0; i < 4; ++i) {
		std::uint8_t hex1 = hexToInt(value_zeroed[i * 2]);
		std::uint8_t hex2 = hexToInt(value_zeroed[i * 2+1]);
		p->_color[i] = (hex1 << 4) | hex2;
	}
}

static void GravBeamChanged(IConVar* var, const char* pOldValue, float flOldValue) {
	ChangeColor(feat_portalcolors.colors, sn_color_grav.GetString());
}

static void BlueChanged(IConVar* var, const char* pOldValue, float flOldValue) {
	ChangeColor(feat_portalcolors.colors + 1, sn_color_blue.GetString());
}

static void OrangeChanged(IConVar* var, const char* pOldValue, float flOldValue) {
	ChangeColor(feat_portalcolors.colors + 2, sn_color_orange.GetString());
}

namespace patterns
{
	PATTERNS(UTIL_Portal_Color,
		"3420",
		"8B 44 24 08 83 E8 00 74 37 83 E8 01 B1 FF 74 1E 83 E8 01 8B 44 24 04 88"
	);
}

IMPL_HOOK_CDECL(PortalColorsFeature, Color, UTIL_Portal_Color, int portal)
{
	if (portal < 0 || portal > 2) {
		return { 255, 255, 255, 255 };
	}
	else {
		return feat_portalcolors.colors[portal];
	}
}

void PortalColorsFeature::InitHooks() {
	HOOK_FUNCTION(client, UTIL_Portal_Color);
}

void PortalColorsFeature::LoadFeature() {
	if (ORIG_UTIL_Portal_Color) {
		InitConcommandBase(sn_color_grav);
		InitConcommandBase(sn_color_blue);
		InitConcommandBase(sn_color_orange);
	}
}
