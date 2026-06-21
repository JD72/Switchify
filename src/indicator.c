#include "indicator.h"
#include "config.h"
#include "layout.h"
#include "primary_layout.h"
#include <math.h>
#include <stdlib.h>

#define INDICATOR_CLASS  L"SwitchifyIndicator"
#define IND_DEF_COLORREF RGB((IND_DEF_COLOR >> 16) & 0xFF, (IND_DEF_COLOR >> 8) & 0xFF, IND_DEF_COLOR & 0xFF)

static HWND   g_hwnd[4] = { 0 };
static HBRUSH g_brush = NULL;
static BOOL   g_enabled = FALSE;
static BOOL   g_shown = FALSE;
static HWND   g_owner = NULL;
static BOOL   g_rainbow = FALSE;
static double g_rainbowHue = 0.0;

static UINT           g_edgeMask = IND_DEF_EDGE_MASK;
static IndicatorAlign g_align    = IND_ALIGN_CENTER;
static int g_th     = 1;
static int g_lenPct = 100;
static int g_scrW   = 0;
static int g_scrH   = 0;

static void ComputeRect(IndicatorEdge edge, float frac, RECT* out)
{
	if (frac < 0.0f) frac = 0.0f;
	if (frac > 1.0f) frac = 1.0f;
	BOOL vertical = (edge == IND_EDGE_LEFT || edge == IND_EDGE_RIGHT);
	int longAxis = vertical ? g_scrH : g_scrW;

	int full = (int)(((long long)longAxis * (long long)g_lenPct) / 100);
	if (full < 1) full = 1;
	if (full > longAxis) full = longAxis;
	int len = (int)(full * frac);
	if (len < 1) len = 1;

	int off = 0;
	if (g_align == IND_ALIGN_CENTER)   off = (longAxis - len) / 2;
	else if (g_align == IND_ALIGN_END) off = longAxis - len;
	if (off < 0) off = 0;

	switch (edge)
	{
	case IND_EDGE_TOP:    out->left = off;           out->top = 0;             out->right = off + len; out->bottom = g_th;      break;
	case IND_EDGE_LEFT:   out->left = 0;             out->top = off;           out->right = g_th;      out->bottom = off + len; break;
	case IND_EDGE_RIGHT:  out->left = g_scrW - g_th; out->top = off;           out->right = g_scrW;    out->bottom = off + len; break;
	case IND_EDGE_BOTTOM:
	default:              out->left = off;           out->top = g_scrH - g_th; out->right = off + len; out->bottom = g_scrH;    break;
	}
}

static void ApplyConfigGeometry(void)
{
	UINT thickness = Config_GetIndicatorThickness(IND_DEF_THICKNESS);
	UINT lengthPct = Config_GetIndicatorLength(IND_DEF_LENGTH);
	g_align    = Config_GetIndicatorAlign(IND_DEF_ALIGN);
	g_edgeMask = Config_GetIndicatorEdgeMask(IND_DEF_EDGE_MASK);

	int W = GetSystemMetrics(SM_CXSCREEN);
	int H = GetSystemMetrics(SM_CYSCREEN);
	int shortMax = (W < H) ? W : H;

	int th = (int)thickness;
	if (th < 1) th = 1;
	if (th > shortMax) th = shortMax;

	g_th     = th;
	g_lenPct = (int)lengthPct;
	g_scrW   = W;
	g_scrH   = H;
}

static HWND CreateEdgeWindow(IndicatorEdge edge)
{
	RECT rc; ComputeRect(edge, 1.0f, &rc);
	HWND h = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
		INDICATOR_CLASS, L"", WS_POPUP,
		rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		NULL, NULL, GetModuleHandleW(NULL), NULL);
	if (h) SetLayeredWindowAttributes(h, 0, 255, LWA_ALPHA);
	return h;
}

static void SyncEdgeWindows(void)
{
	for (int e = 0; e < 4; e++)
	{
		BOOL want = (g_edgeMask & (1u << e)) != 0;
		if (want && !g_hwnd[e])       g_hwnd[e] = CreateEdgeWindow((IndicatorEdge)e);
		else if (!want && g_hwnd[e]) { DestroyWindow(g_hwnd[e]); g_hwnd[e] = NULL; }
	}
}

static COLORREF HsvToColorref(double h)
{
	double c = 1.0;
	double hp = h / 60.0;
	double x = c * (1.0 - fabs(fmod(hp, 2.0) - 1.0));
	double r = 0, g = 0, b = 0;
	if      (hp < 1) { r = c; g = x; }
	else if (hp < 2) { r = x; g = c; }
	else if (hp < 3) { g = c; b = x; }
	else if (hp < 4) { g = x; b = c; }
	else if (hp < 5) { r = x; b = c; }
	else             { r = c; b = x; }
	return RGB((BYTE)(r * 255.0 + 0.5), (BYTE)(g * 255.0 + 0.5), (BYTE)(b * 255.0 + 0.5));
}

static void ApplyBrushColor(COLORREF col)
{
	HBRUSH nb = CreateSolidBrush(col);
	if (!nb) return;
	if (g_brush) DeleteObject(g_brush);
	g_brush = nb;
	BOOL classSet = FALSE;
	for (int e = 0; e < 4; e++)
	{
		if (!g_hwnd[e]) continue;
		if (!classSet) { SetClassLongPtrW(g_hwnd[e], GCLP_HBRBACKGROUND, (LONG_PTR)g_brush); classSet = TRUE; }
		InvalidateRect(g_hwnd[e], NULL, TRUE);
	}
}

static void UpdateRainbowTimer(void)
{
	if (!g_owner) return;
	if (g_shown && g_rainbow) SetTimer(g_owner, RAINBOW_TIMER_ID, 1000, NULL);
	else                      KillTimer(g_owner, RAINBOW_TIMER_ID);
}

static LRESULT CALLBACK IndicatorProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_PAINT)
	{
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(hWnd, &ps);
		RECT rc;
		GetClientRect(hWnd, &rc);
		FillRect(dc, &rc, g_brush);
		EndPaint(hWnd, &ps);
		return 0;
	}
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static void ShowStrip(void)
{
	if (g_shown) return;
	for (int e = 0; e < 4; e++)
	{
		if (!g_hwnd[e]) continue;
		RECT rc; ComputeRect((IndicatorEdge)e, 1.0f, &rc);
		SetWindowPos(g_hwnd[e], NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
			SWP_NOZORDER | SWP_NOACTIVATE);
		ShowWindow(g_hwnd[e], SW_SHOWNA);
	}
	g_shown = TRUE;
	if (g_rainbow)
	{
		g_rainbowHue = (double)(rand() % 360);
		ApplyBrushColor(HsvToColorref(g_rainbowHue));
	}
	UpdateRainbowTimer();
}

static void HideStrip(void)
{
	if (!g_shown) return;
	for (int e = 0; e < 4; e++)
		if (g_hwnd[e]) ShowWindow(g_hwnd[e], SW_HIDE);
	g_shown = FALSE;
	if (g_owner) KillTimer(g_owner, RAINBOW_TIMER_ID);
}

BOOL Indicator_IsEnabled(void) { return g_enabled; }

void Indicator_OnLayoutChanged(LANGID lang)
{
	if (!g_enabled) return;
	HKL p = PrimaryMode_GetPrimary();
	if (!p) return;
	if (lang == LOWORD((DWORD_PTR)p)) HideStrip();
	else                              ShowStrip();
}

void Indicator_SetEnabled(BOOL enable)
{
	if (enable)
	{
		PrimaryMode_EnsurePrimary();
		Config_EnsureIndicatorAppearance();
		g_rainbow = Config_GetIndicatorRainbow(FALSE);
		g_enabled = TRUE;
		Config_SetIndicatorEnabled(TRUE);
		Indicator_OnLayoutChanged(Layout_Current());
	}
	else
	{
		HideStrip();
		g_enabled = FALSE;
		Config_SetIndicatorEnabled(FALSE);
	}
}

void Indicator_Init(HWND owner)
{
	g_owner = owner;
	srand((unsigned)GetTickCount());

	g_enabled = Config_GetIndicatorEnabled(FALSE);
	if (g_enabled)
		Config_EnsureIndicatorAppearance();

	g_rainbow = Config_GetIndicatorRainbow(FALSE);
	if (g_rainbow) g_rainbowHue = (double)(rand() % 360);
	g_brush = CreateSolidBrush(g_rainbow ? HsvToColorref(g_rainbowHue)
	                                     : Config_GetIndicatorColor(IND_DEF_COLORREF));

	ApplyConfigGeometry();

	WNDCLASSW wc = { 0 };
	wc.lpfnWndProc = IndicatorProc;
	wc.hInstance = GetModuleHandleW(NULL);
	wc.lpszClassName = INDICATOR_CLASS;
	wc.hbrBackground = g_brush;
	RegisterClassW(&wc);

	SyncEdgeWindows();

	if (g_enabled)
	{
		PrimaryMode_EnsurePrimary();
		Indicator_OnLayoutChanged(Layout_Current());
	}
}

void Indicator_SetCountdown(float frac)
{
	if (!g_enabled || !g_shown) return;
	for (int e = 0; e < 4; e++)
	{
		if (!g_hwnd[e]) continue;
		RECT rc; ComputeRect((IndicatorEdge)e, frac, &rc);
		SetWindowPos(g_hwnd[e], NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
			SWP_NOZORDER | SWP_NOACTIVATE);
	}
}

void Indicator_OnRainbowTick(void)
{
	if (!g_rainbow || !g_shown) return;
	g_rainbowHue = fmod(g_rainbowHue + 137.5, 360.0);
	ApplyBrushColor(HsvToColorref(g_rainbowHue));
}

void Indicator_RefreshAppearance(void)
{
	ApplyConfigGeometry();
	SyncEdgeWindows();

	g_rainbow = Config_GetIndicatorRainbow(FALSE);
	COLORREF col = g_rainbow ? HsvToColorref(g_rainbowHue)
	                         : Config_GetIndicatorColor(IND_DEF_COLORREF);
	ApplyBrushColor(col);

	if (g_shown)
	{
		for (int e = 0; e < 4; e++)
		{
			if (!g_hwnd[e]) continue;
			RECT rc; ComputeRect((IndicatorEdge)e, 1.0f, &rc);
			SetWindowPos(g_hwnd[e], NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
				SWP_NOZORDER | SWP_NOACTIVATE);
			ShowWindow(g_hwnd[e], SW_SHOWNA);
		}
	}
	UpdateRainbowTimer();
}

void Indicator_PreviewShow(void)
{
	if (!g_enabled) return;
	ShowStrip();
}

void Indicator_Shutdown(void)
{
	if (g_owner) KillTimer(g_owner, RAINBOW_TIMER_ID);
	for (int e = 0; e < 4; e++)
		if (g_hwnd[e]) { DestroyWindow(g_hwnd[e]); g_hwnd[e] = NULL; }
	if (g_brush) { DeleteObject(g_brush); g_brush = NULL; }
	g_shown = FALSE;
}
