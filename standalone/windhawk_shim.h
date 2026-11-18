// Stand-ins for the Windhawk API, so the mod source builds as a normal exe.
//
// Settings live in mouse-battery-tray.ini next to the exe:
//   [Settings]          pollInterval=60, iconStyle=number, ...
//   [CustomDevice0]     vid=1D57, request=..., levelByte=8, ...
// "customDevices[N].field" in the mod maps to section CustomDeviceN, key field.
#pragma once

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <cwchar>
#include <string>

#define WH_MOD_ID L"mouse-battery-tray"

void MbtShimLog(const wchar_t* format, ...);
int Wh_GetIntSetting(const wchar_t* name, ...);
const wchar_t* Wh_GetStringSetting(const wchar_t* name, ...);
void Wh_FreeStringSetting(const wchar_t* value);

#define Wh_Log(...) MbtShimLog(__VA_ARGS__)
