#include "tray.h"
#include "log.h"
#include "autostart.h"
#include "primary_layout.h"
#include "config.h"
#include "indicator.h"
#include "i18n.h"
#include <string.h>

static const UINT g_timerPresets[TIMER_PRESET_COUNT] = { 3, 5, 10, 15, 30, 60 };

static const COLORREF g_indColorPresets[IND_COLOR_PRESET_COUNT] = {
	RGB(0xFF, 0x80, 0x00),
	RGB(0xE0, 0x00, 0x00),
	RGB(0xFF, 0xC8, 0x00),
	RGB(0x00, 0xB0, 0x00),
	RGB(0x00, 0x90, 0xFF),
	RGB(0xD0, 0x00, 0xD0),
};
static const StrId g_indColorStr[IND_COLOR_PRESET_COUNT] = {
	STR_COLOR_ORANGE, STR_COLOR_RED, STR_COLOR_YELLOW,
	STR_COLOR_GREEN, STR_COLOR_BLUE, STR_COLOR_MAGENTA
};
static const UINT g_indThickPresets[IND_THICK_PRESET_COUNT]  = { 3, 5, 8, 12, 20 };
static const UINT g_indLengthPresets[IND_LENGTH_PRESET_COUNT] = { 25, 50, 75, 100 };
static const StrId g_indEdgeStr[4]  = { STR_EDGE_TOP, STR_EDGE_BOTTOM, STR_EDGE_LEFT, STR_EDGE_RIGHT };
static const StrId g_indAlignStr[3] = { STR_ALIGN_START, STR_ALIGN_CENTER, STR_ALIGN_END };

UINT Tray_GetTimerPreset(int index)
{
	if (index < 0 || index >= TIMER_PRESET_COUNT) return 0;
	return g_timerPresets[index];
}

COLORREF Tray_GetIndicatorColorPreset(int index)
{
	if (index < 0 || index >= IND_COLOR_PRESET_COUNT) return g_indColorPresets[0];
	return g_indColorPresets[index];
}

UINT Tray_GetIndicatorThicknessPreset(int index)
{
	if (index < 0 || index >= IND_THICK_PRESET_COUNT) return 0;
	return g_indThickPresets[index];
}

UINT Tray_GetIndicatorLengthPreset(int index)
{
	if (index < 0 || index >= IND_LENGTH_PRESET_COUNT) return 0;
	return g_indLengthPresets[index];
}

static const char* const g_langCodes[LANG_COUNT] = { "en", "ru" };

const char* Tray_GetLangCode(int index)
{
	if (index < 0 || index >= LANG_COUNT) return NULL;
	return g_langCodes[index];
}

extern void SwitchLayout(void);
extern BOOL IsLogToggleBusy(void);

static HKL g_menuLayouts[MAX_LAYOUTS];
static int g_menuLayoutCount = 0;

extern BOOL enabled;

#define TRAY_ICON_ID 1

static NOTIFYICONDATA g_nid = { 0 };
static HICON          g_currentIcon = NULL;
static HWND           g_ownerWnd = NULL;
static HINSTANCE      g_hInst = NULL;

static const char* LangCode(LANGID lang)
{
	WORD primary = PRIMARYLANGID(lang);
	switch (primary)
	{
	case LANG_ENGLISH:   return "EN";
	case LANG_RUSSIAN:   return "RU";
	case LANG_UKRAINIAN: return "UA";
	case LANG_GERMAN:    return "DE";
	case LANG_FRENCH:    return "FR";
	case LANG_SPANISH:   return "ES";
	case LANG_ITALIAN:   return "IT";
	case LANG_POLISH:    return "PL";
	default:             return "??";
	}
}

static HICON RenderLangIcon(const char* code2)
{
	const int W = 16, H = 16;

	HDC screenDC = GetDC(NULL);
	if (!screenDC) return NULL;

	BITMAPINFO bi = { 0 };
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = W;
	bi.bmiHeader.biHeight = H;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;

	void* bits = NULL;
	HBITMAP colorBmp = CreateDIBSection(screenDC, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
	HBITMAP maskBmp = CreateBitmap(W, H, 1, 1, NULL);
	HDC dc = CreateCompatibleDC(screenDC);
	ReleaseDC(NULL, screenDC);

	if (!colorBmp || !maskBmp || !dc || !bits)
	{
		if (colorBmp) DeleteObject(colorBmp);
		if (maskBmp) DeleteObject(maskBmp);
		if (dc) DeleteDC(dc);
		return NULL;
	}

	DWORD* px = (DWORD*)bits;
	for (int i = 0; i < W * H; i++) px[i] = 0x00FFFFFFu;

	HBITMAP oldBmp = (HBITMAP)SelectObject(dc, colorBmp);

	LOGFONTA lf = { 0 };
	lf.lfHeight = -11;
	lf.lfWeight = FW_BOLD;
	lf.lfQuality = ANTIALIASED_QUALITY;
	strcpy_s(lf.lfFaceName, LF_FACESIZE, "Segoe UI");
	HFONT font = CreateFontIndirectA(&lf);
	HFONT oldFont = (HFONT)SelectObject(dc, font);

	SetBkMode(dc, TRANSPARENT);
	SetTextColor(dc, RGB(0, 0, 0));
	RECT r = { 0, 0, W, H };
	DrawTextA(dc, code2, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	SelectObject(dc, oldFont);
	SelectObject(dc, oldBmp);
	DeleteObject(font);
	DeleteDC(dc);

	for (int i = 0; i < W * H; i++)
	{
		DWORD c = px[i];
		BYTE b = (BYTE)(c & 0xFF);
		BYTE g = (BYTE)((c >> 8) & 0xFF);
		BYTE r2 = (BYTE)((c >> 16) & 0xFF);
		BYTE gray = (BYTE)(((int)b + g + r2) / 3);
		BYTE alpha = (BYTE)(255 - gray);
		px[i] = ((DWORD)alpha << 24);
	}

	ICONINFO ii = { 0 };
	ii.fIcon = TRUE;
	ii.hbmColor = colorBmp;
	ii.hbmMask = maskBmp;
	HICON icon = CreateIconIndirect(&ii);

	DeleteObject(colorBmp);
	DeleteObject(maskBmp);
	return icon;
}

BOOL Tray_Init(HWND ownerWnd, HINSTANCE hInst)
{
	g_ownerWnd = ownerWnd;
	g_hInst = hInst;

	g_nid.cbSize = sizeof(NOTIFYICONDATA);
	g_nid.hWnd = ownerWnd;
	g_nid.uID = TRAY_ICON_ID;
	g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	g_nid.uCallbackMessage = TRAY_CALLBACK_MSG;
	g_currentIcon = RenderLangIcon("??");
	g_nid.hIcon = g_currentIcon;
	strcpy_s(g_nid.szTip, sizeof(g_nid.szTip), "Switchify");

	if (!Shell_NotifyIcon(NIM_ADD, &g_nid))
	{
		MessageBox(NULL, "Error calling \"Shell_NotifyIcon(NIM_ADD)\"", "Error", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	return TRUE;
}

void Tray_SetLanguage(LANGID lang)
{
	HICON newIcon = RenderLangIcon(LangCode(lang));
	if (!newIcon) return;

	g_nid.hIcon = newIcon;
	g_nid.uFlags = NIF_ICON;
	Shell_NotifyIcon(NIM_MODIFY, &g_nid);

	if (g_currentIcon) DestroyIcon(g_currentIcon);
	g_currentIcon = newIcon;
}

static HMENU BuildIndicatorSubmenu(BOOL on)
{
	HMENU root = CreatePopupMenu();

	AppendMenuW(root, MF_STRING | (on ? MF_CHECKED : 0), IDM_INDICATOR, I18n_Get(STR_IND_ENABLED));
	AppendMenuW(root, MF_SEPARATOR, 0, NULL);

	UINT grayIfOff = on ? 0 : MF_GRAYED;

	HMENU colorMenu = CreatePopupMenu();
	BOOL rainbowOn = Config_GetIndicatorRainbow(FALSE);
	COLORREF curColor = Config_GetIndicatorColor(
		RGB((IND_DEF_COLOR >> 16) & 0xFF, (IND_DEF_COLOR >> 8) & 0xFF, IND_DEF_COLOR & 0xFF));
	for (int i = 0; i < IND_COLOR_PRESET_COUNT; i++)
	{
		UINT f = MF_STRING | ((!rainbowOn && g_indColorPresets[i] == curColor) ? MF_CHECKED : 0);
		AppendMenuW(colorMenu, f, IDM_IND_COLOR_BASE + i, I18n_Get(g_indColorStr[i]));
	}
	AppendMenuW(colorMenu, MF_SEPARATOR, 0, NULL);
	AppendMenuW(colorMenu, MF_STRING, IDM_IND_COLOR_CUSTOM, I18n_Get(STR_IND_CUSTOM));
	AppendMenuW(colorMenu, MF_STRING | (rainbowOn ? MF_CHECKED : 0), IDM_IND_COLOR_RAINBOW, I18n_Get(STR_COLOR_RAINBOW));
	AppendMenuW(root, MF_POPUP | grayIfOff, (UINT_PTR)colorMenu, I18n_Get(STR_IND_COLOR));

	HMENU thickMenu = CreatePopupMenu();
	UINT curThick = Config_GetIndicatorThickness(IND_DEF_THICKNESS);
	for (int i = 0; i < IND_THICK_PRESET_COUNT; i++)
	{
		wchar_t item[16];
		wsprintfW(item, L"%u px", g_indThickPresets[i]);
		UINT f = MF_STRING | ((g_indThickPresets[i] == curThick) ? MF_CHECKED : 0);
		AppendMenuW(thickMenu, f, IDM_IND_THICK_BASE + i, item);
	}
	AppendMenuW(root, MF_POPUP | grayIfOff, (UINT_PTR)thickMenu, I18n_Get(STR_IND_THICKNESS));

	HMENU lenMenu = CreatePopupMenu();
	UINT curLen = Config_GetIndicatorLength(IND_DEF_LENGTH);
	for (int i = 0; i < IND_LENGTH_PRESET_COUNT; i++)
	{
		wchar_t item[16];
		wsprintfW(item, L"%u %%", g_indLengthPresets[i]);
		UINT f = MF_STRING | ((g_indLengthPresets[i] == curLen) ? MF_CHECKED : 0);
		AppendMenuW(lenMenu, f, IDM_IND_LENGTH_BASE + i, item);
	}
	AppendMenuW(root, MF_POPUP | grayIfOff, (UINT_PTR)lenMenu, I18n_Get(STR_IND_LENGTH));

	HMENU edgeMenu = CreatePopupMenu();
	UINT edgeMask = Config_GetIndicatorEdgeMask(IND_DEF_EDGE_MASK);
	for (int i = 0; i < 4; i++)
	{
		UINT f = MF_STRING | ((edgeMask & (1u << i)) ? MF_CHECKED : 0);
		AppendMenuW(edgeMenu, f, IDM_IND_EDGE_BASE + i, I18n_Get(g_indEdgeStr[i]));
	}
	AppendMenuW(root, MF_POPUP | grayIfOff, (UINT_PTR)edgeMenu, I18n_Get(STR_IND_EDGE));

	HMENU alignMenu = CreatePopupMenu();
	IndicatorAlign curAlign = Config_GetIndicatorAlign(IND_DEF_ALIGN);
	for (int i = 0; i < 3; i++)
	{
		UINT f = MF_STRING | (((IndicatorAlign)i == curAlign) ? MF_CHECKED : 0);
		AppendMenuW(alignMenu, f, IDM_IND_ALIGN_BASE + i, I18n_Get(g_indAlignStr[i]));
	}
	AppendMenuW(root, MF_POPUP | grayIfOff, (UINT_PTR)alignMenu, I18n_Get(STR_IND_ALIGN));

	return root;
}

static void ShowContextMenu(POINT pt)
{
	HMENU menu = CreatePopupMenu();
	if (!menu) return;

	AppendMenuW(menu, MF_STRING, IDM_TOGGLE, I18n_Get(enabled ? STR_MENU_DISABLE : STR_MENU_ENABLE));
	AppendMenuW(menu, MF_SEPARATOR, 0, NULL);

	UINT logFlags = MF_STRING | (Log_IsEnabled() ? MF_CHECKED : 0);
	if (IsLogToggleBusy()) logFlags |= MF_GRAYED;
	AppendMenuW(menu, logFlags, IDM_LOG, I18n_Get(STR_MENU_LOG));
	AppendMenuW(menu, MF_STRING | (Autostart_IsEnabled() ? MF_CHECKED : 0),
		IDM_AUTOSTART, I18n_Get(STR_MENU_AUTOSTART));

	HMENU langSub = CreatePopupMenu();
	BOOL isEn = strcmp(I18n_CurrentLang(), "en") == 0;
	AppendMenuW(langSub, MF_STRING | (isEn ? MF_CHECKED : 0),  IDM_LANG_BASE + 0, L"English");
	AppendMenuW(langSub, MF_STRING | (!isEn ? MF_CHECKED : 0), IDM_LANG_BASE + 1, L"Русский");
	AppendMenuW(menu, MF_POPUP, (UINT_PTR)langSub, I18n_Get(STR_MENU_LANGUAGE));

	AppendMenuW(menu, MF_SEPARATOR, 0, NULL);

	BOOL modeOn = PrimaryMode_IsEnabled();
	AppendMenuW(menu, MF_STRING | (modeOn ? MF_CHECKED : 0),
		IDM_PRIMARY_MODE, I18n_Get(STR_MENU_PRIMARY_MODE));

	HMENU sub = CreatePopupMenu();
	g_menuLayoutCount = GetKeyboardLayoutList(MAX_LAYOUTS, g_menuLayouts);
	HKL primary = PrimaryMode_GetPrimary();
	for (int i = 0; i < g_menuLayoutCount; i++)
	{
		LANGID lang = LOWORD((DWORD_PTR)g_menuLayouts[i]);
		wchar_t name[8] = { 0 };
		MultiByteToWideChar(CP_ACP, 0, LangCode(lang), -1, name, 8);
		UINT f = MF_STRING | ((g_menuLayouts[i] == primary) ? MF_CHECKED : 0);
		AppendMenuW(sub, f, IDM_PRIMARY_BASE + i, name);
	}
	AppendMenuW(menu, MF_POPUP | (modeOn ? 0 : MF_GRAYED), (UINT_PTR)sub, I18n_Get(STR_MENU_PRIMARY));

	HMENU timerSub = CreatePopupMenu();
	UINT curSecs = Config_GetInactivitySeconds(10);
	for (int i = 0; i < TIMER_PRESET_COUNT; i++)
	{
		wchar_t item[24];
		wsprintfW(item, I18n_Get(STR_TIMER_SEC_FMT), g_timerPresets[i]);
		UINT f = MF_STRING | ((g_timerPresets[i] == curSecs) ? MF_CHECKED : 0);
		AppendMenuW(timerSub, f, IDM_TIMER_BASE + i, item);
	}
	wchar_t timerLabel[48];
	wsprintfW(timerLabel, I18n_Get(STR_MENU_TIMER_FMT), curSecs);
	AppendMenuW(menu, MF_POPUP | (modeOn ? 0 : MF_GRAYED), (UINT_PTR)timerSub, timerLabel);

	UINT cdFlags = MF_STRING | (Config_GetCountdownEnabled(FALSE) ? MF_CHECKED : 0)
		| ((modeOn && Indicator_IsEnabled()) ? 0 : MF_GRAYED);
	AppendMenuW(menu, cdFlags, IDM_COUNTDOWN, I18n_Get(STR_MENU_COUNTDOWN));
	AppendMenuW(menu, MF_SEPARATOR, 0, NULL);

	HMENU indMenu = BuildIndicatorSubmenu(Indicator_IsEnabled());
	AppendMenuW(menu, MF_POPUP, (UINT_PTR)indMenu, I18n_Get(STR_MENU_INDICATOR));
	AppendMenuW(menu, MF_SEPARATOR, 0, NULL);

	AppendMenuW(menu, MF_STRING, IDM_ABOUT, I18n_Get(STR_MENU_ABOUT));
	AppendMenuW(menu, MF_STRING, IDM_EXIT, I18n_Get(STR_MENU_EXIT));

	SetForegroundWindow(g_ownerWnd);
	TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_ownerWnd, NULL);
	DestroyMenu(menu);
}

void Tray_HandleCallback(WPARAM wParam, LPARAM lParam)
{
	(void)wParam;
	if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU)
	{
		POINT pt;
		GetCursorPos(&pt);
		ShowContextMenu(pt);
	}
	else if (lParam == WM_LBUTTONDBLCLK)
	{
		SwitchLayout();
	}
}

void Tray_Shutdown(void)
{
	Shell_NotifyIcon(NIM_DELETE, &g_nid);
	if (g_currentIcon)
	{
		DestroyIcon(g_currentIcon);
		g_currentIcon = NULL;
	}
}

HKL Tray_GetMenuLayout(int index)
{
	if (index < 0 || index >= g_menuLayoutCount) return NULL;
	return g_menuLayouts[index];
}
