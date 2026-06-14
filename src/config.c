#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

static char g_iniPath[MAX_PATH] = { 0 };

void Config_Init(void)
{
	char exePath[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, exePath, MAX_PATH);
	char* sep = strrchr(exePath, '\\');
	if (sep) *sep = '\0';
	strncpy_s(g_iniPath, MAX_PATH, exePath, _TRUNCATE);
	strncat_s(g_iniPath, MAX_PATH, "\\switchy.ini", _TRUNCATE);
}

BOOL Config_GetLogEnabled(BOOL fallback)
{
	char buf[8] = { 0 };
	GetPrivateProfileStringA("General", "LogEnabled", "", buf, sizeof(buf), g_iniPath);
	if (strcmp(buf, "0") == 0) return FALSE;
	if (strcmp(buf, "1") == 0) return TRUE;
	return fallback;
}

void Config_SetLogEnabled(BOOL value)
{
	WritePrivateProfileStringA("General", "LogEnabled", value ? "1" : "0", g_iniPath);
}

BOOL Config_GetPrimaryModeEnabled(BOOL fallback)
{
	char buf[8] = { 0 };
	GetPrivateProfileStringA("PrimaryLayout", "ModeEnabled", "", buf, sizeof(buf), g_iniPath);
	if (strcmp(buf, "0") == 0) return FALSE;
	if (strcmp(buf, "1") == 0) return TRUE;
	return fallback;
}

void Config_SetPrimaryModeEnabled(BOOL value)
{
	WritePrivateProfileStringA("PrimaryLayout", "ModeEnabled", value ? "1" : "0", g_iniPath);
}

LANGID Config_GetPrimaryLang(LANGID fallback)
{
	char buf[16] = { 0 };
	GetPrivateProfileStringA("PrimaryLayout", "PrimaryLang", "", buf, sizeof(buf), g_iniPath);
	if (buf[0] == '\0') return fallback;
	LANGID v = (LANGID)strtoul(buf, NULL, 16);
	return (v != 0) ? v : fallback;
}

void Config_SetPrimaryLang(LANGID lang)
{
	char buf[16] = { 0 };
	sprintf_s(buf, sizeof(buf), "%04x", (unsigned)lang);
	WritePrivateProfileStringA("PrimaryLayout", "PrimaryLang", buf, g_iniPath);
}

UINT Config_GetInactivitySeconds(UINT fallback)
{
	UINT v = GetPrivateProfileIntA("PrimaryLayout", "InactivitySeconds", fallback, g_iniPath);
	return (v != 0) ? v : fallback;
}

void Config_SetInactivitySeconds(UINT seconds)
{
	char buf[16];
	sprintf_s(buf, sizeof(buf), "%u", seconds);
	WritePrivateProfileStringA("PrimaryLayout", "InactivitySeconds", buf, g_iniPath);
}

BOOL Config_GetIndicatorEnabled(BOOL fallback)
{
	char buf[8] = { 0 };
	GetPrivateProfileStringA("Indicator", "Enabled", "", buf, sizeof(buf), g_iniPath);
	if (strcmp(buf, "0") == 0) return FALSE;
	if (strcmp(buf, "1") == 0) return TRUE;
	return fallback;
}

void Config_SetIndicatorEnabled(BOOL value)
{
	WritePrivateProfileStringA("Indicator", "Enabled", value ? "1" : "0", g_iniPath);
}

COLORREF Config_GetIndicatorColor(COLORREF fallback)
{
	char buf[16] = { 0 };
	GetPrivateProfileStringA("Indicator", "Color", "", buf, sizeof(buf), g_iniPath);
	if (strlen(buf) != 6) return fallback;
	for (int i = 0; i < 6; i++)
		if (!isxdigit((unsigned char)buf[i])) return fallback;
	DWORD rgb = (DWORD)strtoul(buf, NULL, 16);
	return RGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

UINT Config_GetIndicatorThickness(UINT fallback)
{
	char buf[16] = { 0 };
	GetPrivateProfileStringA("Indicator", "Thickness", "", buf, sizeof(buf), g_iniPath);
	if (buf[0] == '\0') return fallback;
	long v = strtol(buf, NULL, 10);
	if (v < 1) return fallback;
	if (v > 10000) v = 10000;
	return (UINT)v;
}

UINT Config_GetIndicatorLength(UINT fallback)
{
	char buf[16] = { 0 };
	GetPrivateProfileStringA("Indicator", "Length", "", buf, sizeof(buf), g_iniPath);
	if (buf[0] == '\0') return fallback;
	long v = strtol(buf, NULL, 10);
	if (v < 1) return fallback;
	if (v > 100) v = 100;
	return (UINT)v;
}

IndicatorEdge Config_GetIndicatorEdge(IndicatorEdge fallback)
{
	char buf[16] = { 0 };
	GetPrivateProfileStringA("Indicator", "Edge", "", buf, sizeof(buf), g_iniPath);
	if (_stricmp(buf, "top") == 0)    return IND_EDGE_TOP;
	if (_stricmp(buf, "bottom") == 0) return IND_EDGE_BOTTOM;
	if (_stricmp(buf, "left") == 0)   return IND_EDGE_LEFT;
	if (_stricmp(buf, "right") == 0)  return IND_EDGE_RIGHT;
	return fallback;
}

IndicatorAlign Config_GetIndicatorAlign(IndicatorAlign fallback)
{
	char buf[16] = { 0 };
	GetPrivateProfileStringA("Indicator", "Align", "", buf, sizeof(buf), g_iniPath);
	if (_stricmp(buf, "start") == 0)  return IND_ALIGN_START;
	if (_stricmp(buf, "center") == 0) return IND_ALIGN_CENTER;
	if (_stricmp(buf, "end") == 0)    return IND_ALIGN_END;
	return fallback;
}

void Config_EnsureIndicatorAppearance(void)
{
	char buf[16] = { 0 };
	char val[16];
	GetPrivateProfileStringA("Indicator", "Color", "", buf, sizeof(buf), g_iniPath);
	if (buf[0] == '\0')
	{
		sprintf_s(val, sizeof(val), "%06X", (unsigned)(IND_DEF_COLOR & 0xFFFFFFu));
		WritePrivateProfileStringA("Indicator", "Color", val, g_iniPath);
	}
	buf[0] = '\0'; GetPrivateProfileStringA("Indicator", "Thickness", "", buf, sizeof(buf), g_iniPath);
	if (buf[0] == '\0') { sprintf_s(val, sizeof(val), "%u", IND_DEF_THICKNESS); WritePrivateProfileStringA("Indicator", "Thickness", val, g_iniPath); }
	buf[0] = '\0'; GetPrivateProfileStringA("Indicator", "Edge", "", buf, sizeof(buf), g_iniPath);
	if (buf[0] == '\0') WritePrivateProfileStringA("Indicator", "Edge", "bottom", g_iniPath);
	buf[0] = '\0'; GetPrivateProfileStringA("Indicator", "Length", "", buf, sizeof(buf), g_iniPath);
	if (buf[0] == '\0') { sprintf_s(val, sizeof(val), "%u", IND_DEF_LENGTH); WritePrivateProfileStringA("Indicator", "Length", val, g_iniPath); }
	buf[0] = '\0'; GetPrivateProfileStringA("Indicator", "Align", "", buf, sizeof(buf), g_iniPath);
	if (buf[0] == '\0') WritePrivateProfileStringA("Indicator", "Align", "center", g_iniPath);
}

void Config_SetIndicatorColor(COLORREF color)
{
	char buf[8];
	sprintf_s(buf, sizeof(buf), "%02X%02X%02X",
		GetRValue(color), GetGValue(color), GetBValue(color));
	WritePrivateProfileStringA("Indicator", "Color", buf, g_iniPath);
}

void Config_SetIndicatorThickness(UINT px)
{
	char buf[16];
	sprintf_s(buf, sizeof(buf), "%u", px);
	WritePrivateProfileStringA("Indicator", "Thickness", buf, g_iniPath);
}

void Config_SetIndicatorLength(UINT pct)
{
	char buf[16];
	sprintf_s(buf, sizeof(buf), "%u", pct);
	WritePrivateProfileStringA("Indicator", "Length", buf, g_iniPath);
}

void Config_SetIndicatorEdge(IndicatorEdge edge)
{
	const char* s = "bottom";
	switch (edge)
	{
	case IND_EDGE_TOP:    s = "top";    break;
	case IND_EDGE_BOTTOM: s = "bottom"; break;
	case IND_EDGE_LEFT:   s = "left";   break;
	case IND_EDGE_RIGHT:  s = "right";  break;
	}
	WritePrivateProfileStringA("Indicator", "Edge", s, g_iniPath);
}

void Config_SetIndicatorAlign(IndicatorAlign align)
{
	const char* s = "center";
	switch (align)
	{
	case IND_ALIGN_START:  s = "start";  break;
	case IND_ALIGN_CENTER: s = "center"; break;
	case IND_ALIGN_END:    s = "end";    break;
	}
	WritePrivateProfileStringA("Indicator", "Align", s, g_iniPath);
}

BOOL Config_GetCountdownEnabled(BOOL fallback)
{
	char buf[8] = { 0 };
	GetPrivateProfileStringA("Countdown", "Enabled", "", buf, sizeof(buf), g_iniPath);
	if (strcmp(buf, "0") == 0) return FALSE;
	if (strcmp(buf, "1") == 0) return TRUE;
	return fallback;
}

void Config_SetCountdownEnabled(BOOL value)
{
	WritePrivateProfileStringA("Countdown", "Enabled", value ? "1" : "0", g_iniPath);
}

void Config_GetUiLang(char* out, size_t outSize)
{
	if (!out || outSize == 0) return;
	char buf[16] = { 0 };
	GetPrivateProfileStringA("General", "Language", "", buf, sizeof(buf), g_iniPath);

	char clean[8] = { 0 };
	int n = 0;
	for (int i = 0; buf[i] && n < 7; i++)
	{
		char c = buf[i];
		if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
		if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) clean[n++] = c;
	}
	if (n == 0) strcpy_s(clean, sizeof(clean), "en");
	strncpy_s(out, outSize, clean, _TRUNCATE);
}

void Config_SetUiLang(const char* code)
{
	if (!code) return;
	WritePrivateProfileStringA("General", "Language", code, g_iniPath);
}
