#include "pch.h"
#include <windows.h>
#include "Bouncing.h"
#include "Game.h"
#include <string>
#include <cstdlib>
#include "UI.h"

// ================= PATH =================
 std::string GetIniPath(HMODULE module)
{
    char path[MAX_PATH];
    GetModuleFileNameA(module, path, MAX_PATH);

    char* slash = strrchr(path, '\\');
    if (slash) *(slash + 1) = 0;

    return std::string(path) + "BouncingCars.ini";
}

 float ReadFloat(const char* section, const char* key, float def, const char* path)
{
    char buf[64];
    GetPrivateProfileStringA(section, key, "", buf, sizeof(buf), path);
    if (buf[0] == 0) return def;
    return (float)atof(buf);
}

 int ReadInt(const char* section, const char* key, int def, const char* path)
{
    return GetPrivateProfileIntA(section, key, def, path);
}

 void LoadConfig(BounceConfig& cfg, const char* path)
{
    cfg.bpm = ReadFloat("BouncingCars", "BPM", 120.0f, path);
    cfg.amplitude = ReadFloat("BouncingCars", "Amplitude", 0.33f, path);
    cfg.easingType = ReadInt("BouncingCars", "Easing", 0, path);
    cfg.enabled = ReadInt("BouncingCars", "Enabled", 1, path);
    cfg.toggleKey = ReadInt("BouncingCars", "ToggleKey", VK_F2, path);
    cfg.menuKey = ReadInt("BouncingCars", "MenuKey", VK_F1, path);
    if (cfg.amplitude > 2.0f) cfg.amplitude = 2.0f;
}

 void WriteFloat(const char* section, const char* key, float value, const char* path)
{
    char buf[64];
    sprintf_s(buf, "%f", value);
    WritePrivateProfileStringA(section, key, buf, path);
}

 void WriteInt(const char* section, const char* key, int value, const char* path)
{
    char buf[32];
    sprintf_s(buf, "%d", value);
    WritePrivateProfileStringA(section, key, buf, path);
}

 void SaveConfig(const BounceConfig& cfg, const char* path)
{
    WriteFloat("BouncingCars", "BPM", cfg.bpm, path);
    WriteFloat("BouncingCars", "Amplitude", cfg.amplitude, path);
    WriteInt("BouncingCars", "Easing", cfg.easingType, path);
    WriteInt("BouncingCars", "Enabled", cfg.enabled, path);
    WriteInt("BouncingCars", "ToggleKey", cfg.toggleKey, path);
    WriteInt("BouncingCars", "MenuKey", cfg.menuKey, path);
}

BounceConfig cfg;

Matrix4& CarScaleMatrix = *(Matrix4*)0x9B34B0;

HMODULE gModule = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD reason,
    LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        gModule = hModule;

        DisableThreadLibraryCalls(hModule);

        // init hook D3D
        CreateThread(nullptr, 0, [](LPVOID) -> DWORD
            {
                InitD3DHook();
                return 0;
            }, nullptr, 0, nullptr);

        std::string iniPath = GetIniPath(gModule);
        LoadConfig(cfg, iniPath.c_str());
    }
    return TRUE;
}
