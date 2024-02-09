#include "feature.hpp"
#include "../plugin.h"
#include <Windows.h>
#include "d3d9.h"

// Mouse input stuff
class InputFeature : public FeatureWrapper<InputFeature> {
public:
	int cx = 0, cy = 0, rx = 0, ry = 0;
	CRITICAL_SECTION rawMouseData;
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

	HWND inwin;
	RAWINPUTDEVICE Rid;
	bool m_bRawInputSupported = false;
	DECL_HOOK_STDCALL(int, GetCursorPos, POINT*);
	DECL_HOOK_STDCALL(BOOL, SetCursorPos, int x, int y);
};

ConVar sn_m_rawinput("sn_m_rawinput", "1", FCVAR_ARCHIVE, "Raw mouse input");
ConVar sn_m_factor("sn_m_factor", "1", FCVAR_ARCHIVE, "Number of hardware mouse counts per step");
static InputFeature feat_input;

bool InputFeature::ShouldLoadFeature() { return true; }

static int m_factor() { return max(1, sn_m_factor.GetInt()); }

void InputFeature::InitHooks() {
	auto handle = GetModuleHandleA("user32.dll");
	ORIG_GetCursorPos = (InputFeature::_GetCursorPos)GetProcAddress(handle, "GetCursorPos");
	ORIG_SetCursorPos = (InputFeature::_SetCursorPos)GetProcAddress(handle, "SetCursorPos");
	if (ORIG_GetCursorPos && ORIG_SetCursorPos) {
		AddRawHook("user32", (void**)&ORIG_GetCursorPos, InputFeature::HOOKED_GetCursorPos);
		AddRawHook("user32", (void**)&ORIG_SetCursorPos, InputFeature::HOOKED_SetCursorPos);
	}
}

IMPL_HOOK_STDCALL(InputFeature, int, GetCursorPos, POINT* p) {
	if (!sn_m_rawinput.GetBool() || !feat_input.m_bRawInputSupported) {
		return feat_input.ORIG_GetCursorPos(p);
	}

	p->x = feat_input.cx;
	p->y = feat_input.cy;
	return 1;
}

IMPL_HOOK_STDCALL(InputFeature, BOOL, SetCursorPos, int x, int y) {
	feat_input.cx = x;
	feat_input.cy = y;
	return feat_input.ORIG_SetCursorPos(x, y);
}

static LRESULT CALLBACK wndproc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case WM_INPUT: {
		char buf[sizeof(RAWINPUTHEADER) + sizeof(RAWMOUSE) /* = 40 */];
		UINT sz = sizeof(buf);
		if (GetRawInputData((HRAWINPUT)lp, RID_INPUT, buf, &sz, sizeof(RAWINPUTHEADER)) != -1) {
			RAWINPUT* ri = (RAWINPUT*)buf;
			if (ri->header.dwType == RIM_TYPEMOUSE) {
				int d = m_factor();

				EnterCriticalSection(&feat_input.rawMouseData);
				int dx = feat_input.rx + ri->data.mouse.lLastX;
				int dy = feat_input.ry + ri->data.mouse.lLastY;
				feat_input.cx += dx / d;
				feat_input.cy += dy / d;
				feat_input.rx = dx % d;
				feat_input.ry = dy % d;
				LeaveCriticalSection(&feat_input.rawMouseData);

				return 0;
			}
		}
		break;
	};

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(wnd, msg, wp, lp);
}

void InputFeature::LoadFeature() {
	if (!ORIG_GetCursorPos || !ORIG_SetCursorPos) {
		return;
	}

	WNDCLASSEXW wc = {.cbSize = sizeof(wc), .lpfnWndProc = wndproc, .lpszClassName = L"RInput"};

	if (!RegisterClassExW(&wc)) {
		Warning("RInput already loaded, sn_m_rawinput not used!\n");
		return;
	}

	m_bRawInputSupported = true;
	inwin = CreateWindowExW(0, L"RInput", L"RInput", 0, 0, 0, 0, 0, 0, 0, 0, 0);

	if (!inwin) {
		goto e1;
	}

	Rid.usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
	Rid.usUsage = 0x02;     // HID_USAGE_GENERIC_MOUSE
	Rid.dwFlags = 0;        // adds mouse and also ignores legacy mouse messages
	Rid.hwndTarget = inwin;

	InitializeCriticalSection(&rawMouseData);

	if (RegisterRawInputDevices(&Rid, 1, sizeof(Rid)) == FALSE) {
		Warning("Unable to register raw input device, sn_m_rawinput is not available\n");
		goto e2;
	}

	InitConcommandBase(sn_m_rawinput);
	InitConcommandBase(sn_m_factor);

	return;

e2:
	DeleteCriticalSection(&rawMouseData);
	DestroyWindow(inwin);
e1:
	UnregisterClassW(L"RInput", 0);
	m_bRawInputSupported = false;
}

static void RemoveRawInputDevice() {
	RAWINPUTDEVICE rd = {
	    .usUsagePage = 0x01,
	    .usUsage = 0x02,
	    .dwFlags = RIDEV_REMOVE,
	    .hwndTarget = 0,
	};
	RegisterRawInputDevices(&rd, 1, sizeof(rd));
}

void InputFeature::UnloadFeature() {
	if (m_bRawInputSupported) {
		RAWINPUTDEVICE rd = {
		    .usUsagePage = 0x01,
		    .usUsage = 0x02,
		    .dwFlags = RIDEV_REMOVE,
		    .hwndTarget = 0,
		};
		DeleteCriticalSection(&rawMouseData);
		DestroyWindow(inwin);
		RemoveRawInputDevice();
		UnregisterClassW(L"RInput", 0);
	}
}
