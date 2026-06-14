#pragma once
#include <Windows.h>


void Indicator_Init(void);
BOOL Indicator_IsEnabled(void);
void Indicator_SetEnabled(BOOL enable);
void Indicator_OnLayoutChanged(LANGID lang);
void Indicator_SetCountdown(float frac);
void Indicator_RefreshAppearance(void);
void Indicator_PreviewShow(void);
void Indicator_Shutdown(void);
