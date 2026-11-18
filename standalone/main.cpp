// Mouse Battery Tray, standalone build: the Windhawk mod source plus a small
// shim, so the same code runs without Windhawk.

#define MBT_STANDALONE 1
#include "windhawk_shim.h"

#include "../mods/mouse-battery-tray.wh.cpp"

#include <shlobj.h>

namespace {

struct Default {
    const wchar_t* key;
    const wchar_t* value;
};

// Must match the defaults in the mod's settings block.
const Default kDefaults[] = {
    {L"pollInterval", L"60"},     {L"iconMode", L"perDevice"},
    {L"iconStyle", L"number"},    {L"colorByLevel", L"1"},
    {L"lowBattery", L"20"},       {L"showWhenNone", L"1"},
    {L"bluetooth", L"1"},         {L"logitech", L"1"},
    {L"razer", L"1"},             {L"probeUnknown", L"1"},
    {L"allDevices", L"0"},        {L"method", L"feature"},
    {L"levelByte", L"8"},         {L"chargingByte", L"-1"},
    {L"levelScale", L"100"},
};

std::wstring g_iniPath;

std::wstring ExeDir() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring s = path;
    return s.substr(0, s.find_last_of(L'\\') + 1);
}

// "customDevices[2].vid" -> section "CustomDevice2", key "vid".
void SplitName(const std::wstring& name, std::wstring& section, std::wstring& key) {
    size_t open = name.find(L'[');
    size_t close = name.find(L"].");
    if (name.rfind(L"customDevices[", 0) == 0 && open != std::wstring::npos &&
        close != std::wstring::npos) {
        section = L"CustomDevice" + name.substr(open + 1, close - open - 1);
        key = name.substr(close + 2);
    } else {
        section = L"Settings";
        key = name;
    }
}

const wchar_t* DefaultFor(const std::wstring& key) {
    for (auto& d : kDefaults) {
        if (key == d.key) {
            return d.value;
        }
    }
    return L"";
}

std::wstring Lookup(const wchar_t* format, va_list args) {
    wchar_t name[256];
    _vsnwprintf(name, 255, format, args);
    name[255] = 0;
    std::wstring section, key;
    SplitName(name, section, key);
    wchar_t value[1024];
    GetPrivateProfileStringW(section.c_str(), key.c_str(), DefaultFor(key), value,
                             ARRAYSIZE(value), g_iniPath.c_str());
    return value;
}

void WriteDefaultIni() {
    if (GetFileAttributesW(g_iniPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return;
    }
    const char* text =
        "; Mouse Battery Tray settings. Changes apply after a restart of the app\r\n"
        "; (right-click the tray icon > Exit, then start it again).\r\n"
        "\r\n"
        "[Settings]\r\n"
        "; seconds between battery reads (10-3600)\r\n"
        "pollInterval=60\r\n"
        "; perDevice = one icon per mouse, lowest = one icon for the lowest battery\r\n"
        "iconMode=perDevice\r\n"
        "; number or battery\r\n"
        "iconStyle=number\r\n"
        "colorByLevel=1\r\n"
        "; low battery notification threshold in %, 0 = off\r\n"
        "lowBattery=20\r\n"
        "showWhenNone=1\r\n"
        "bluetooth=1\r\n"
        "logitech=1\r\n"
        "razer=1\r\n"
        "; send the common read-only battery queries to unknown mouse receivers\r\n"
        "probeUnknown=1\r\n"
        "; also show keyboards, headsets and other devices\r\n"
        "allDevices=0\r\n"
        "\r\n"
        "; Custom devices: copy this section as [CustomDevice0], [CustomDevice1]...\r\n"
        "; See docs/ADDING_A_MOUSE.md.\r\n"
        ";[CustomDevice0]\r\n"
        ";name=My mouse\r\n"
        ";vid=1D57\r\n"
        ";pid=\r\n"
        ";usagePage=\r\n"
        ";method=feature\r\n"
        ";request=00 00 00 02 02 00 83\r\n"
        ";match=1=A1,6=83\r\n"
        ";levelByte=8\r\n"
        ";chargingByte=7\r\n"
        ";levelScale=100\r\n";
    HANDLE f = CreateFileW(g_iniPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(f, text, (DWORD)strlen(text), &written, nullptr);
        CloseHandle(f);
    }
}

const wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t kRunValue[] = L"MouseBatteryTray";

}  // namespace

void MbtShimLog(const wchar_t* format, ...) {
    wchar_t buf[1024];
    va_list args;
    va_start(args, format);
    _vsnwprintf(buf, 1022, format, args);
    va_end(args);
    buf[1022] = 0;
    wcscat_s(buf, L"\n");
    OutputDebugStringW(buf);
}

int Wh_GetIntSetting(const wchar_t* name, ...) {
    va_list args;
    va_start(args, name);
    std::wstring v = Lookup(name, args);
    va_end(args);
    if (v == L"true") return 1;
    if (v == L"false") return 0;
    return _wtoi(v.c_str());
}

const wchar_t* Wh_GetStringSetting(const wchar_t* name, ...) {
    va_list args;
    va_start(args, name);
    std::wstring v = Lookup(name, args);
    va_end(args);
    return _wcsdup(v.c_str());
}

void Wh_FreeStringSetting(const wchar_t* value) {
    free((void*)value);
}

bool StandaloneAutostartEnabled() {
    wchar_t value[MAX_PATH * 2];
    DWORD size = sizeof(value);
    return RegGetValueW(HKEY_CURRENT_USER, kRunKey, kRunValue, RRF_RT_REG_SZ,
                        nullptr, value, &size) == ERROR_SUCCESS;
}

void StandaloneSetAutostart(bool enable) {
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_SET_VALUE, &key) !=
        ERROR_SUCCESS) {
        return;
    }
    if (enable) {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::wstring cmd = L"\"" + std::wstring(path) + L"\"";
        RegSetValueExW(key, kRunValue, 0, REG_SZ, (const BYTE*)cmd.c_str(),
                       (DWORD)((cmd.size() + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(key, kRunValue);
    }
    RegCloseKey(key);
}

void StandaloneOpenSettings() {
    WriteDefaultIni();
    ShellExecuteW(nullptr, L"open", L"notepad.exe", g_iniPath.c_str(), nullptr,
                  SW_SHOWNORMAL);
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"MouseBatteryTray_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }
    using SetDpiCtx_t = BOOL(WINAPI*)(HANDLE);
    auto setDpi = (SetDpiCtx_t)GetProcAddress(GetModuleHandleW(L"user32.dll"),
                                             "SetProcessDpiAwarenessContext");
    if (setDpi) {
        setDpi((HANDLE)-4);  // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
    }

    g_iniPath = ExeDir() + L"mouse-battery-tray.ini";
    WriteDefaultIni();

    if (!WhTool_ModInit()) {
        return 1;
    }
    // Runs until "Exit" closes the tray window, which also stops the worker.
    WaitForSingleObject(mbt::g_uiThread, INFINITE);
    WhTool_ModUninit();
    if (mutex) {
        CloseHandle(mutex);
    }
    return 0;
}
