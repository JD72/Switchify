#pragma once
#include <Windows.h>

void Config_Init(void);

BOOL Config_GetLogEnabled(BOOL fallback);

void Config_SetLogEnabled(BOOL value);

BOOL   Config_GetPrimaryModeEnabled(BOOL fallback);
void   Config_SetPrimaryModeEnabled(BOOL value);
LANGID Config_GetPrimaryLang(LANGID fallback);
void   Config_SetPrimaryLang(LANGID lang);
UINT   Config_GetInactivitySeconds(UINT fallback);
void   Config_SetInactivitySeconds(UINT seconds);

typedef enum { IND_EDGE_TOP = 0, IND_EDGE_BOTTOM = 1, IND_EDGE_LEFT = 2, IND_EDGE_RIGHT = 3 } IndicatorEdge;
typedef enum { IND_ALIGN_START = 0, IND_ALIGN_CENTER = 1, IND_ALIGN_END = 2 } IndicatorAlign;

#define IND_DEF_COLOR     0xFF8000u
#define IND_DEF_THICKNESS 5u
#define IND_DEF_LENGTH    100u
#define IND_DEF_EDGE      IND_EDGE_BOTTOM
#define IND_DEF_ALIGN     IND_ALIGN_CENTER

BOOL           Config_GetIndicatorEnabled(BOOL fallback);
void           Config_SetIndicatorEnabled(BOOL value);
COLORREF       Config_GetIndicatorColor(COLORREF fallback);
UINT           Config_GetIndicatorThickness(UINT fallback);
UINT           Config_GetIndicatorLength(UINT fallback);
IndicatorEdge  Config_GetIndicatorEdge(IndicatorEdge fallback);
IndicatorAlign Config_GetIndicatorAlign(IndicatorAlign fallback);
void           Config_EnsureIndicatorAppearance(void);
void           Config_SetIndicatorColor(COLORREF color);
void           Config_SetIndicatorThickness(UINT px);
void           Config_SetIndicatorLength(UINT pct);
void           Config_SetIndicatorEdge(IndicatorEdge edge);
void           Config_SetIndicatorAlign(IndicatorAlign align);

BOOL Config_GetCountdownEnabled(BOOL fallback);
void Config_SetCountdownEnabled(BOOL value);

void Config_GetUiLang(char* out, size_t outSize);
void Config_SetUiLang(const char* code);
