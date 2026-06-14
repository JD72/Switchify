#pragma once
#include <Windows.h>

typedef void (*LayoutChangedCallback)(LANGID lang);

void Layout_Init(LayoutChangedCallback cb);

void Layout_Poll(void);

LANGID Layout_Current(void);

HWND Layout_LastAppWindow(void);
