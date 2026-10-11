#include "xinput_proxy.h"
#include "jetseat_bridge.h"
#include <string>
#include <cstdio>
#include "log.h"

// ========== Определения указателей ==========
XInputSetState_t Original_XInputSetState = nullptr;
XInputGetState_t Original_XInputGetState = nullptr;
XInputGetCapabilities_t Original_XInputGetCapabilities = nullptr;
XInputEnable_t Original_XInputEnable = nullptr;
XInputGetDSoundAudioDeviceGuids_t Original_XInputGetDSoundAudioDeviceGuids = nullptr;
XInputGetBatteryInformation_t Original_XInputGetBatteryInformation = nullptr;
XInputGetKeystroke_t Original_XInputGetKeystroke = nullptr;

XInputGetStateEx_t Original_XInputGetStateEx = nullptr;
XInputWaitForGuideButton_t Original_XInputWaitForGuideButton = nullptr;
XInputCancelGuideButtonWait_t Original_XInputCancelGuideButtonWait = nullptr;
XInputPowerOffController_t Original_XInputPowerOffController = nullptr;

// ========== Состояние инициализации ==========
static HANDLE g_initDone = nullptr;
static HANDLE g_initThread = nullptr;
static HMODULE g_selfModule = nullptr;

// ========== Загрузка системной библиотеки ==========
static HMODULE LoadSystemXInput(HMODULE hSelf) {
    wchar_t selfPath[MAX_PATH] = { 0 };
    GetModuleFileNameW(hSelf, selfPath, MAX_PATH);

    // Извлекаем имя файла (xinput1_3.dll или xinput1_4.dll)
    const wchar_t* fileName = wcsrchr(selfPath, L'\\');
    fileName = fileName ? fileName + 1 : selfPath;

    wchar_t systemPath[MAX_PATH] = { 0 };
    UINT len = GetSystemDirectoryW(systemPath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return nullptr;

    std::wstring fullPath = std::wstring(systemPath) + L"\\" + fileName;

    char pathUtf8[MAX_PATH * 2] = { 0 };
    WideCharToMultiByte(CP_UTF8, 0, fullPath.c_str(), -1, pathUtf8, sizeof(pathUtf8), nullptr, nullptr);
    Log("[LoadSystemXInput] Trying to load:");
    Log(pathUtf8);

    HMODULE h = LoadLibraryW(fullPath.c_str());
    Log(h ? "[LoadSystemXInput] LoadLibraryW OK" : "[LoadSystemXInput] LoadLibraryW FAILED");
    return h;
}

// ========== Получение указателей на функции ==========
static void ResolveSystemFunctions(HMODULE hSystem) {
    Original_XInputSetState = (XInputSetState_t)GetProcAddress(hSystem, "XInputSetState");
    Original_XInputGetState = (XInputGetState_t)GetProcAddress(hSystem, "XInputGetState");
    Original_XInputGetCapabilities = (XInputGetCapabilities_t)GetProcAddress(hSystem, "XInputGetCapabilities");
    Original_XInputEnable = (XInputEnable_t)GetProcAddress(hSystem, "XInputEnable");
    Original_XInputGetDSoundAudioDeviceGuids = (XInputGetDSoundAudioDeviceGuids_t)GetProcAddress(hSystem, "XInputGetDSoundAudioDeviceGuids");
    Original_XInputGetBatteryInformation = (XInputGetBatteryInformation_t)GetProcAddress(hSystem, "XInputGetBatteryInformation");
    Original_XInputGetKeystroke = (XInputGetKeystroke_t)GetProcAddress(hSystem, "XInputGetKeystroke");

    // Функции 1.4 / недокументированные — по ординалам
    Original_XInputGetStateEx = (XInputGetStateEx_t)GetProcAddress(hSystem, MAKEINTRESOURCEA(100));
    Original_XInputWaitForGuideButton = (XInputWaitForGuideButton_t)GetProcAddress(hSystem, MAKEINTRESOURCEA(101));
    Original_XInputCancelGuideButtonWait = (XInputCancelGuideButtonWait_t)GetProcAddress(hSystem, MAKEINTRESOURCEA(102));
    Original_XInputPowerOffController = (XInputPowerOffController_t)GetProcAddress(hSystem, MAKEINTRESOURCEA(103));

    Log(Original_XInputSetState ? "[Resolve] SetState OK" : "[Resolve] SetState MISSING");
    Log(Original_XInputGetState ? "[Resolve] GetState OK" : "[Resolve] GetState MISSING");
    Log(Original_XInputGetDSoundAudioDeviceGuids ? "[Resolve] DSound OK" : "[Resolve] DSound MISSING");
    Log(Original_XInputGetStateEx ? "[Resolve] StateEx OK" : "[Resolve] StateEx MISSING");
}

// ========== Поток инициализации ==========
static DWORD WINAPI InitThreadProc(LPVOID) {
    Log("[InitThread] Started");

    HMODULE hSystem = LoadSystemXInput(g_selfModule);
    if (hSystem) {
        ResolveSystemFunctions(hSystem);
    }
    else {
        Log("[InitThread] System XInput not loaded");
    }

    // Инициализация JetSeat — тоже в фоновом потоке
    InitJetSeat();

    Log("[InitThread] Finished");
    if (g_initDone) SetEvent(g_initDone);
    return 0;
}

// ========== Публичные функции управления инициализацией ==========
void StartInitThread(HMODULE hModule) {
    g_selfModule = hModule;
    g_initDone = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_initDone) {
        Log("[StartInitThread] CreateEvent failed");
        return;
    }
    g_initThread = CreateThread(nullptr, 0, InitThreadProc, nullptr, 0, nullptr);
    if (!g_initThread) {
        Log("[StartInitThread] CreateThread failed");
        SetEvent(g_initDone);
    }
}

void StopInitThread() {
    if (g_initThread) {
        WaitForSingleObject(g_initThread, 3000);
        CloseHandle(g_initThread);
        g_initThread = nullptr;
    }
    if (g_initDone) {
        CloseHandle(g_initDone);
        g_initDone = nullptr;
    }
}

void WaitForStartup() {
    if (g_initDone) {
        WaitForSingleObject(g_initDone, 3000);
    }
}

// ========== Реализации XInput API ==========

DWORD WINAPI XInputSetState(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration) noexcept {
    WaitForStartup();

    DWORD result = ERROR_DEVICE_NOT_CONNECTED;
    if (Original_XInputSetState) {
        result = Original_XInputSetState(dwUserIndex, pVibration);
    }

    if (pVibration && dwUserIndex == 0) {
        ProcessVibration(pVibration->wLeftMotorSpeed, pVibration->wRightMotorSpeed);
    }

    return result;
}

DWORD WINAPI XInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState) noexcept {
    WaitForStartup();
    if (Original_XInputGetState) {
        return Original_XInputGetState(dwUserIndex, pState);
    }
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities) noexcept {
    WaitForStartup();
    if (Original_XInputGetCapabilities) {
        return Original_XInputGetCapabilities(dwUserIndex, dwFlags, pCapabilities);
    }
    return ERROR_DEVICE_NOT_CONNECTED;
}

void WINAPI XInputEnable(BOOL enable) noexcept {
    WaitForStartup();
    if (Original_XInputEnable) {
        Original_XInputEnable(enable);
    }
}

DWORD WINAPI XInputGetDSoundAudioDeviceGuids(DWORD dwUserIndex, GUID* pDSoundRenderGuid, GUID* pDSoundCaptureGuid) noexcept {
    WaitForStartup();
    if (Original_XInputGetDSoundAudioDeviceGuids) {
        return Original_XInputGetDSoundAudioDeviceGuids(dwUserIndex, pDSoundRenderGuid, pDSoundCaptureGuid);
    }
    if (pDSoundRenderGuid) *pDSoundRenderGuid = GUID_NULL;
    if (pDSoundCaptureGuid) *pDSoundCaptureGuid = GUID_NULL;
    return ERROR_SUCCESS;
}

DWORD WINAPI XInputGetBatteryInformation(DWORD dwUserIndex, BYTE devType, XINPUT_BATTERY_INFORMATION* pBatteryInformation) noexcept {
    WaitForStartup();
    if (Original_XInputGetBatteryInformation) {
        return Original_XInputGetBatteryInformation(dwUserIndex, devType, pBatteryInformation);
    }
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputGetKeystroke(DWORD dwUserIndex, DWORD dwReserved, PXINPUT_KEYSTROKE pKeystroke) noexcept {
    WaitForStartup();
    if (Original_XInputGetKeystroke) {
        return Original_XInputGetKeystroke(dwUserIndex, dwReserved, pKeystroke);
    }
    return ERROR_DEVICE_NOT_CONNECTED;
}

// ========== Недокументированные функции 1.4 ==========

DWORD WINAPI XInputGetStateEx(DWORD dwUserIndex, XINPUT_STATE* pState) noexcept {
    WaitForStartup();
    if (Original_XInputGetStateEx) {
        return Original_XInputGetStateEx(dwUserIndex, pState);
    }
    // Fallback на обычный GetState
    if (Original_XInputGetState) {
        return Original_XInputGetState(dwUserIndex, pState);
    }
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputWaitForGuideButton(DWORD dwUserIndex, DWORD dwFlag, LPVOID pVoid) noexcept {
    WaitForStartup();
    if (Original_XInputWaitForGuideButton) {
        return Original_XInputWaitForGuideButton(dwUserIndex, dwFlag, pVoid);
    }
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputCancelGuideButtonWait(DWORD dwUserIndex) noexcept {
    WaitForStartup();
    if (Original_XInputCancelGuideButtonWait) {
        return Original_XInputCancelGuideButtonWait(dwUserIndex);
    }
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputPowerOffController(DWORD dwUserIndex) noexcept {
    WaitForStartup();
    if (Original_XInputPowerOffController) {
        return Original_XInputPowerOffController(dwUserIndex);
    }
    return ERROR_DEVICE_NOT_CONNECTED;
}