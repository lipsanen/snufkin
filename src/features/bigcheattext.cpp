#include "feature.hpp"
#include "../plugin.h"
#include "utils/signals.hpp"
#include "d3d9.h"
#include "d3dx9.h"

// Feature description
class BigCheatTextFeature : public FeatureWrapper<BigCheatTextFeature>
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;
};

static LPD3DXFONT font = nullptr;
static BigCheatTextFeature feat_cheattext;

static void DrawCheatTextFunc(IDirect3D9* thisptr, IDirect3DDevice9* pDevice)
{
	if (!CheatsEnabled())
	{
		return;
	}

	if (!font) {
		D3DXCreateFont(pDevice, 24, 0, FW_REGULAR, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Verdana", &font);
	}

	IDirect3DSwapChain9* chain;
	pDevice->GetSwapChain(0, &chain);
	D3DPRESENT_PARAMETERS params;
	chain->GetPresentParameters(&params);

	RECT textRect;
	textRect.left = 10;
	textRect.top = params.BackBufferHeight - 34;
	textRect.bottom = params.BackBufferHeight;
	textRect.right = 300;
	font->DrawTextA(NULL, "Cheats enabled", -1, &textRect, DT_NOCLIP | DT_LEFT, D3DCOLOR_ARGB(255, 255, 255, 255));
}

bool BigCheatTextFeature::ShouldLoadFeature()
{
	return true;
}

void BigCheatTextFeature::InitHooks() {}

void BigCheatTextFeature::LoadFeature() {
	signals::DX9_EndScene.Connect(DrawCheatTextFunc);
}

void BigCheatTextFeature::UnloadFeature() {}
