#include "autostart.h"
#include <string.h>

static const char* RUN_KEY = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const char* VALUE_NAME = "Switchify";

BOOL Autostart_IsEnabled(void)
{
	HKEY hKey = NULL;
	if (RegOpenKeyExA(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
	{
		return FALSE;
	}
	LONG res = RegQueryValueExA(hKey, VALUE_NAME, NULL, NULL, NULL, NULL);
	RegCloseKey(hKey);
	return res == ERROR_SUCCESS;
}

void Autostart_SetEnabled(BOOL enable)
{
	HKEY hKey = NULL;
	if (RegOpenKeyExA(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
	{
		return;
	}

	if (enable)
	{
		char exePath[MAX_PATH] = { 0 };
		DWORD len = GetModuleFileNameA(NULL, exePath, MAX_PATH);
		if (len > 0 && len < MAX_PATH)
		{
			RegSetValueExA(hKey, VALUE_NAME, 0, REG_SZ,
				(const BYTE*)exePath, (DWORD)(strlen(exePath) + 1));
		}
	}
	else
	{
		RegDeleteValueA(hKey, VALUE_NAME);
	}

	RegCloseKey(hKey);
}
