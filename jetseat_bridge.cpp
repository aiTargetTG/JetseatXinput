#include "jetseat_bridge.h"
#include <initguid.h>
#include "IUberwoorf.h"
#include <cmath>
#include <cstdio>
#include "log.h"

// Кэш последних отправленных амплитуд
static BYTE g_lastLegL = 255;
static BYTE g_lastLegR = 255;
static BYTE g_lastLumbarL = 255;
static BYTE g_lastLumbarR = 255;
static BYTE g_lastBackL = 255;
static BYTE g_lastBackR = 255;

// COM-объект JetSeat
static IUberwoorf* g_jetseat = nullptr;

// Хендлы эффектов (6 зон)
static UINT g_effectLegL = 0, g_effectLegR = 0;
static UINT g_effectLumbarL = 0, g_effectLumbarR = 0;
static UINT g_effectBackL = 0, g_effectBackR = 0;

// Хендлы вибраций
static UINT g_vibLegL = 0, g_vibLegR = 0;
static UINT g_vibLumbarL = 0, g_vibLumbarR = 0;
static UINT g_vibBackL = 0, g_vibBackR = 0;

// Флаг инициализации
static bool g_initialized = false;

void InitJetSeat() {
    Log("[InitJetSeat] Called");
    if (g_initialized) {
        Log("[InitJetSeat] Already initialized");
        return;
    }

    // Инициализация COM (как в irFFB)
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        char buf[64];
        sprintf_s(buf, "[InitJetSeat] CoInitializeEx failed hr=0x%08X", hr);
        Log(buf);
        return;
    }
    Log("[InitJetSeat] CoInitializeEx OK");

    hr = CoInitializeSecurity(
        NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL
    );
    Log("[InitJetSeat] CoInitializeSecurity done");

    // Создание COM-объекта
    hr = CoCreateInstance(CLSID_Uberwoorf, NULL, CLSCTX_ALL,
        IID_IUberwoorf, (void**)&g_jetseat);
    if (FAILED(hr) || !g_jetseat) {
        char buf[64];
        sprintf_s(buf, "[InitJetSeat] CoCreateInstance failed hr=0x%08X", hr);
        Log(buf);
        return;
    }
    Log("[InitJetSeat] CoCreateInstance OK");

    // Открытие устройства
    UINT openStatus = 0;
    hr = g_jetseat->uwOpen(&openStatus);
    if (FAILED(hr)) {
        char buf[64];
        sprintf_s(buf, "[InitJetSeat] uwOpen failed hr=0x%08X status=%u", hr, openStatus);
        Log(buf);
        return;
    }
    Log("[InitJetSeat] uwOpen OK");

    hr = g_jetseat->uwReset();
    Log("[InitJetSeat] uwReset done");

    // === Создание эффектов для 6 зон ===
    HRESULT hrCreate;

    hrCreate = g_jetseat->uwCreateEffect(&g_effectLegL);
    char buf[128];
    sprintf_s(buf, "[InitJetSeat] CreateEffect LegL hr=0x%08X handle=%u", hrCreate, g_effectLegL);
    Log(buf);
    g_jetseat->uwSetEffectGain(g_effectLegL, 100);

    hrCreate = g_jetseat->uwCreateEffect(&g_effectLegR);
    sprintf_s(buf, "[InitJetSeat] CreateEffect LegR hr=0x%08X handle=%u", hrCreate, g_effectLegR);
    Log(buf);
    g_jetseat->uwSetEffectGain(g_effectLegR, 100);

    hrCreate = g_jetseat->uwCreateEffect(&g_effectLumbarL);
    sprintf_s(buf, "[InitJetSeat] CreateEffect LumbarL hr=0x%08X handle=%u", hrCreate, g_effectLumbarL);
    Log(buf);
    g_jetseat->uwSetEffectGain(g_effectLumbarL, 100);

    hrCreate = g_jetseat->uwCreateEffect(&g_effectLumbarR);
    sprintf_s(buf, "[InitJetSeat] CreateEffect LumbarR hr=0x%08X handle=%u", hrCreate, g_effectLumbarR);
    Log(buf);
    g_jetseat->uwSetEffectGain(g_effectLumbarR, 100);

    hrCreate = g_jetseat->uwCreateEffect(&g_effectBackL);
    sprintf_s(buf, "[InitJetSeat] CreateEffect BackL hr=0x%08X handle=%u", hrCreate, g_effectBackL);
    Log(buf);
    g_jetseat->uwSetEffectGain(g_effectBackL, 100);

    hrCreate = g_jetseat->uwCreateEffect(&g_effectBackR);
    sprintf_s(buf, "[InitJetSeat] CreateEffect BackR hr=0x%08X handle=%u", hrCreate, g_effectBackR);
    Log(buf);
    g_jetseat->uwSetEffectGain(g_effectBackR, 100);

    // Заводим вибрацию с амплитудой 0, конечной длительностью 60 секунд
    UINT dur[2] = { 0, 60000 };
    BYTE amp[2] = { 0, 0 };
    HRESULT hrSet;

    hrSet = g_jetseat->uwSetVibration(g_effectLegL, UW_SIT_LEFT, dur, amp, 2, &g_vibLegL);
    sprintf_s(buf, "[InitJetSeat] SetVibration LegL hr=0x%08X vib=%u", hrSet, g_vibLegL);
    Log(buf);

    hrSet = g_jetseat->uwSetVibration(g_effectLegR, UW_SIT_RIGHT, dur, amp, 2, &g_vibLegR);
    sprintf_s(buf, "[InitJetSeat] SetVibration LegR hr=0x%08X vib=%u", hrSet, g_vibLegR);
    Log(buf);

    hrSet = g_jetseat->uwSetVibration(g_effectLumbarL, UW_BACK_LOW_LEFT, dur, amp, 2, &g_vibLumbarL);
    sprintf_s(buf, "[InitJetSeat] SetVibration LumbarL hr=0x%08X vib=%u", hrSet, g_vibLumbarL);
    Log(buf);

    hrSet = g_jetseat->uwSetVibration(g_effectLumbarR, UW_BACK_LOW_RIGHT, dur, amp, 2, &g_vibLumbarR);
    sprintf_s(buf, "[InitJetSeat] SetVibration LumbarR hr=0x%08X vib=%u", hrSet, g_vibLumbarR);
    Log(buf);

    hrSet = g_jetseat->uwSetVibration(g_effectBackL, UW_BACK_MID_LEFT, dur, amp, 2, &g_vibBackL);
    sprintf_s(buf, "[InitJetSeat] SetVibration BackL hr=0x%08X vib=%u", hrSet, g_vibBackL);
    Log(buf);

    hrSet = g_jetseat->uwSetVibration(g_effectBackR, UW_BACK_MID_RIGHT, dur, amp, 2, &g_vibBackR);
    sprintf_s(buf, "[InitJetSeat] SetVibration BackR hr=0x%08X vib=%u", hrSet, g_vibBackR);
    Log(buf);

    // Запускаем все эффекты (count=1, чтобы проверить, работает ли старт)
    HRESULT hrStart;

    hrStart = g_jetseat->uwStartEffect(g_effectLegL, INFINITE);
    sprintf_s(buf, "[InitJetSeat] StartEffect LegL hr=0x%08X", hrStart);
    Log(buf);

    hrStart = g_jetseat->uwStartEffect(g_effectLegR, INFINITE);
    sprintf_s(buf, "[InitJetSeat] StartEffect LegR hr=0x%08X", hrStart);
    Log(buf);

    hrStart = g_jetseat->uwStartEffect(g_effectLumbarL, INFINITE);
    sprintf_s(buf, "[InitJetSeat] StartEffect LumbarL hr=0x%08X", hrStart);
    Log(buf);

    hrStart = g_jetseat->uwStartEffect(g_effectLumbarR, INFINITE);
    sprintf_s(buf, "[InitJetSeat] StartEffect LumbarR hr=0x%08X", hrStart);
    Log(buf);

    hrStart = g_jetseat->uwStartEffect(g_effectBackL, INFINITE);
    sprintf_s(buf, "[InitJetSeat] StartEffect BackL hr=0x%08X", hrStart);
    Log(buf);

    hrStart = g_jetseat->uwStartEffect(g_effectBackR, INFINITE);
    sprintf_s(buf, "[InitJetSeat] StartEffect BackR hr=0x%08X", hrStart);
    Log(buf);

    //Сброс кэша при повторной инициализации
    g_lastLegL = 255;
    g_lastLegR = 255;
    g_lastLumbarL = 255;
    g_lastLumbarR = 255;
    g_lastBackL = 255;
    g_lastBackR = 255;

    g_initialized = true;
    Log("[InitJetSeat] DONE");
}

void ShutdownJetSeat() {
    // Намеренно ничего не делаем.
    // COM-объект освобождать в DllMain при DLL_PROCESS_DETACH нельзя:
    // uwapi.dll может быть выгружена раньше нашей DLL, что вызовет ACCESS_VIOLATION.
    g_jetseat = nullptr;
    g_initialized = false;
}

// ============================================================
// ПЯТЫЙ ВАРИАНТ: СТЕРЕО-МАППИНГ ПО РАЗНИЦЕ КАНАЛОВ
// + ТРОТТЛИНГ: обновляем драйвер только при значимом изменении
// ============================================================


// Порог изменения: если амплитуда изменилась меньше чем на столько,
// команда драйверу не отправляется
static const BYTE AMP_THRESHOLD = 3;

// Проверка: нужно ли обновлять зону
static inline bool NeedsUpdate(BYTE current, BYTE last) {
    if (current > last) {
        return (current - last) >= AMP_THRESHOLD;
    }
    return (last - current) >= AMP_THRESHOLD;
}

void ProcessVibration(WORD leftMotor, WORD rightMotor) {
    if (!g_initialized || !g_jetseat) {
        Log("[ProcessVibration] JetSeat not initialized, skipping");
        return;
    }

    // Конвертация XInput (0..65535) -> JetSeat BYTE (0..255)
    BYTE ampLeft = (BYTE)(leftMotor >> 8);
    BYTE ampRight = (BYTE)(rightMotor >> 8);

    WORD avg = ((UINT)leftMotor + (UINT)rightMotor) / 2;
    BYTE ampBack = (BYTE)(avg >> 8);
    BYTE ampLumbar = (BYTE)(avg >> 8);

    BYTE ampLegL = (leftMotor > rightMotor) ? ampLeft : 0;
    BYTE ampLegR = (rightMotor > leftMotor) ? ampRight : 0;

    // Паттерн: startDelay=0, duration=100 мс, обе точки с одинаковой амплитудой
    UINT dur[2] = { 0, 100 };
    HRESULT hr;

    // ===== LegL =====
    if (NeedsUpdate(ampLegL, g_lastLegL)) {
        BYTE a[2] = { ampLegL, ampLegL };
        hr = g_jetseat->uwUpdateVibration(g_effectLegL, g_vibLegL, UW_SIT_LEFT, dur, a, 2);
        if (FAILED(hr)) {
            char buf[128];
            sprintf_s(buf, "[ProcessVibration] LegL hr=0x%08X amp=%u", hr, ampLegL);
            Log(buf);
        }
        g_lastLegL = ampLegL;
    }

    // ===== LegR =====
    if (NeedsUpdate(ampLegR, g_lastLegR)) {
        BYTE a[2] = { ampLegR, ampLegR };
        hr = g_jetseat->uwUpdateVibration(g_effectLegR, g_vibLegR, UW_SIT_RIGHT, dur, a, 2);
        if (FAILED(hr)) {
            char buf[128];
            sprintf_s(buf, "[ProcessVibration] LegR hr=0x%08X amp=%u", hr, ampLegR);
            Log(buf);
        }
        g_lastLegR = ampLegR;
    }

    // ===== LumbarL =====
    if (NeedsUpdate(ampLumbar, g_lastLumbarL)) {
        BYTE a[2] = { ampLumbar, ampLumbar };
        hr = g_jetseat->uwUpdateVibration(g_effectLumbarL, g_vibLumbarL, UW_BACK_LOW_LEFT, dur, a, 2);
        if (FAILED(hr)) {
            char buf[128];
            sprintf_s(buf, "[ProcessVibration] LumbarL hr=0x%08X amp=%u", hr, ampLumbar);
            Log(buf);
        }
        g_lastLumbarL = ampLumbar;
    }

    // ===== LumbarR =====
    if (NeedsUpdate(ampLumbar, g_lastLumbarR)) {
        BYTE a[2] = { ampLumbar, ampLumbar };
        hr = g_jetseat->uwUpdateVibration(g_effectLumbarR, g_vibLumbarR, UW_BACK_LOW_RIGHT, dur, a, 2);
        if (FAILED(hr)) {
            char buf[128];
            sprintf_s(buf, "[ProcessVibration] LumbarR hr=0x%08X amp=%u", hr, ampLumbar);
            Log(buf);
        }
        g_lastLumbarR = ampLumbar;
    }

    // ===== BackL =====
    if (NeedsUpdate(ampBack, g_lastBackL)) {
        BYTE a[2] = { ampBack, ampBack };
        hr = g_jetseat->uwUpdateVibration(g_effectBackL, g_vibBackL, UW_BACK_MID_LEFT, dur, a, 2);
        if (FAILED(hr)) {
            char buf[128];
            sprintf_s(buf, "[ProcessVibration] BackL hr=0x%08X amp=%u", hr, ampBack);
            Log(buf);
        }
        g_lastBackL = ampBack;
    }

    // ===== BackR =====
    if (NeedsUpdate(ampBack, g_lastBackR)) {
        BYTE a[2] = { ampBack, ampBack };
        hr = g_jetseat->uwUpdateVibration(g_effectBackR, g_vibBackR, UW_BACK_MID_RIGHT, dur, a, 2);
        if (FAILED(hr)) {
            char buf[128];
            sprintf_s(buf, "[ProcessVibration] BackR hr=0x%08X amp=%u", hr, ampBack);
            Log(buf);
        }
        g_lastBackR = ampBack;
    }
}