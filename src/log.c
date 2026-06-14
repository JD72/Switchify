#include "log.h"
#include <stdio.h>
#include <string.h>

static FILE* g_file = NULL;
static BOOL  g_enabled = FALSE;
static int   g_currentDay = 0;
static char  g_logDir[MAX_PATH] = { 0 };

static void ComputeLogDir(void)
{
	char exePath[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, exePath, MAX_PATH);
	char* sep = strrchr(exePath, '\\');
	if (sep) *sep = '\0';
	strncpy_s(g_logDir, MAX_PATH, exePath, _TRUNCATE);
	strncat_s(g_logDir, MAX_PATH, "\\log", _TRUNCATE);
}

static BOOL EnsureLogDir(void)
{
	if (CreateDirectoryA(g_logDir, NULL)) return TRUE;
	return GetLastError() == ERROR_ALREADY_EXISTS;
}

static FILE* OpenTodayFile(const SYSTEMTIME* now)
{
	char path[MAX_PATH] = { 0 };
	snprintf(path, MAX_PATH, "%s\\switchy-%04d-%02d-%02d.log",
		g_logDir, now->wYear, now->wMonth, now->wDay);
	FILE* f = NULL;
	if (fopen_s(&f, path, "a") != 0) return NULL;
	g_currentDay = now->wDay;
	return f;
}

void Log_Init(BOOL enabledByDefault)
{
	ComputeLogDir();
	g_enabled = enabledByDefault;
}

void Log_SetEnabled(BOOL enable)
{
	if (g_enabled && !enable)
	{
		Log_Write(LOG_INFO, "Logging disabled");
		g_enabled = FALSE;
		if (g_file)
		{
			fclose(g_file);
			g_file = NULL;
		}
	}
	else if (!g_enabled && enable)
	{
		g_enabled = TRUE;
		Log_Write(LOG_INFO, "Logging enabled");
	}
}

BOOL Log_IsEnabled(void)
{
	return g_enabled;
}

void Log_Write(LogLevel level, const char* msg)
{
	if (!g_enabled) return;

	SYSTEMTIME now;
	GetLocalTime(&now);

	if (!g_file || g_currentDay != (int)now.wDay)
	{
		if (g_file) { fclose(g_file); g_file = NULL; }
		if (!EnsureLogDir()) return;
		g_file = OpenTodayFile(&now);
		if (!g_file) return;
	}

	const char* levelName = (level == LOG_ERROR) ? "ERROR" : "INFO";
	fprintf(g_file, "[%04d-%02d-%02d %02d:%02d:%02d] %s: %s\n",
		now.wYear, now.wMonth, now.wDay,
		now.wHour, now.wMinute, now.wSecond,
		levelName, msg);
	fflush(g_file);
}

void Log_Shutdown(void)
{
	if (g_file)
	{
		fclose(g_file);
		g_file = NULL;
	}
}
