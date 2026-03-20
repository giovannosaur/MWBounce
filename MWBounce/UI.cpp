#include "pch.h"
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "MinHook.h"

#include "Bouncing.h"
#include "Game.h"
#include <stdio.h>
#include <string>

extern std::string GetIniPath(HMODULE);
extern void SaveConfig(const BounceConfig&, const char*);
extern HMODULE gModule;

// dari main
extern BounceConfig cfg;

// ================= GLOBAL =================
LPD3DXFONT gFont = nullptr;
bool showMenu = false;

int selected = 0;
const int maxItems = 5;

static float gTime = 0.0f;

const char* easingNames[] = {
    "Linear",
    "Sine",
    "Quad",
    "Cubic",
    "Quart",
    "Quint",
    "Expo",
    "Circ",
    "Back",
    "Elastic",
    "Bounce"
};

// ================= TYPE =================
typedef HRESULT(__stdcall* EndScene_t)(LPDIRECT3DDEVICE9);
EndScene_t oEndScene = nullptr;

typedef HRESULT(__stdcall* Reset_t)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
Reset_t oReset = nullptr;

HRESULT __stdcall hkReset(LPDIRECT3DDEVICE9 device, D3DPRESENT_PARAMETERS* pp)
{
    if (gFont)
        gFont->OnLostDevice();

    HRESULT hr = oReset(device, pp);

    if (gFont)
        gFont->OnResetDevice();

    return hr;
}

// ================= UI =================
void RenderSimpleUI(LPDIRECT3DDEVICE9 device)
{
    if (!gFont)
    {
        D3DXCreateFontA(device, 18, 0, FW_BOLD, 1, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            "Arial", &gFont);
    }

    if (!showMenu) return;

    RECT rect = { 50, 50, 400, 400 };

    char buf[512];

    sprintf_s(buf,
        "%s BPM: %.1f\n"
        "%s Amplitude: %.2f\n"
        "%s Easing: %s\n"
        "%s Enabled: %s\n"
        "%s Save Config\n",

        selected == 0 ? ">" : " ", cfg.bpm,
        selected == 1 ? ">" : " ", cfg.amplitude,
        selected == 2 ? ">" : " ", easingNames[cfg.easingType],
        selected == 3 ? ">" : " ", cfg.enabled ? "ON" : "OFF",
        selected == 4 ? ">" : " "
    );

    gFont->DrawTextA(NULL, buf, -1, &rect, DT_NOCLIP, D3DCOLOR_ARGB(255, 255, 255, 255));
}

// ================= INPUT =================
void HandleInput()
{
    if (!showMenu) return;

    static bool prevI = false, prevK = false, prevJ = false, prevL = false;

    bool nowI = (GetAsyncKeyState('I') & 0x8000) != 0;
    bool nowK = (GetAsyncKeyState('K') & 0x8000) != 0;
    bool nowJ = (GetAsyncKeyState('J') & 0x8000) != 0;
    bool nowL = (GetAsyncKeyState('L') & 0x8000) != 0;

    static float holdTimeJ = 0.0f;
    static float holdTimeL = 0.0f;

    const float initialDelay = 0.4f;
    const float repeatRate = 0.05f;

    float dt = *(float*)0x009142D8;

    // ===== NAVIGASI =====
    if (nowI && !prevI) selected--;
    if (nowK && !prevK) selected++;

    if (selected < 0) selected = maxItems - 1;
    if (selected >= maxItems) selected = 0;

    // ===== TAP (J) =====
    if (nowJ && !prevJ)
    {
        holdTimeJ = 0.0f;

        switch (selected)
        {
        case 0: cfg.bpm -= 1.0f; break;
        case 1: cfg.amplitude -= 0.01f; break;
        case 2: cfg.easingType = (cfg.easingType - 1 + 11) % 11; break;
        case 3: cfg.enabled = !cfg.enabled; break;
        }
    }

    // ===== TAP (L) =====
    if (nowL && !prevL)
    {
        holdTimeL = 0.0f;

        switch (selected)
        {
        case 0: cfg.bpm += 1.0f; break;
        case 1: cfg.amplitude += 0.01f; break;
        case 2: cfg.easingType = (cfg.easingType + 1) % 11; break;
        case 3: cfg.enabled = !cfg.enabled; break;
        case 4:
        {
            std::string path = GetIniPath(gModule);
            SaveConfig(cfg, path.c_str());
            break;
        }
        }
    }

    // ===== HOLD (J) =====
    if (nowJ)
    {
        holdTimeJ += dt;

        if (holdTimeJ > initialDelay)
        {
            holdTimeJ -= repeatRate;

            switch (selected)
            {
            case 0: cfg.bpm -= 1.0f; break;
            case 1: cfg.amplitude -= 0.01f; break;
            case 2: cfg.easingType = (cfg.easingType - 1 + 11) % 11; break;
            case 3: cfg.enabled = !cfg.enabled; break;
            }
        }
    }
    else
    {
        holdTimeJ = 0.0f;
    }

    // ===== HOLD (L) =====
    if (nowL)
    {
        holdTimeL += dt;

        if (holdTimeL > initialDelay)
        {
            holdTimeL -= repeatRate;

            switch (selected)
            {
            case 0: cfg.bpm += 1.0f; break;
            case 1: cfg.amplitude += 0.01f; break;
            case 2: cfg.easingType = (cfg.easingType + 1) % 11; break;
            case 3: cfg.enabled = !cfg.enabled; break;
            case 4:
            {
                std::string path = GetIniPath(gModule);
                SaveConfig(cfg, path.c_str());
                break;
            }
            }
        }
    }
    else
    {
        holdTimeL = 0.0f;
    }

    prevI = nowI;
    prevK = nowK;
    prevJ = nowJ;
    prevL = nowL;
}

// ================= HOOK =================
HRESULT __stdcall hkEndScene(LPDIRECT3DDEVICE9 device)
{
    // toggle menu
    static bool prev = false;
    bool now = (GetAsyncKeyState(cfg.menuKey) & 0x8000) != 0;

    if (now && !prev)
        showMenu = !showMenu;

    prev = now;

    HandleInput();

    // ===== TIME FIX =====
    float dt = *(float*)0x009142D8;
    gTime += dt;

    float globalTime = gTime;

    // ===== APPLY =====
    if (cfg.enabled)
    {
        ApplyBounce(CarScaleMatrix, globalTime, cfg);
    }
    else
    {
        CarScaleMatrix.x = { 1,0,0,0 };
        CarScaleMatrix.y = { 0,1,0,0 };
        CarScaleMatrix.z = { 0,0,1,0 };
        CarScaleMatrix.p = { 0,0,0,1 };
    }

    RenderSimpleUI(device);

    return oEndScene(device);
}

// ================= INIT =================
void InitD3DHook()
{
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);

    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = GetForegroundWindow();

    IDirect3DDevice9* dummyDevice = nullptr;

    if (d3d->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        d3dpp.hDeviceWindow,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &d3dpp,
        &dummyDevice) < 0)
    {
        return;
    }

    void** vtable = *reinterpret_cast<void***>(dummyDevice);
    void* endSceneAddr = vtable[42]; // D3D9 EndScene

    MH_Initialize();
    MH_CreateHook(endSceneAddr, &hkEndScene, reinterpret_cast<void**>(&oEndScene));
    MH_CreateHook(vtable[16], hkReset, reinterpret_cast<void**>(&oReset));
    MH_EnableHook(MH_ALL_HOOKS);

    dummyDevice->Release();
    d3d->Release();
}