#include "primary_layout.h"
#include "config.h"
#include "layout.h"

#define MAX_PL_LAYOUTS 16

static HWND  g_ownerWnd = NULL;
static BOOL  g_enabled = FALSE;
static HKL   g_primaryHkl = NULL;
static DWORD g_lastActivityTick = 0;
static BOOL  g_timerActive = FALSE;
static int   g_returnAttempts = 0;

static HKL ResolveHklByLang(LANGID lang)
{
	if (lang == 0) return NULL;
	HKL list[MAX_PL_LAYOUTS];
	int n = GetKeyboardLayoutList(MAX_PL_LAYOUTS, list);
	for (int i = 0; i < n; i++)
	{
		if (LOWORD((DWORD_PTR)list[i]) == lang) return list[i];
	}
	return NULL;
}

void PrimaryMode_EnsurePrimary(void)
{
	if (g_primaryHkl) return;
	HKL hkl = ResolveHklByLang(0x0409);
	if (!hkl)
	{
		HKL list[MAX_PL_LAYOUTS];
		int n = GetKeyboardLayoutList(MAX_PL_LAYOUTS, list);
		if (n > 0) hkl = list[0];
	}
	if (hkl)
	{
		g_primaryHkl = hkl;
		Config_SetPrimaryLang(LOWORD((DWORD_PTR)hkl));
	}
}

BOOL PrimaryMode_IsEnabled(void) { return g_enabled; }
HKL  PrimaryMode_GetPrimary(void) { return g_primaryHkl; }

void PrimaryMode_OnKeyActivity(void)
{
	g_lastActivityTick = GetTickCount();
}

void PrimaryMode_OnLayoutChanged(LANGID lang)
{
	BOOL shouldRun = g_enabled && (g_primaryHkl != NULL)
		&& (lang != LOWORD((DWORD_PTR)g_primaryHkl));

	if (shouldRun)
	{
		g_lastActivityTick = GetTickCount();
		g_returnAttempts = 0;
		if (!g_timerActive)
		{
			SetTimer(g_ownerWnd, PRIMARY_TIMER_ID, 500, NULL);
			g_timerActive = TRUE;
		}
	}
	else
	{
		if (g_timerActive)
		{
			KillTimer(g_ownerWnd, PRIMARY_TIMER_ID);
			g_timerActive = FALSE;
		}
	}
}

void PrimaryMode_OnTimer(void)
{
	UINT secs = Config_GetInactivitySeconds(10);
	if (GetTickCount() - g_lastActivityTick < secs * 1000) return;

	HWND target = Layout_LastAppWindow();
	if (!target || !IsWindow(target)) target = GetForegroundWindow();
	if (target) PostMessageW(target, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)g_primaryHkl);

	if (++g_returnAttempts >= 3)
	{
		KillTimer(g_ownerWnd, PRIMARY_TIMER_ID);
		g_timerActive = FALSE;
	}
}

BOOL PrimaryMode_IsCountingDown(void)
{
	return g_enabled && g_timerActive;
}

float PrimaryMode_RemainingFraction(void)
{
	UINT  secs  = Config_GetInactivitySeconds(10);
	DWORD total = secs * 1000;
	if (total == 0) return 0.0f;
	DWORD elapsed = GetTickCount() - g_lastActivityTick;
	if (elapsed >= total) return 0.0f;
	return (float)(total - elapsed) / (float)total;
}

void PrimaryMode_SetEnabled(BOOL enable)
{
	if (enable) PrimaryMode_EnsurePrimary();
	g_enabled = enable;
	Config_SetPrimaryModeEnabled(enable);
	PrimaryMode_OnLayoutChanged(Layout_Current());
}

void PrimaryMode_SetPrimary(HKL hkl)
{
	g_primaryHkl = hkl;
	Config_SetPrimaryLang(LOWORD((DWORD_PTR)hkl));
	PrimaryMode_OnLayoutChanged(Layout_Current());
}

void PrimaryMode_Init(HWND ownerWnd)
{
	g_ownerWnd = ownerWnd;
	g_enabled = Config_GetPrimaryModeEnabled(FALSE);
	g_primaryHkl = ResolveHklByLang(Config_GetPrimaryLang(0));
	if (g_enabled) PrimaryMode_EnsurePrimary();
	PrimaryMode_OnLayoutChanged(Layout_Current());
}
