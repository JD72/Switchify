#pragma once
#include <Windows.h>

typedef enum { LOG_INFO, LOG_ERROR } LogLevel;

void Log_Init(BOOL enabledByDefault);

void Log_SetEnabled(BOOL enable);

BOOL Log_IsEnabled(void);

void Log_Write(LogLevel level, const char* msg);

void Log_Shutdown(void);
