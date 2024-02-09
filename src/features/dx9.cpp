#include "feature.hpp"
#include "../plugin.h"
#include "utils/signals.hpp"

#include <d3d9.h>
#include <d3dx9.h>

// DX9 hook
class DX9Feature : public FeatureWrapper<DX9Feature> {
public:
	bool m_bEvenCall = false;

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void PreHook() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

	//uintptr_t ORIG_EndScene2;
	DECL_HOOK_THISCALL(HRESULT, EndScene, IDirect3D9*, IDirect3DDevice9*);
};

static DX9Feature feat_dx9;

bool DX9Feature::ShouldLoadFeature() { return true; }


namespace patterns
{
	PATTERNS(EndScene,
		"pattern1",
		"6A 14 B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 7D ?? 8B DF 8D 47 ?? F7 DB 1B DB 23 D8 89 5D ?? 33 F6 89 75 ?? 39 73 ?? 75 ??");
}


IMPL_HOOK_THISCALL(DX9Feature, HRESULT, EndScene, IDirect3D9*, IDirect3DDevice9* pDevice) {
	// Source does graphics does two calls for every frame
	// looks a bit ugly but what you gonna do
	if (feat_dx9.m_bEvenCall) {
		signals::DX9_EndScene(thisptr, pDevice);
	}

	feat_dx9.m_bEvenCall = !feat_dx9.m_bEvenCall;

	return feat_dx9.ORIG_EndScene(thisptr, pDevice);
}

void DX9Feature::PreHook() {
	// Slower fallback method in case there's some dx9 version meme
	if (ORIG_EndScene == nullptr) {
		Warning("Hooking dx9 the slow way!\n");
		IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION); // create IDirect3D9 object

		D3DPRESENT_PARAMETERS d3dparams = { 0 };
		d3dparams.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dparams.hDeviceWindow = GetForegroundWindow();
		d3dparams.Windowed = true;

		IDirect3DDevice9* pDevice = nullptr;

		HRESULT result = pD3D->CreateDevice(D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			d3dparams.hDeviceWindow,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING,
			&d3dparams,
			&pDevice);
		if (FAILED(result) || !pDevice) {
			pD3D->Release();
			pD3D = nullptr;
			return;
		}
		//if device creation worked out -> lets get the virtual table:
		void** vftable = *reinterpret_cast<void***>(pDevice);
		feat_dx9.ORIG_EndScene = (DX9Feature::_EndScene)vftable[42];

		Feature::AddRawHook("d3d9.dll", (void**)&feat_dx9.ORIG_EndScene, (void*)DX9Feature::HOOKED_EndScene);
		pD3D->Release();
		pD3D = nullptr;
	}
}

void DX9Feature::InitHooks() {
	HOOK_FUNCTION(d3d9, EndScene);
}

void DX9Feature::LoadFeature() {}

void DX9Feature::UnloadFeature() {}
