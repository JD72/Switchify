#include <Windows.h>
#include <stdio.h>
#include "layout.h"
#include "tray.h"
#include "log.h"
#include "config.h"
#include "autostart.h"
#include "primary_layout.h"
#include "indicator.h"
#include "i18n.h"
#include <commctrl.h>

#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

typedef struct {
	BOOL popup;
} Settings;

void ShowError(LPCSTR message);
DWORD GetOSVersion();
void PressKey(int keyCode);
void ReleaseKey(int keyCode);
void ToggleCapsLockState();
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK AppWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void OnLayoutChanged(LANGID lang);
BOOL CreateAppWindow(void);

HHOOK hHook;
static BOOL g_logToggleBusy = FALSE;

BOOL IsLogToggleBusy(void)
{
	return g_logToggleBusy;
}
BOOL enabled = TRUE;
BOOL keystrokeCapsProcessed = FALSE;
BOOL keystrokeShiftProcessed = FALSE;
BOOL winPressed = FALSE;

#define APP_WINDOW_CLASS "SwitchifyAppWindow"
#define LAYOUT_TIMER_ID  1
#define LAYOUT_POLL_MS   120
#define INDICATOR_PREVIEW_TIMER_ID 3
#define INDICATOR_PREVIEW_MS       1500

HWND hAppWindow = NULL;

Settings settings = {
	.popup = FALSE
};

int main(int argc, char** argv)
{
	Config_Init();
	Log_Init(Config_GetLogEnabled(TRUE));
	I18n_Init();

	BOOL restartMode = (argc > 1 && strcmp(argv[1], "--restart") == 0);
	if (restartMode)
	{
		HANDLE existing = OpenMutexA(SYNCHRONIZE, FALSE, "Switchify");
		int attempts = 0;
		while (existing && attempts < 3)
		{
			CloseHandle(existing);
			Sleep(700);
			existing = OpenMutexA(SYNCHRONIZE, FALSE, "Switchify");
			attempts++;
		}
		if (existing) CloseHandle(existing);
	}

	if (argc > 1 && strcmp(argv[1], "nopopup") == 0)
	{
		settings.popup = FALSE;
	}
	else
	{
		settings.popup = GetOSVersion() >= 10;
	}
#if _DEBUG
	printf("Pop-up is %s\n", settings.popup ? "enabled" : "disabled");
#endif

	HANDLE hMutex = CreateMutex(0, 0, "Switchify");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		ShowError("Another instance of Switchify is already running!");
		return 1;
	}

	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);
	if (hHook == NULL)
	{
		ShowError("Error calling \"SetWindowsHookEx(...)\"");
		return 1;
	}

	Layout_Init(OnLayoutChanged);
	if (!CreateAppWindow())
	{
		return 1;
	}

	MSG messages;
	while (GetMessage(&messages, NULL, 0, 0))
	{
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	UnhookWindowsHookEx(hHook);

	return 0;
}

void ShowError(LPCSTR message)
{
	MessageBox(NULL, message, "Error", MB_OK | MB_ICONERROR);
	Log_Write(LOG_ERROR, message);
}

DWORD GetOSVersion()
{
	HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
	RTL_OSVERSIONINFOW osvi = { 0 };

	if (hMod)
	{
		RtlGetVersionPtr p = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");

		if (p)
		{
			osvi.dwOSVersionInfoSize = sizeof(osvi);
			p(&osvi);
		}
	}

	return osvi.dwMajorVersion;
}

void PressKey(int keyCode)
{
	keybd_event(keyCode, 0, 0, 0);
}

void ReleaseKey(int keyCode)
{
	keybd_event(keyCode, 0, KEYEVENTF_KEYUP, 0);
}

void ToggleCapsLockState()
{
	PressKey(VK_CAPITAL);
	ReleaseKey(VK_CAPITAL);
#if _DEBUG
	printf("Caps Lock state has been toggled\n");
#endif
}

void SwitchLayout(void)
{
	if (settings.popup)
	{
		PressKey(VK_LWIN);
		PressKey(VK_SPACE);
		ReleaseKey(VK_SPACE);
		ReleaseKey(VK_LWIN);
	}
	else
	{
		PressKey(VK_MENU);
		PressKey(VK_LSHIFT);
		ReleaseKey(VK_MENU);
		ReleaseKey(VK_LSHIFT);
	}
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT* key = (KBDLLHOOKSTRUCT*)lParam;

	if (nCode == HC_ACTION && !(key->flags & LLKHF_INJECTED))
	{
#if _DEBUG
		const char* keyStatus = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) ? "pressed" : "released";
		printf("Key %d has been %s\n", key->vkCode, keyStatus);
#endif

		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
			PrimaryMode_OnKeyActivity();

		if (key->vkCode == VK_CAPITAL)
		{
			if ((wParam == WM_SYSKEYDOWN || wParam == WM_SYSKEYUP) && enabled)
			{
				return 1;
			}

			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
			{
				keystrokeCapsProcessed = FALSE;

				if (winPressed)
				{
					winPressed = FALSE;
					ReleaseKey(VK_LWIN);
				}

				if (enabled && !settings.popup)
				{
					if (!keystrokeShiftProcessed)
					{
						PressKey(VK_MENU);
						PressKey(VK_LSHIFT);
						ReleaseKey(VK_MENU);
						ReleaseKey(VK_LSHIFT);
					}
					else
					{
						keystrokeShiftProcessed = FALSE;
					}
				}
			}

			if (!enabled)
			{
				return CallNextHookEx(hHook, nCode, wParam, lParam);
			}

			if (wParam == WM_KEYDOWN && !keystrokeCapsProcessed)
			{
				keystrokeCapsProcessed = TRUE;

				if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
				{
					ToggleCapsLockState();
					return 1;
				}
				else
				{
					if (settings.popup)
					{
						PressKey(VK_LWIN);
						PressKey(VK_SPACE);
						ReleaseKey(VK_SPACE);
						winPressed = TRUE;
					}
				}
			}
			return 1;
		}

		else if (key->vkCode == VK_LSHIFT)
		{
			if ((wParam == WM_KEYUP || wParam == WM_SYSKEYUP) && !keystrokeCapsProcessed)
			{
				keystrokeShiftProcessed = FALSE;
			}

			if (!enabled)
			{
				return CallNextHookEx(hHook, nCode, wParam, lParam);
			}

			if (wParam == WM_KEYDOWN && !keystrokeShiftProcessed)
			{
				keystrokeShiftProcessed = TRUE;

				if (keystrokeCapsProcessed == TRUE)
				{
					ToggleCapsLockState();
					if (settings.popup)
					{
						PressKey(VK_LWIN);
						PressKey(VK_SPACE);
						ReleaseKey(VK_SPACE);
						winPressed = TRUE;
					}

					return 0;
				}
			}
			return 0;
		}
	}

	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

void OnLayoutChanged(LANGID lang)
{
#if _DEBUG
	printf("Layout -> %04x\n", lang);
#endif
	Tray_SetLanguage(lang);
	PrimaryMode_OnLayoutChanged(lang);
	Indicator_OnLayoutChanged(lang);
}

static void ApplyIndicatorAppearanceChange(void)
{
	Indicator_RefreshAppearance();
	Indicator_PreviewShow();
	SetTimer(hAppWindow, INDICATOR_PREVIEW_TIMER_ID, INDICATOR_PREVIEW_MS, NULL);
}

static HRESULT CALLBACK AboutTaskDlgCallback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LONG_PTR refData)
{
	(void)hwnd; (void)wParam; (void)refData;
	if (msg == TDN_HYPERLINK_CLICKED)
		ShellExecuteW(NULL, L"open", (LPCWSTR)lParam, NULL, NULL, SW_SHOWNORMAL);
	return S_OK;
}

LRESULT CALLBACK AppWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_TIMER:
		if (wParam == LAYOUT_TIMER_ID)
		{
			Layout_Poll();
		}
		else if (wParam == PRIMARY_TIMER_ID)
		{
			PrimaryMode_OnTimer();
			if (PrimaryMode_IsCountingDown() && Config_GetCountdownEnabled(FALSE))
				Indicator_SetCountdown(PrimaryMode_RemainingFraction());
		}
		else if (wParam == INDICATOR_PREVIEW_TIMER_ID)
		{
			KillTimer(hWnd, INDICATOR_PREVIEW_TIMER_ID);
			Indicator_OnLayoutChanged(Layout_Current());
		}
		return 0;
	case TRAY_CALLBACK_MSG:
		Tray_HandleCallback(wParam, lParam);
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_TOGGLE:
			enabled = !enabled;
			return 0;
		case IDM_AUTOSTART:
			Autostart_SetEnabled(!Autostart_IsEnabled());
			return 0;
		case IDM_ABOUT:
		{
			TASKDIALOGCONFIG cfg = { sizeof(cfg) };
			cfg.hwndParent      = hWnd;
			cfg.dwFlags         = TDF_ENABLE_HYPERLINKS | TDF_ALLOW_DIALOG_CANCELLATION;
			cfg.pszWindowTitle  = I18n_Get(STR_ABOUT_TITLE);
			cfg.pszMainIcon     = TD_INFORMATION_ICON;
			cfg.pszContent      = I18n_Get(STR_ABOUT_BODY);
			cfg.dwCommonButtons = TDCBF_OK_BUTTON;
			cfg.pfCallback      = AboutTaskDlgCallback;
			TaskDialogIndirect(&cfg, NULL, NULL, NULL);
			return 0;
		}
		case IDM_LOG:
			if (g_logToggleBusy) return 0;
			g_logToggleBusy = TRUE;

			Log_SetEnabled(!Log_IsEnabled());
			Config_SetLogEnabled(Log_IsEnabled());

			if (MessageBoxW(hWnd,
					I18n_Get(STR_LOG_RESTART_BODY),
					I18n_Get(STR_DLG_TITLE),
					MB_YESNO | MB_ICONQUESTION) == IDYES)
			{
				char exePath[MAX_PATH] = { 0 };
				GetModuleFileNameA(NULL, exePath, MAX_PATH);
				char cmdLine[MAX_PATH + 32] = { 0 };
				snprintf(cmdLine, sizeof(cmdLine), "\"%s\" --restart", exePath);

				STARTUPINFOA si = { sizeof(si) };
				PROCESS_INFORMATION pi = { 0 };
				if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
				{
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
					DestroyWindow(hWnd);
					return 0;
				}
				ShowError("Error calling \"CreateProcess(...)\" for restart");
			}

			g_logToggleBusy = FALSE;
			return 0;
		case IDM_PRIMARY_MODE:
			PrimaryMode_SetEnabled(!PrimaryMode_IsEnabled());
			return 0;
		case IDM_INDICATOR:
			Indicator_SetEnabled(!Indicator_IsEnabled());
			return 0;
		case IDM_COUNTDOWN:
			Config_SetCountdownEnabled(!Config_GetCountdownEnabled(FALSE));
			return 0;
		case IDM_IND_COLOR_CUSTOM:
		{
			static COLORREF customColors[16] = { 0 };
			CHOOSECOLORW cc = { 0 };
			cc.lStructSize = sizeof(cc);
			cc.hwndOwner = hWnd;
			cc.lpCustColors = customColors;
			cc.rgbResult = Config_GetIndicatorColor(
				RGB((IND_DEF_COLOR >> 16) & 0xFF, (IND_DEF_COLOR >> 8) & 0xFF, IND_DEF_COLOR & 0xFF));
			cc.Flags = CC_RGBINIT | CC_FULLOPEN;
			if (ChooseColorW(&cc))
			{
				Config_SetIndicatorColor(cc.rgbResult);
				ApplyIndicatorAppearanceChange();
			}
			return 0;
		}
		case IDM_EXIT:
			DestroyWindow(hWnd);
			return 0;
		default:
			if (LOWORD(wParam) >= IDM_PRIMARY_BASE &&
				LOWORD(wParam) < IDM_PRIMARY_BASE + MAX_LAYOUTS)
			{
				HKL hkl = Tray_GetMenuLayout(LOWORD(wParam) - IDM_PRIMARY_BASE);
				if (hkl)
				{
					PrimaryMode_SetPrimary(hkl);
					Indicator_OnLayoutChanged(Layout_Current());
				}
			}
			else if (LOWORD(wParam) >= IDM_TIMER_BASE &&
				LOWORD(wParam) < IDM_TIMER_BASE + TIMER_PRESET_COUNT)
			{
				UINT secs = Tray_GetTimerPreset(LOWORD(wParam) - IDM_TIMER_BASE);
				if (secs) Config_SetInactivitySeconds(secs);
			}
			else if (LOWORD(wParam) >= IDM_IND_COLOR_BASE &&
				LOWORD(wParam) < IDM_IND_COLOR_BASE + IND_COLOR_PRESET_COUNT)
			{
				Config_SetIndicatorColor(Tray_GetIndicatorColorPreset(LOWORD(wParam) - IDM_IND_COLOR_BASE));
				ApplyIndicatorAppearanceChange();
			}
			else if (LOWORD(wParam) >= IDM_IND_THICK_BASE &&
				LOWORD(wParam) < IDM_IND_THICK_BASE + IND_THICK_PRESET_COUNT)
			{
				UINT px = Tray_GetIndicatorThicknessPreset(LOWORD(wParam) - IDM_IND_THICK_BASE);
				if (px) { Config_SetIndicatorThickness(px); ApplyIndicatorAppearanceChange(); }
			}
			else if (LOWORD(wParam) >= IDM_IND_LENGTH_BASE &&
				LOWORD(wParam) < IDM_IND_LENGTH_BASE + IND_LENGTH_PRESET_COUNT)
			{
				UINT pct = Tray_GetIndicatorLengthPreset(LOWORD(wParam) - IDM_IND_LENGTH_BASE);
				if (pct) { Config_SetIndicatorLength(pct); ApplyIndicatorAppearanceChange(); }
			}
			else if (LOWORD(wParam) >= IDM_IND_EDGE_BASE &&
				LOWORD(wParam) < IDM_IND_EDGE_BASE + 4)
			{
				Config_SetIndicatorEdge((IndicatorEdge)(LOWORD(wParam) - IDM_IND_EDGE_BASE));
				ApplyIndicatorAppearanceChange();
			}
			else if (LOWORD(wParam) >= IDM_IND_ALIGN_BASE &&
				LOWORD(wParam) < IDM_IND_ALIGN_BASE + 3)
			{
				Config_SetIndicatorAlign((IndicatorAlign)(LOWORD(wParam) - IDM_IND_ALIGN_BASE));
				ApplyIndicatorAppearanceChange();
			}
			else if (LOWORD(wParam) >= IDM_LANG_BASE &&
				LOWORD(wParam) < IDM_LANG_BASE + LANG_COUNT)
			{
				const char* code = Tray_GetLangCode(LOWORD(wParam) - IDM_LANG_BASE);
				if (code) I18n_SetLanguage(code);
			}
			return 0;
		}
		return 0;
	case WM_DESTROY:
		Log_Write(LOG_INFO, "Switchify stopping");
		Tray_Shutdown();
		KillTimer(hWnd, LAYOUT_TIMER_ID);
		KillTimer(hWnd, PRIMARY_TIMER_ID);
		KillTimer(hWnd, INDICATOR_PREVIEW_TIMER_ID);
		Indicator_Shutdown();
		Log_Shutdown();
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
}

BOOL CreateAppWindow(void)
{
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = AppWindowProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = APP_WINDOW_CLASS;

	if (!RegisterClass(&wc))
	{
		ShowError("Error calling \"RegisterClass(...)\"");
		return FALSE;
	}

	hAppWindow = CreateWindowEx(0, APP_WINDOW_CLASS, "Switchify", 0,
		0, 0, 0, 0, HWND_MESSAGE, NULL, wc.hInstance, NULL);
	if (hAppWindow == NULL)
	{
		ShowError("Error calling \"CreateWindowEx(...)\"");
		return FALSE;
	}

	SetTimer(hAppWindow, LAYOUT_TIMER_ID, LAYOUT_POLL_MS, NULL);

	if (!Tray_Init(hAppWindow, wc.hInstance))
	{
		return FALSE;
	}
	Tray_SetLanguage(Layout_Current());
	PrimaryMode_Init(hAppWindow);
	Indicator_Init();
	Log_Write(LOG_INFO, "Switchify started");

	return TRUE;
}