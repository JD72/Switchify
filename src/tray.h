#pragma once
#include <Windows.h>

#define TRAY_CALLBACK_MSG (WM_APP + 1)

#define IDM_TOGGLE        1001
#define IDM_AUTOSTART     1002
#define IDM_ABOUT         1003
#define IDM_LOG           1005
#define IDM_EXIT          1004
#define IDM_PRIMARY_MODE   1006
#define IDM_INDICATOR      1007
#define IDM_COUNTDOWN      1008
#define IDM_PRIMARY_BASE   2000
#define MAX_LAYOUTS        16
#define IDM_TIMER_BASE     2100
#define TIMER_PRESET_COUNT 6

#define IDM_IND_COLOR_CUSTOM    1009
#define IDM_IND_COLOR_BASE      2200
#define IND_COLOR_PRESET_COUNT  6
#define IDM_IND_THICK_BASE      2300
#define IND_THICK_PRESET_COUNT  5
#define IDM_IND_LENGTH_BASE     2400
#define IND_LENGTH_PRESET_COUNT 4
#define IDM_IND_EDGE_BASE       2500
#define IDM_IND_ALIGN_BASE      2600

#define IDM_LANG_BASE           2700
#define LANG_COUNT              2

BOOL Tray_Init(HWND ownerWnd, HINSTANCE hInst);

void Tray_SetLanguage(LANGID lang);

void Tray_HandleCallback(WPARAM wParam, LPARAM lParam);

void Tray_Shutdown(void);

HKL Tray_GetMenuLayout(int index);

UINT Tray_GetTimerPreset(int index);

COLORREF Tray_GetIndicatorColorPreset(int index);
UINT     Tray_GetIndicatorThicknessPreset(int index);
UINT     Tray_GetIndicatorLengthPreset(int index);

const char* Tray_GetLangCode(int index);
