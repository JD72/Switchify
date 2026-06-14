#pragma once
#include <Windows.h>

#define PRIMARY_TIMER_ID 2

void PrimaryMode_Init(HWND ownerWnd);

BOOL PrimaryMode_IsEnabled(void);
void PrimaryMode_SetEnabled(BOOL enable);

HKL  PrimaryMode_GetPrimary(void);
void PrimaryMode_SetPrimary(HKL hkl);

void PrimaryMode_OnKeyActivity(void);
void PrimaryMode_OnLayoutChanged(LANGID lang);
void PrimaryMode_OnTimer(void);

BOOL  PrimaryMode_IsCountingDown(void);
float PrimaryMode_RemainingFraction(void);

void PrimaryMode_EnsurePrimary(void);
