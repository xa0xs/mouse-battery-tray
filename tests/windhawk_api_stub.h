// Minimal declarations of the Windhawk API, used only to compile-check the
// mod outside Windhawk (Windhawk itself force-includes its own header).
#pragma once
#include <windows.h>

#define WH_MOD_ID L"mouse-battery-tray"
void Wh_Log(const wchar_t* format, ...);
int Wh_GetIntSetting(const wchar_t* name, ...);
const wchar_t* Wh_GetStringSetting(const wchar_t* name, ...);
void Wh_FreeStringSetting(const wchar_t* value);
BOOL Wh_SetFunctionHook(void* target, void* hook, void** original);
