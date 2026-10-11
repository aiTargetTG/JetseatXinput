#pragma once
#include <Windows.h>

// Инициализация/остановка
void InitJetSeat();
void ShutdownJetSeat();

// Обработка вибрации (пятый вариант: стерео-маппинг)
void ProcessVibration(WORD leftMotor, WORD rightMotor);