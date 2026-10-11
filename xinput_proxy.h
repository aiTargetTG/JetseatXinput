#pragma once
#include <Windows.h>
#include <Xinput.h>

// ========== Типы указателей на оригинальные функции ==========
typedef DWORD(WINAPI* XInputSetState_t)(DWORD, XINPUT_VIBRATION*);
typedef DWORD(WINAPI* XInputGetState_t)(DWORD, XINPUT_STATE*);
typedef DWORD(WINAPI* XInputGetCapabilities_t)(DWORD, DWORD, XINPUT_CAPABILITIES*);
typedef void(WINAPI* XInputEnable_t)(BOOL);
typedef DWORD(WINAPI* XInputGetDSoundAudioDeviceGuids_t)(DWORD, GUID*, GUID*);
typedef DWORD(WINAPI* XInputGetBatteryInformation_t)(DWORD, BYTE, XINPUT_BATTERY_INFORMATION*);
typedef DWORD(WINAPI* XInputGetKeystroke_t)(DWORD, DWORD, PXINPUT_KEYSTROKE);

// Указатели на функции, которых нет в <Xinput.h> стандарта 1.3
typedef DWORD(WINAPI* XInputGetStateEx_t)(DWORD, XINPUT_STATE*);
typedef DWORD(WINAPI* XInputWaitForGuideButton_t)(DWORD, DWORD, LPVOID);
typedef DWORD(WINAPI* XInputCancelGuideButtonWait_t)(DWORD);
typedef DWORD(WINAPI* XInputPowerOffController_t)(DWORD);

// ========== Указатели на оригинальные функции ==========
extern XInputSetState_t Original_XInputSetState;
extern XInputGetState_t Original_XInputGetState;
extern XInputGetCapabilities_t Original_XInputGetCapabilities;
extern XInputEnable_t Original_XInputEnable;
extern XInputGetDSoundAudioDeviceGuids_t Original_XInputGetDSoundAudioDeviceGuids;
extern XInputGetBatteryInformation_t Original_XInputGetBatteryInformation;
extern XInputGetKeystroke_t Original_XInputGetKeystroke;

extern XInputGetStateEx_t Original_XInputGetStateEx;
extern XInputWaitForGuideButton_t Original_XInputWaitForGuideButton;
extern XInputCancelGuideButtonWait_t Original_XInputCancelGuideButtonWait;
extern XInputPowerOffController_t Original_XInputPowerOffController;

// ========== Управление инициализацией ==========
// Вызывается из DllMain: создаёт поток инициализации.
void StartInitThread(HMODULE hModule);

// Вызывается из DllMain при выгрузке: останавливает всё.
void StopInitThread();

// Ждёт завершения инициализации. Вызывается в начале каждой прокси-функции.
void WaitForStartup();