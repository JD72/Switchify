#include "indicator.h"
#include "config.h"
#include "layout.h"
#include "primary_layout.h"

#define INDICATOR_CLASS  L"SwitchifyIndicator"
#define IND_DEF_COLORREF RGB((IND_DEF_COLOR >> 16) & 0xFF, (IND_DEF_COLOR >> 8) & 0xFF, IND_DEF_COLOR & 0xFF)

static HWND   g_hwnd = NULL;
static HBRUSH g_brush = NULL;
static BOOL   g_enabled = FALSE;
static BOOL   g_shown = FALSE;

static IndicatorEdge  g_edge   = IND_EDGE_BOTTOM;
static IndicatorAlign g_align  = IND_ALIGN_CENTER;
static int g_th      = 1;
static int g_fullLen = 1;
static int g_scrW    = 0;
static int g_scrH    = 0;

static void ComputeRect(float frac, RECT* out)
{
	if (frac < 0.0f) frac = 0.0f;
	if (frac > 1.0f) frac = 1.0f;
	BOOL vertical = (g_edge == IND_EDGE_LEFT || g_edge == IND_EDGE_RIGHT);
	int longAxis = vertical ? g_scrH : g_scrW;

	int len = (int)(g_fullLen * frac);
	if (len < 1) len = 1;
	if (len > longAxis) len = longAxis;

	int off = 0;
	if (g_align == IND_ALIGN_CENTER)   off = (longAxis - len) / 2;
	else if (g_align == IND_ALIGN_END) off = longAxis - len;
	if (off < 0) off = 0;

	switch (g_edge)
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
	UINT           thickness = Config_GetIndicatorThickness(IND_DEF_THICKNESS);
	UINT           lengthPct = Config_GetIndicatorLength(IND_DEF_LENGTH);
	IndicatorEdge  edge      = Config_GetIndicatorEdge(IND_DEF_EDGE);
	IndicatorAlign align     = Config_GetIndicatorAlign(IND_DEF_ALIGN);

	int W = GetSystemMetrics(SM_CXSCREEN);
	int H = GetSystemMetrics(SM_CYSCREEN);
	BOOL vertical = (edge == IND_EDGE_LEFT || edge == IND_EDGE_RIGHT);
	int shortMax = vertical ? W : H;
	int longAxis = vertical ? H : W;

	int th = (int)thickness;
	if (th < 1) th = 1;
	if (th > shortMax) th = shortMax;

	int len = (int)(((long long)longAxis * (long long)lengthPct) / 100);
	if (len < 1) len = 1;
	if (len > longAxis) len = longAxis;

	g_edge    = edge;
	g_align   = align;
	g_th      = th;
	g_fullLen = len;
	g_scrW    = W;
	g_scrH    = H;
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
	if (g_hwnd && !g_shown)
	{
		RECT rc; ComputeRect(1.0f, &rc);
		SetWindowPos(g_hwnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
			SWP_NOZORDER | SWP_NOACTIVATE);
		ShowWindow(g_hwnd, SW_SHOWNA);
		g_shown = TRUE;
	}
}

static void HideStrip(void)
{
	if (g_hwnd && g_shown)
	{
		ShowWindow(g_hwnd, SW_HIDE);
		g_shown = FALSE;
	}
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

void Indicator_Init(void)
{
	g_enabled = Config_GetIndicatorEnabled(FALSE);
	if (g_enabled)
		Config_EnsureIndicatorAppearance();

	g_brush = CreateSolidBrush(Config_GetIndicatorColor(IND_DEF_COLORREF));

	ApplyConfigGeometry();

	RECT rc; ComputeRect(1.0f, &rc);
	int x = rc.left, y = rc.top, w = rc.right - rc.left, h = rc.bottom - rc.top;

	WNDCLASSW wc = { 0 };
	wc.lpfnWndProc = IndicatorProc;
	wc.hInstance = GetModuleHandleW(NULL);
	wc.lpszClassName = INDICATOR_CLASS;
	wc.hbrBackground = g_brush;
	RegisterClassW(&wc);

	g_hwnd = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
		INDICATOR_CLASS, L"", WS_POPUP,
		x, y, w, h,
		NULL, NULL, GetModuleHandleW(NULL), NULL);
	if (g_hwnd)
		SetLayeredWindowAttributes(g_hwnd, 0, 255, LWA_ALPHA);

	if (g_enabled)
	{
		PrimaryMode_EnsurePrimary();
		Indicator_OnLayoutChanged(Layout_Current());
	}
}

void Indicator_SetCountdown(float frac)
{
	if (!g_enabled || !g_shown || !g_hwnd) return;
	RECT rc; ComputeRect(frac, &rc);
	SetWindowPos(g_hwnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		SWP_NOZORDER | SWP_NOACTIVATE);
}

void Indicator_RefreshAppearance(void)
{
	if (!g_hwnd) return;

	HBRUSH newBrush = CreateSolidBrush(Config_GetIndicatorColor(IND_DEF_COLORREF));
	if (newBrush)
	{
		SetClassLongPtrW(g_hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)newBrush);
		if (g_brush) DeleteObject(g_brush);
		g_brush = newBrush;
		InvalidateRect(g_hwnd, NULL, TRUE);
	}

	ApplyConfigGeometry();
	if (g_shown)
	{
		RECT rc; ComputeRect(1.0f, &rc);
		SetWindowPos(g_hwnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
			SWP_NOZORDER | SWP_NOACTIVATE);
	}
}

void Indicator_PreviewShow(void)
{
	if (!g_enabled || !g_hwnd) return;
	ShowStrip();
}

void Indicator_Shutdown(void)
{
	if (g_hwnd) { DestroyWindow(g_hwnd); g_hwnd = NULL; }
	if (g_brush) { DeleteObject(g_brush); g_brush = NULL; }
	g_shown = FALSE;
}
