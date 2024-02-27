#include "convar.h"
#include "feature.hpp"
#include "portal_utils.hpp"
#include "shadow.hpp"
#include "cdll_int.h"
#include "view_shared.h"
#include "utils/math.hpp"
#include "../plugin.h"
#include "signals.hpp"
#include "game_detection.hpp"
#include <d3dx9.h>

// Overlay hook stuff, could combine with overlay renderer as well
class OverlayFeature : public FeatureWrapper<OverlayFeature>
{
public:
	bool hasRenderedOverlay = false;
	bool renderingOverlay = false;
	// these point to the call stack, only valid while we're in RenderView()
	CViewSetup* mainView = nullptr;
	CViewSetup* overlayView = nullptr;
	size_t overlayWidth = 1280, overlayHeight = 720;

	DECL_HOOK_THISCALL(void,
		CViewRender__RenderView,
		void*,
		CViewSetup* cameraView,
		int nClearFlags,
		int whatToDraw);
	static void DrawOverlayCrosshair(IDirect3D9*, IDirect3DDevice9*);

protected:
	virtual void InitHooks() override;

	virtual void PreHook() override;

	virtual void LoadFeature() override;

private:
	int QueueOverlayRenderView_Offset = -1;

	void CViewRender__QueueOverlayRenderView(void* thisptr,
		const CViewSetup& renderView,
		int nClearFlags,
		int whatToDraw);
	void ModifyView(CViewSetup* renderView);
	void ModifyScreenFlags(int& clearFlags, int& drawFlags);
};

ConVar sn_overlay("sn_overlay",
                      "0",
                      FCVAR_DEMO | FCVAR_ARCHIVE,
                      "Enables the overlay camera in the top left of the screen.\n");
ConVar sn_overlay_type("sn_overlay_type",
                           "0",
                           FCVAR_DEMO | FCVAR_ARCHIVE,
                           "OverlayFeature type:\n"
                           "  0 = save glitch body\n"
                           "  1 = angle glitch simulation\n"
                           "  2 = rear view mirror\n"
                           "  3 = havok view mirror\n"
                           "  4 = no camera transform (even when behind SG portal)\n");
ConVar sn_overlay_portal(
    "sn_overlay_portal",
    "auto",
    FCVAR_CHEAT | FCVAR_DEMO | FCVAR_ARCHIVE,
    "Chooses the portal for the overlay camera. Valid options are blue/orange/portal index. For the SG camera this is the portal you save glitch on, for angle glitch simulation this is the portal you enter.\n");
ConVar sn_overlay_width("sn_overlay_width",
                            "480",
                            FCVAR_DEMO | FCVAR_ARCHIVE,
                            "Determines the width of the overlay. Height is determined automatically from width.\n",
                            true,
                            20.0f,
                            true,
                            3840.0f);
ConVar sn_overlay_fov("sn_overlay_fov",
                          "90",
                          FCVAR_DEMO | FCVAR_ARCHIVE,
                          "Determines the FOV of the overlay.\n",
                          true,
                          5.0f,
                          true,
                          140.0f);
ConVar sn_overlay_swap("sn_overlay_swap", "0", FCVAR_DEMO | FCVAR_ARCHIVE, "Swap alternate view and main screen?\n");

OverlayFeature feat_overlay;

namespace patterns
{
	PATTERNS(CViewRender__RenderView,
	         "3420",
	         "55 8B EC 83 E4 F8 81 EC 24 01 00 00");
} // namespace patterns

void OverlayFeature::InitHooks()
{
	HOOK_FUNCTION(client, CViewRender__RenderView);
}

void OverlayFeature::PreHook()
{
	if (!ORIG_CViewRender__RenderView)
		return;

	if (utils::GetBuildNumber() >= 5135)
		QueueOverlayRenderView_Offset = 26;
	else
		QueueOverlayRenderView_Offset = 24;

}

void OverlayFeature::LoadFeature()
{
	signals::DX9_EndScene.Connect(OverlayFeature::DrawOverlayCrosshair);

	InitConcommandBase(sn_overlay);
	InitConcommandBase(sn_overlay_type);
	InitConcommandBase(sn_overlay_portal);
	InitConcommandBase(sn_overlay_width);
	InitConcommandBase(sn_overlay_fov);
	InitConcommandBase(sn_overlay_swap);
}

void OverlayFeature::DrawOverlayCrosshair(IDirect3D9* thisptr, IDirect3DDevice9* pDevice)
{
	if (!CheckCheatVarBool(sn_overlay) || !feat_overlay.hasRenderedOverlay)
	{
		return;
	}

	feat_overlay.hasRenderedOverlay = false;
	const int x = feat_overlay.overlayWidth / 2;
	const int y = feat_overlay.overlayHeight / 2;
	const int width = 10;
	const int thickness = 1;

	D3DRECT rects[3] = {{x - thickness / 2, y - width / 2, x + thickness / 2 + 1, y + width / 2 + 1},
	                   {x - width / 2, y - thickness / 2, x - thickness / 2, y + thickness / 2 + 1},
	                   {x + thickness / 2 + 1, y - thickness / 2, x + width / 2 + 1, y + thickness / 2 + 1}};
	pDevice->Clear(3, rects, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 0, 255, 0), 0.0f, 0);
}

IMPL_HOOK_THISCALL(OverlayFeature,
                   void,
                   CViewRender__RenderView,
                   void*,
                   CViewSetup* cameraView,
                   int nClearFlags,
                   int whatToDraw)
{
	if (!CheckCheatVarBool(sn_overlay))
	{
		feat_overlay.ORIG_CViewRender__RenderView(thisptr, cameraView, nClearFlags, whatToDraw);
		return;
	}

	/*
	* If the overlay is enabled, we'll trigger a recursive call to RenderView() via QueueOverlayRenderView(). You
	* could in theory use this to have several overlays, in which case you'd need to keep track of the overlay
	* depth to make everyone happy.
	*/

	static int callDepth = 0;
	callDepth++;

	// this is hardcoded to false for overlays, so we copy its value to be the same as for the main view
	static bool doBloomAndToneMapping;

	if (callDepth == 1)
	{
		doBloomAndToneMapping = cameraView->m_bDoBloomAndToneMapping;
	}
	else
	{
		feat_overlay.renderingOverlay = true;
		feat_overlay.hasRenderedOverlay = true;
		cameraView->m_bDoBloomAndToneMapping = doBloomAndToneMapping;
	}

	if (!feat_overlay.renderingOverlay)
	{
		// Queue a RenderView() call, this will make a copy of the params that we got called with. This
		// function is in the SDK but has different virtual offsets for 3420/5135 :/.
		feat_overlay.CViewRender__QueueOverlayRenderView(thisptr, *cameraView, nClearFlags, whatToDraw);
	}
	feat_overlay.ModifyView(cameraView);
	feat_overlay.ModifyScreenFlags(nClearFlags, whatToDraw);

	// it just so happens that we don't need to make a full copy of the view and can use pointers instead
	if (feat_overlay.renderingOverlay)
		feat_overlay.overlayView = cameraView;
	else
		feat_overlay.mainView = cameraView;

	feat_overlay.ORIG_CViewRender__RenderView(thisptr, cameraView, nClearFlags, whatToDraw);
	callDepth--;
	if (callDepth == 1)
		feat_overlay.renderingOverlay = false;

}

void OverlayFeature::CViewRender__QueueOverlayRenderView(void* thisptr,
                                                  const CViewSetup& renderView,
                                                  int nClearFlags,
                                                  int whatToDraw)
{
	Assert(QueueOverlayRenderView_Offset != -1);
	typedef void(__thiscall * _QueueOverlayRenderView)(void* thisptr, const CViewSetup&, int, int);
	((_QueueOverlayRenderView**)thisptr)[0][QueueOverlayRenderView_Offset](thisptr,
	                                                                       renderView,
	                                                                       nClearFlags,
	                                                                       whatToDraw);
}

void OverlayFeature::ModifyView(CViewSetup* renderView)
{
	if (renderingOverlay)
	{
		// scale this view to be smol

		renderView->x = 0;
		renderView->y = 0;

		// game does some funny fov scaling, fixup so that overlay fov is scaled in the same way as the main view
		const float half_ang = M_PI_F / 360.f;
		float aspect = renderView->m_flAspectRatio;
		renderView->fov = atan(tan(sn_overlay_fov.GetFloat() * half_ang) * .75f * aspect) / half_ang;
		overlayWidth = renderView->width = sn_overlay_width.GetFloat();
		overlayHeight = renderView->height = static_cast<int>(renderView->width / renderView->m_flAspectRatio);
	}

	if (renderingOverlay != sn_overlay_swap.GetBool())
	{
		// move/rotate this view

		switch (sn_overlay_type.GetInt())
		{
		case 0: // saveglitch offset
			feat_portal_utils.calculateSGPosition(sn_overlay_portal.GetString(), renderView->origin, renderView->angles);
			break;
		case 1: // angle glitch tp pos
			feat_portal_utils.calculateAGPosition(sn_overlay_portal.GetString(), renderView->origin, renderView->angles);
			break;
		case 2: // rear view cam
			renderView->angles.y += 180;
			break;
		case 3: // havok pos
			feat_player_shadow.GetPlayerHavokPos(&renderView->origin, nullptr);
			renderView->origin += feat_entprops.m_vecViewOffset.GetValue();
			break;
		case 4: // don't transform when behind sg portal
			renderView->origin = feat_entprops.GetPlayerEyePosition();
			renderView->angles = feat_entprops.GetPlayerEyeAngles();
			break;
		default:
			break;
		}
		// normalize yaw
		renderView->angles.y = utils::NormalizeDeg(renderView->angles.y);
		renderView->angles.z = 0;
	}
}

void OverlayFeature::ModifyScreenFlags(int& clearFlags, int& drawFlags)
{
	if (renderingOverlay)
	{
		drawFlags = RENDERVIEW_UNSPECIFIED;
		clearFlags |= VIEW_CLEAR_COLOR;
	}
	else if (sn_overlay_swap.GetBool())
	{
		drawFlags &= ~RENDERVIEW_DRAWVIEWMODEL;
	}
}
