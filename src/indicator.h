#pragma once
#include <Windows.h>

#define RAINBOW_TIMER_ID 4

void Indicator_Init(HWND owner);
BOOL Indicator_IsEnabled(void);
void Indicator_SetEnabled(BOOL enable);
void Indicator_OnLayoutChanged(LANGID lang);
void Indicator_SetCountdown(float frac);
void Indicator_OnRainbowTick(void);
void Indicator_RefreshAppearance(void);
void Indicator_PreviewShow(void);
void Indicator_Shutdown(void);
