#include "layout.h"
#include <string.h>

static LANGID g_currentLang = 0;
static LayoutChangedCallback g_cb = NULL;
static HWND  g_lastAppWnd = NULL;

static BOOL IsIgnorableForeground(HWND fg)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    if (pid == GetCurrentProcessId()) return TRUE;

    char cls[64] = { 0 };
    GetClassNameA(fg, cls, sizeof(cls));
    return (strcmp(cls, "Shell_TrayWnd") == 0) || (strcmp(cls, "Shell_SecondaryTrayWnd") == 0);
}

static LANGID GetForegroundLang(void)
{
    HWND fg = GetForegroundWindow();
    if (!fg)
    {
        return 0;
    }
    if (IsIgnorableForeground(fg))
    {
        return 0;
    }

    g_lastAppWnd = fg;

    HWND target = fg;
    GUITHREADINFO gti = { sizeof(gti) };
    if (GetGUIThreadInfo(0, &gti) && gti.hwndFocus)
    {
        target = gti.hwndFocus;
    }

    DWORD tid = GetWindowThreadProcessId(target, NULL);
    HKL hkl = GetKeyboardLayout(tid);
    return LOWORD(hkl);
}

HWND Layout_LastAppWindow(void)
{
    return g_lastAppWnd;
}

void Layout_Init(LayoutChangedCallback cb)
{
    g_cb = cb;
    LANGID lang = GetForegroundLang();
    if (lang != 0)
    {
        g_currentLang = lang;
    }
}

void Layout_Poll(void)
{
    LANGID lang = GetForegroundLang();
    if (lang == 0)
    {
        return;
    }
    if (lang != g_currentLang)
    {
        g_currentLang = lang;
        if (g_cb)
        {
            g_cb(lang);
        }
    }
}

LANGID Layout_Current(void)
{
    return g_currentLang;
}
