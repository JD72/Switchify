#include "i18n.h"
#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const wchar_t* const g_en[STR_COUNT] = {
	L"Enable", L"Disable", L"Logging", L"Run at startup",
	L"Language", L"Primary layout mode", L"Primary layout",
	L"Timer (%u sec)", L"%u sec", L"Countdown indicator",
	L"Layout indicator", L"About", L"Exit",
	L"Enabled", L"Color", L"Custom…", L"Thickness",
	L"Length", L"Edge", L"Alignment",
	L"Orange", L"Red", L"Yellow",
	L"Green", L"Blue", L"Magenta", L"Rainbow",
	L"Top", L"Bottom", L"Left", L"Right",
	L"Start", L"Center", L"End",
	L"About",
	L"Switchify\nSwitches the keyboard layout with CapsLock.\nFork of Switchy by erryox.\n"
	L"GitHub: <A HREF=\"https://github.com/JD72/Switchify\">https://github.com/JD72/Switchify</A>\n\nShortcuts:\n"
	L"  CapsLock          — switch layout\n"
	L"  Shift+CapsLock    — toggle Caps Lock\n"
	L"  Double click      — switch layout",
	L"Switchify",
	L"Logging changes take effect after restarting Switchify.\n\nRestart now?"
};

static const char* const g_keys[STR_COUNT] = {
	"menu.enable", "menu.disable", "menu.log", "menu.autostart",
	"menu.language", "menu.primaryMode", "menu.primary",
	"menu.timerFmt", "timer.secFmt", "menu.countdown",
	"menu.indicator", "menu.about", "menu.exit",
	"ind.enabled", "ind.color", "ind.custom", "ind.thickness",
	"ind.length", "ind.edge", "ind.align",
	"color.orange", "color.red", "color.yellow",
	"color.green", "color.blue", "color.magenta", "color.rainbow",
	"edge.top", "edge.bottom", "edge.left", "edge.right",
	"align.start", "align.center", "align.end",
	"about.title", "about.body",
	"dlg.title", "log.restartBody"
};

static wchar_t* g_loaded[STR_COUNT] = { 0 };
static char     g_curLang[8] = "en";
static char     g_dir[MAX_PATH] = { 0 };

static void FreeLoaded(void)
{
	for (int i = 0; i < STR_COUNT; i++)
	{
		if (g_loaded[i]) { free(g_loaded[i]); g_loaded[i] = NULL; }
	}
}

static int KeyToId(const char* key)
{
	for (int i = 0; i < STR_COUNT; i++)
		if (strcmp(key, g_keys[i]) == 0) return i;
	return -1;
}

static void UnescapeNewlines(char* s)
{
	char* w = s;
	for (char* r = s; *r; )
	{
		if (r[0] == '\\' && r[1] == 'n') { *w++ = '\n'; r += 2; }
		else { *w++ = *r++; }
	}
	*w = '\0';
}

static void StoreWide(int id, const char* utf8)
{
	int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	if (wlen <= 0) return;
	wchar_t* buf = (wchar_t*)malloc((size_t)wlen * sizeof(wchar_t));
	if (!buf) return;
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, buf, wlen);
	if (g_loaded[id]) free(g_loaded[id]);
	g_loaded[id] = buf;
}

static void LoadFile(const char* code)
{
	FreeLoaded();

	char path[MAX_PATH] = { 0 };
	sprintf_s(path, sizeof(path), "%s\\%s.lng", g_dir, code);

	HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) return;

	DWORD size = GetFileSize(h, NULL);
	if (size == INVALID_FILE_SIZE || size == 0) { CloseHandle(h); return; }
	if (size > 65536) size = 65536;
	char* data = (char*)malloc((size_t)size + 1);
	if (!data) { CloseHandle(h); return; }
	DWORD got = 0;
	ReadFile(h, data, size, &got, NULL);
	CloseHandle(h);
	data[got] = '\0';

	char* p = data;
	if (got >= 3 && (unsigned char)p[0] == 0xEF &&
		(unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF)
		p += 3;

	while (*p)
	{
		char* nl = strchr(p, '\n');
		char* lineEnd = nl ? nl : (p + strlen(p));
		char saved = *lineEnd;
		*lineEnd = '\0';

		size_t len = strlen(p);
		while (len && (p[len - 1] == '\r' || p[len - 1] == ' ' || p[len - 1] == '\t'))
			p[--len] = '\0';
		char* line = p;
		while (*line == ' ' || *line == '\t') line++;

		if (*line && *line != '#')
		{
			char* eq = strchr(line, '=');
			if (eq)
			{
				*eq = '\0';
				char* key = line;
				char* val = eq + 1;
				size_t klen = strlen(key);
				while (klen && (key[klen - 1] == ' ' || key[klen - 1] == '\t'))
					key[--klen] = '\0';
				while (*key == ' ' || *key == '\t') key++;
				while (*val == ' ' || *val == '\t') val++;

				int id = KeyToId(key);
				if (id >= 0)
				{
					UnescapeNewlines(val);
					StoreWide(id, val);
				}
			}
		}

		if (!nl) break;
		*lineEnd = saved;
		p = nl + 1;
	}

	free(data);
}

void I18n_Init(void)
{
	char exePath[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, exePath, MAX_PATH);
	char* sep = strrchr(exePath, '\\');
	if (sep) *sep = '\0';
	strncpy_s(g_dir, MAX_PATH, exePath, _TRUNCATE);

	char code[8] = { 0 };
	Config_GetUiLang(code, sizeof(code));
	strncpy_s(g_curLang, sizeof(g_curLang), code, _TRUNCATE);
	LoadFile(g_curLang);
}

void I18n_SetLanguage(const char* code)
{
	if (!code || !*code) return;
	Config_SetUiLang(code);
	strncpy_s(g_curLang, sizeof(g_curLang), code, _TRUNCATE);
	LoadFile(g_curLang);
}

const wchar_t* I18n_Get(StrId id)
{
	if ((int)id < 0 || id >= STR_COUNT) return g_en[0];
	return g_loaded[id] ? g_loaded[id] : g_en[id];
}

const char* I18n_CurrentLang(void)
{
	return g_curLang;
}
