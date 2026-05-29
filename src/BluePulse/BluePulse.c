#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define APP_NAME L"BluePulse"
#define APP_TAGLINE L"Mant\x00E9n viva tu VM con un pulso azul"
#define APP_MUTEX_NAME L"WinIdleKeeper.SingleInstance"
#define APP_SETTINGS_FOLDER L"WinIdleKeeper"
#define IDI_APP_ICON 101

#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 500
#define WINDOW_MIN_WIDTH 640
#define WINDOW_MIN_HEIGHT 460

#define ID_TIMER 1
#define WMAPP_TRAYICON (WM_APP + 1)

#define ID_ENABLE_CHECK 1001
#define ID_IDLE_EDIT 1002
#define ID_IDLE_SPIN 1003
#define ID_STATE_VALUE 1004
#define ID_IDLE_VALUE 1005
#define ID_LAST_ACTION_VALUE 1006
#define ID_DETAIL_LABEL 1007
#define ID_NOTE_LABEL 1008
#define ID_MINIMIZE_BUTTON 1009
#define ID_EXIT_BUTTON 1010
#define ID_LABEL_CONTROL 1011
#define ID_LABEL_IDLE_THRESHOLD 1012
#define ID_LABEL_STATE 1013
#define ID_LABEL_IDLE_CURRENT 1014
#define ID_LABEL_LAST_ACTION 1015
#define ID_IDLE_UNIT 1016

#define ID_TRAY_OPEN 2001
#define ID_TRAY_TOGGLE 2002
#define ID_TRAY_EXIT 2003

typedef struct AppSettings {
    BOOL enabled;
    int idleMinutes;
    BOOL minimizeToTrayOnClose;
} AppSettings;

static const COLORREF APP_COLOR_NAVY = RGB(7, 33, 70);
static const COLORREF APP_COLOR_CORE_BLUE = RGB(0, 68, 129);
static const COLORREF APP_COLOR_MEDIUM_BLUE = RGB(25, 115, 184);
static const COLORREF APP_COLOR_SKY_BLUE = RGB(91, 190, 255);
static const COLORREF APP_COLOR_AQUA = RGB(45, 204, 205);
static const COLORREF APP_COLOR_BACKGROUND = RGB(244, 248, 251);
static const COLORREF APP_COLOR_SURFACE = RGB(255, 255, 255);
static const COLORREF APP_COLOR_MUTED = RGB(91, 99, 110);
static const COLORREF APP_COLOR_DANGER = RGB(183, 34, 48);

static HINSTANCE g_instance = NULL;
static HWND g_mainWindow = NULL;
static HWND g_enabledCheck = NULL;
static HWND g_idleEdit = NULL;
static HWND g_idleSpin = NULL;
static HWND g_stateValue = NULL;
static HWND g_idleValue = NULL;
static HWND g_lastActionValue = NULL;
static HWND g_detailLabel = NULL;
static HWND g_noteLabel = NULL;
static HWND g_minimizeButton = NULL;
static HWND g_exitButton = NULL;
static HWND g_leftLabels[5] = { NULL };
static HWND g_idleUnitLabel = NULL;
static HFONT g_titleFont = NULL;
static HFONT g_subtitleFont = NULL;
static HFONT g_bodyFont = NULL;
static HFONT g_buttonFont = NULL;
static HBRUSH g_backgroundBrush = NULL;
static HBRUSH g_surfaceBrush = NULL;
static HBRUSH g_headerBrush = NULL;
static HICON g_appIcon = NULL;
static HMENU g_trayMenu = NULL;
static NOTIFYICONDATAW g_trayIconData;
static HANDLE g_singleInstanceMutex = NULL;
static AppSettings g_settings = { FALSE, 4, TRUE };
static BOOL g_isExiting = FALSE;
static BOOL g_isUpdatingIdleInput = FALSE;
static ULONGLONG g_lastRenewTick = 0;
static wchar_t g_lastActionText[160] = L"Sin acciones";

static HFONT CreateUiFont(int pointSize, int weight)
{
    HDC screen = GetDC(NULL);
    int height = -MulDiv(pointSize, GetDeviceCaps(screen, LOGPIXELSY), 72);
    HFONT font;
    ReleaseDC(NULL, screen);

    font = CreateFontW(
        height,
        0,
        0,
        0,
        weight,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI");
    return font;
}

static HICON LoadAppIcon(void)
{
    HICON icon = (HICON)LoadImageW(
        g_instance,
        MAKEINTRESOURCEW(IDI_APP_ICON),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON),
        LR_DEFAULTCOLOR);

    if (icon == NULL) {
        icon = LoadIconW(NULL, MAKEINTRESOURCEW(32512));
    }

    return icon;
}

static BOOL GetSettingsPath(wchar_t *path, size_t pathCount)
{
    wchar_t appData[MAX_PATH];
    wchar_t directory[MAX_PATH];

    if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appData) != S_OK) {
        return FALSE;
    }

    swprintf(directory, MAX_PATH, L"%ls\\%ls", appData, APP_SETTINGS_FOLDER);
    CreateDirectoryW(directory, NULL);
    swprintf(path, pathCount, L"%ls\\settings.json", directory);
    return TRUE;
}

static BOOL ParseJsonBool(const char *json, const char *key, BOOL fallbackValue)
{
    char pattern[64];
    char *match;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    match = strstr(json, pattern);
    if (match == NULL) {
        return fallbackValue;
    }

    match = strchr(match, ':');
    if (match == NULL) {
        return fallbackValue;
    }

    match++;
    while (*match == ' ' || *match == '\t' || *match == '\r' || *match == '\n') {
        match++;
    }

    if (_strnicmp(match, "true", 4) == 0) {
        return TRUE;
    }

    if (_strnicmp(match, "false", 5) == 0) {
        return FALSE;
    }

    return fallbackValue;
}

static int ParseJsonInt(const char *json, const char *key, int fallbackValue)
{
    char pattern[64];
    char *match;
    long value;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    match = strstr(json, pattern);
    if (match == NULL) {
        return fallbackValue;
    }

    match = strchr(match, ':');
    if (match == NULL) {
        return fallbackValue;
    }

    match++;
    while (*match == ' ' || *match == '\t' || *match == '\r' || *match == '\n') {
        match++;
    }

    value = strtol(match, NULL, 10);
    if (value < 1 || value > 60) {
        return fallbackValue;
    }

    return (int)value;
}

static void LoadSettings(AppSettings *settings)
{
    wchar_t path[MAX_PATH];
    FILE *file;
    long length;
    char *json;
    size_t bytesRead;

    settings->enabled = FALSE;
    settings->idleMinutes = 4;
    settings->minimizeToTrayOnClose = TRUE;

    if (!GetSettingsPath(path, MAX_PATH)) {
        return;
    }

    file = _wfopen(path, L"rb");
    if (file == NULL) {
        return;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return;
    }

    length = ftell(file);
    if (length <= 0) {
        fclose(file);
        return;
    }

    rewind(file);
    json = (char *)malloc((size_t)length + 1);
    if (json == NULL) {
        fclose(file);
        return;
    }

    bytesRead = fread(json, 1, (size_t)length, file);
    fclose(file);
    json[bytesRead] = '\0';

    settings->enabled = ParseJsonBool(json, "Enabled", settings->enabled);
    settings->idleMinutes = ParseJsonInt(json, "IdleMinutes", settings->idleMinutes);
    settings->minimizeToTrayOnClose = ParseJsonBool(json, "MinimizeToTrayOnClose", settings->minimizeToTrayOnClose);

    free(json);
}

static void SaveSettings(const AppSettings *settings)
{
    wchar_t path[MAX_PATH];
    FILE *file;

    if (!GetSettingsPath(path, MAX_PATH)) {
        return;
    }

    file = _wfopen(path, L"wb");
    if (file == NULL) {
        return;
    }

    fprintf(
        file,
        "{\n"
        "  \"Enabled\": %s,\n"
        "  \"IdleMinutes\": %d,\n"
        "  \"MinimizeToTrayOnClose\": %s\n"
        "}\n",
        settings->enabled ? "true" : "false",
        settings->idleMinutes,
        settings->minimizeToTrayOnClose ? "true" : "false");

    fclose(file);
}

static void EnsureIdleMinutesRange(void)
{
    if (g_settings.idleMinutes < 1) {
        g_settings.idleMinutes = 1;
    }
    if (g_settings.idleMinutes > 60) {
        g_settings.idleMinutes = 60;
    }
}

static BOOL EnableKeepAwake(void)
{
    EXECUTION_STATE result = SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
    return result != 0;
}

static void DisableKeepAwake(void)
{
    SetThreadExecutionState(ES_CONTINUOUS);
}

static DWORD GetIdleMilliseconds(void)
{
    LASTINPUTINFO inputInfo;
    inputInfo.cbSize = sizeof(inputInfo);
    inputInfo.dwTime = 0;

    if (!GetLastInputInfo(&inputInfo)) {
        return 0;
    }

    return GetTickCount() - inputInfo.dwTime;
}

static void RecordLastAction(const wchar_t *prefix)
{
    SYSTEMTIME localTime;
    GetLocalTime(&localTime);
    swprintf(
        g_lastActionText,
        sizeof(g_lastActionText) / sizeof(g_lastActionText[0]),
        L"%ls a las %02u:%02u:%02u",
        prefix,
        localTime.wHour,
        localTime.wMinute,
        localTime.wSecond);
    SetWindowTextW(g_lastActionValue, g_lastActionText);
}

static void UpdateTrayPresentation(void)
{
    wchar_t tip[128];

    if (g_trayMenu != NULL) {
        ModifyMenuW(g_trayMenu, ID_TRAY_TOGGLE, MF_BYCOMMAND | MF_STRING, ID_TRAY_TOGGLE, g_settings.enabled ? L"Deshabilitar" : L"Habilitar");
    }

    swprintf(tip, sizeof(tip) / sizeof(tip[0]), L"%ls - %ls", APP_NAME, g_settings.enabled ? L"Activado" : L"Desactivado");

    ZeroMemory(&g_trayIconData, sizeof(g_trayIconData));
    g_trayIconData.cbSize = sizeof(g_trayIconData);
    g_trayIconData.hWnd = g_mainWindow;
    g_trayIconData.uID = 1;
    g_trayIconData.uFlags = NIF_TIP | NIF_ICON | NIF_MESSAGE;
    g_trayIconData.uCallbackMessage = WMAPP_TRAYICON;
    g_trayIconData.hIcon = g_appIcon;
    wcsncpy(g_trayIconData.szTip, tip, (sizeof(g_trayIconData.szTip) / sizeof(g_trayIconData.szTip[0])) - 1);
    Shell_NotifyIconW(NIM_MODIFY, &g_trayIconData);
}

static void UpdateStatusPresentation(void)
{
    SetWindowTextW(g_stateValue, g_settings.enabled ? L"Activado" : L"Desactivado");
    SetWindowTextW(g_lastActionValue, g_lastActionText);
    InvalidateRect(g_stateValue, NULL, TRUE);
    UpdateTrayPresentation();
}

static void UpdateIdleLabel(void)
{
    DWORD idleMilliseconds = GetIdleMilliseconds();
    DWORD totalSeconds = idleMilliseconds / 1000;
    DWORD hours = totalSeconds / 3600;
    DWORD minutes = (totalSeconds % 3600) / 60;
    DWORD seconds = totalSeconds % 60;
    wchar_t buffer[32];

    if (hours > 0) {
        swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%02lu:%02lu:%02lu", hours, minutes, seconds);
    } else {
        swprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), L"%02lu:%02lu", minutes, seconds);
    }

    SetWindowTextW(g_idleValue, buffer);

    if (g_settings.enabled) {
        DWORD thresholdMilliseconds = (DWORD)(g_settings.idleMinutes * 60 * 1000);
        ULONGLONG now = GetTickCount64();
        if (idleMilliseconds >= thresholdMilliseconds && now - g_lastRenewTick >= 60000ULL) {
            if (EnableKeepAwake()) {
                g_lastRenewTick = now;
                RecordLastAction(L"Pantalla mantenida activa");
            }
        }
    }
}

static void SetEnabledState(BOOL enabled, BOOL persist)
{
    if (enabled) {
        if (!EnableKeepAwake()) {
            MessageBoxW(g_mainWindow, L"No se pudo activar el mantenimiento de pantalla activa.", APP_NAME, MB_OK | MB_ICONWARNING);
            enabled = FALSE;
        }
    } else {
        DisableKeepAwake();
    }

    g_settings.enabled = enabled;
    SendMessageW(g_enabledCheck, BM_SETCHECK, enabled ? BST_CHECKED : BST_UNCHECKED, 0);

    if (persist) {
        SaveSettings(&g_settings);
    }

    UpdateStatusPresentation();
}

static void HideToTray(void)
{
    NOTIFYICONDATAW balloon;
    ShowWindow(g_mainWindow, SW_HIDE);

    ZeroMemory(&balloon, sizeof(balloon));
    balloon.cbSize = sizeof(balloon);
    balloon.hWnd = g_mainWindow;
    balloon.uID = 1;
    balloon.uFlags = NIF_INFO;
    wcsncpy(balloon.szInfoTitle, APP_NAME, (sizeof(balloon.szInfoTitle) / sizeof(balloon.szInfoTitle[0])) - 1);
    wcsncpy(balloon.szInfo, L"La aplicacion sigue disponible desde la bandeja del sistema.", (sizeof(balloon.szInfo) / sizeof(balloon.szInfo[0])) - 1);
    balloon.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &balloon);
}

static void RestoreFromTray(void)
{
    ShowWindow(g_mainWindow, SW_SHOW);
    ShowWindow(g_mainWindow, SW_RESTORE);
    SetForegroundWindow(g_mainWindow);
}

static void ExitApplication(void)
{
    g_isExiting = TRUE;
    SaveSettings(&g_settings);
    DestroyWindow(g_mainWindow);
}

static void UpdateIdleThresholdFromInput(BOOL persist)
{
    BOOL translated = FALSE;
    int value = GetDlgItemInt(g_mainWindow, ID_IDLE_EDIT, &translated, FALSE);

    if (!translated) {
        return;
    }

    if (value < 1) {
        value = 1;
    }
    if (value > 60) {
        value = 60;
    }

    g_settings.idleMinutes = value;

    g_isUpdatingIdleInput = TRUE;
    SetDlgItemInt(g_mainWindow, ID_IDLE_EDIT, (UINT)g_settings.idleMinutes, FALSE);
    g_isUpdatingIdleInput = FALSE;

    if (persist) {
        SaveSettings(&g_settings);
    }
}

static void CreateUiResources(void)
{
    if (g_titleFont == NULL) {
        g_titleFont = CreateUiFont(18, FW_BOLD);
        g_subtitleFont = CreateUiFont(9, FW_NORMAL);
        g_bodyFont = CreateUiFont(9, FW_NORMAL);
        g_buttonFont = CreateUiFont(9, FW_BOLD);
        g_backgroundBrush = CreateSolidBrush(APP_COLOR_BACKGROUND);
        g_surfaceBrush = CreateSolidBrush(APP_COLOR_SURFACE);
        g_headerBrush = CreateSolidBrush(APP_COLOR_CORE_BLUE);
    }
}

static void DestroyUiResources(void)
{
    DeleteObject(g_titleFont);
    DeleteObject(g_subtitleFont);
    DeleteObject(g_bodyFont);
    DeleteObject(g_buttonFont);
    DeleteObject(g_backgroundBrush);
    DeleteObject(g_surfaceBrush);
    DeleteObject(g_headerBrush);
    g_titleFont = NULL;
    g_subtitleFont = NULL;
    g_bodyFont = NULL;
    g_buttonFont = NULL;
    g_backgroundBrush = NULL;
    g_surfaceBrush = NULL;
    g_headerBrush = NULL;
}

static void CreateControls(HWND hwnd)
{
    int i;
    static const wchar_t *leftTexts[5] = {
        L"Control",
        L"Umbral de inactividad",
        L"Estado",
        L"Inactividad actual",
        L"Ultima accion"
    };
    static const int leftIds[5] = {
        ID_LABEL_CONTROL,
        ID_LABEL_IDLE_THRESHOLD,
        ID_LABEL_STATE,
        ID_LABEL_IDLE_CURRENT,
        ID_LABEL_LAST_ACTION
    };

    CreateUiResources();

    g_detailLabel = CreateWindowExW(0, L"STATIC", L"Mantiene la pantalla activa sin permisos de administrador.\r\nNo simula entrada de usuario ni evita politicas corporativas de bloqueo.", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)ID_DETAIL_LABEL, g_instance, NULL);

    for (i = 0; i < 5; i++) {
        g_leftLabels[i] = CreateWindowExW(0, L"STATIC", leftTexts[i], WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)(INT_PTR)leftIds[i], g_instance, NULL);
    }

    g_enabledCheck = CreateWindowExW(0, L"BUTTON", L"Habilitar utilidad", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0, 0, 0, 0, hwnd, (HMENU)ID_ENABLE_CHECK, g_instance, NULL);
    g_idleEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_AUTOHSCROLL, 0, 0, 0, 0, hwnd, (HMENU)ID_IDLE_EDIT, g_instance, NULL);
    g_idleSpin = CreateWindowExW(0, UPDOWN_CLASSW, NULL, WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_SETBUDDYINT | UDS_ARROWKEYS, 0, 0, 0, 0, hwnd, (HMENU)ID_IDLE_SPIN, g_instance, NULL);
    SendMessageW(g_idleSpin, UDM_SETRANGE32, 1, 60);
    SendMessageW(g_idleSpin, UDM_SETBUDDY, (WPARAM)g_idleEdit, 0);
    g_idleUnitLabel = CreateWindowExW(0, L"STATIC", L"min", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)ID_IDLE_UNIT, g_instance, NULL);

    g_stateValue = CreateWindowExW(0, L"STATIC", L"Desactivado", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)ID_STATE_VALUE, g_instance, NULL);
    g_idleValue = CreateWindowExW(0, L"STATIC", L"00:00", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)ID_IDLE_VALUE, g_instance, NULL);
    g_lastActionValue = CreateWindowExW(0, L"STATIC", L"Sin acciones", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)ID_LAST_ACTION_VALUE, g_instance, NULL);

    g_minimizeButton = CreateWindowExW(0, L"BUTTON", L"Minimizar a bandeja", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_MINIMIZE_BUTTON, g_instance, NULL);
    g_exitButton = CreateWindowExW(0, L"BUTTON", L"Salir", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_EXIT_BUTTON, g_instance, NULL);

    g_noteLabel = CreateWindowExW(0, L"STATIC", L"Nota: si una directiva de empresa fuerza el bloqueo por inactividad, esta aplicacion no la modifica ni la elude.", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)ID_NOTE_LABEL, g_instance, NULL);

    SendMessageW(g_detailLabel, WM_SETFONT, (WPARAM)g_bodyFont, TRUE);
    for (i = 0; i < 5; i++) {
        SendMessageW(g_leftLabels[i], WM_SETFONT, (WPARAM)g_bodyFont, TRUE);
    }
    SendMessageW(g_enabledCheck, WM_SETFONT, (WPARAM)g_bodyFont, TRUE);
    SendMessageW(g_idleEdit, WM_SETFONT, (WPARAM)g_bodyFont, TRUE);
    SendMessageW(g_idleUnitLabel, WM_SETFONT, (WPARAM)g_bodyFont, TRUE);
    SendMessageW(g_stateValue, WM_SETFONT, (WPARAM)g_bodyFont, TRUE);
    SendMessageW(g_idleValue, WM_SETFONT, (WPARAM)g_bodyFont, TRUE);
    SendMessageW(g_lastActionValue, WM_SETFONT, (WPARAM)g_bodyFont, TRUE);
    SendMessageW(g_minimizeButton, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
    SendMessageW(g_exitButton, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
    SendMessageW(g_noteLabel, WM_SETFONT, (WPARAM)g_bodyFont, TRUE);

    SetDlgItemInt(hwnd, ID_IDLE_EDIT, (UINT)g_settings.idleMinutes, FALSE);
    SendMessageW(g_enabledCheck, BM_SETCHECK, g_settings.enabled ? BST_CHECKED : BST_UNCHECKED, 0);
}

static void LayoutControls(HWND hwnd)
{
    RECT rect;
    int margin = 18;
    int leftWidth = 190;
    int rightX;
    int rightWidth;
    int contentWidth;
    int y;

    GetClientRect(hwnd, &rect);
    contentWidth = rect.right - (margin * 2);
    rightX = margin + leftWidth;
    rightWidth = rect.right - rightX - margin;

    MoveWindow(g_detailLabel, margin + 4, 108, contentWidth - 8, 42, TRUE);

    y = 172;
    MoveWindow(g_leftLabels[0], margin, y, leftWidth - 12, 24, TRUE);
    MoveWindow(g_enabledCheck, rightX, y, rightWidth, 24, TRUE);

    y += 34;
    MoveWindow(g_leftLabels[1], margin, y, leftWidth - 12, 24, TRUE);
    MoveWindow(g_idleEdit, rightX, y - 2, 72, 26, TRUE);
    MoveWindow(g_idleSpin, rightX + 72, y - 2, 20, 26, TRUE);
    MoveWindow(g_idleUnitLabel, rightX + 104, y, 40, 24, TRUE);

    y += 34;
    MoveWindow(g_leftLabels[2], margin, y, leftWidth - 12, 24, TRUE);
    MoveWindow(g_stateValue, rightX, y, rightWidth, 24, TRUE);

    y += 34;
    MoveWindow(g_leftLabels[3], margin, y, leftWidth - 12, 24, TRUE);
    MoveWindow(g_idleValue, rightX, y, rightWidth, 24, TRUE);

    y += 34;
    MoveWindow(g_leftLabels[4], margin, y, leftWidth - 12, 24, TRUE);
    MoveWindow(g_lastActionValue, rightX, y, rightWidth, 24, TRUE);

    y += 50;
    MoveWindow(g_minimizeButton, margin, y, 178, 34, TRUE);
    MoveWindow(g_exitButton, margin + 194, y, 84, 34, TRUE);
    MoveWindow(g_noteLabel, margin + 2, y + 56, contentWidth - 4, 44, TRUE);
}

static void ShowTrayMenu(HWND hwnd)
{
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(g_trayMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
}

static LRESULT HandleControlColor(HDC hdc, HWND control)
{
    int controlId = GetDlgCtrlID(control);
    SetBkMode(hdc, TRANSPARENT);

    if (controlId == ID_DETAIL_LABEL || controlId == ID_NOTE_LABEL) {
        SetTextColor(hdc, APP_COLOR_MUTED);
        return (LRESULT)g_backgroundBrush;
    }

    if (controlId == ID_STATE_VALUE) {
        SetTextColor(hdc, g_settings.enabled ? APP_COLOR_AQUA : APP_COLOR_DANGER);
        return (LRESULT)g_backgroundBrush;
    }

    if (controlId == ID_IDLE_VALUE || controlId == ID_LAST_ACTION_VALUE || controlId == ID_IDLE_UNIT) {
        SetTextColor(hdc, APP_COLOR_NAVY);
        return (LRESULT)g_backgroundBrush;
    }

    if (controlId >= ID_LABEL_CONTROL && controlId <= ID_LABEL_LAST_ACTION) {
        SetTextColor(hdc, APP_COLOR_NAVY);
        return (LRESULT)g_backgroundBrush;
    }

    SetTextColor(hdc, APP_COLOR_NAVY);
    return (LRESULT)g_backgroundBrush;
}

static void DrawButton(LPDRAWITEMSTRUCT drawItem)
{
    RECT rect = drawItem->rcItem;
    COLORREF backgroundColor;
    COLORREF borderColor;
    COLORREF textColor;
    HBRUSH fillBrush;
    HPEN borderPen;
    HGDIOBJ oldBrush;
    HGDIOBJ oldPen;
    int oldBkMode;
    wchar_t text[64];

    if (drawItem->CtlID == ID_MINIMIZE_BUTTON) {
        backgroundColor = APP_COLOR_MEDIUM_BLUE;
        borderColor = APP_COLOR_MEDIUM_BLUE;
        textColor = RGB(255, 255, 255);
    } else {
        backgroundColor = APP_COLOR_SURFACE;
        borderColor = APP_COLOR_SKY_BLUE;
        textColor = APP_COLOR_CORE_BLUE;
    }

    if ((drawItem->itemState & ODS_SELECTED) != 0) {
        backgroundColor = RGB(
            max(0, (int)GetRValue(backgroundColor) - 14),
            max(0, (int)GetGValue(backgroundColor) - 14),
            max(0, (int)GetBValue(backgroundColor) - 14));
    }

    fillBrush = CreateSolidBrush(backgroundColor);
    borderPen = CreatePen(PS_SOLID, 1, borderColor);
    oldBrush = SelectObject(drawItem->hDC, fillBrush);
    oldPen = SelectObject(drawItem->hDC, borderPen);
    Rectangle(drawItem->hDC, rect.left, rect.top, rect.right, rect.bottom);

    GetWindowTextW(drawItem->hwndItem, text, sizeof(text) / sizeof(text[0]));
    SetTextColor(drawItem->hDC, textColor);
    oldBkMode = SetBkMode(drawItem->hDC, TRANSPARENT);
    DrawTextW(drawItem->hDC, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SetBkMode(drawItem->hDC, oldBkMode);

    SelectObject(drawItem->hDC, oldBrush);
    SelectObject(drawItem->hDC, oldPen);
    DeleteObject(fillBrush);
    DeleteObject(borderPen);
}

static void PaintWindow(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT clientRect;
    RECT headerRect;
    RECT titleRect;
    RECT subtitleRect;
    HGDIOBJ oldFont;
    int oldBkMode;

    GetClientRect(hwnd, &clientRect);
    FillRect(hdc, &clientRect, g_backgroundBrush);

    headerRect.left = 18;
    headerRect.top = 18;
    headerRect.right = clientRect.right - 18;
    headerRect.bottom = 104;
    FillRect(hdc, &headerRect, g_headerBrush);

    oldBkMode = SetBkMode(hdc, TRANSPARENT);
    oldFont = SelectObject(hdc, g_titleFont);
    SetTextColor(hdc, RGB(255, 255, 255));
    titleRect.left = 32;
    titleRect.top = 28;
    titleRect.right = headerRect.right - 16;
    titleRect.bottom = 58;
    DrawTextW(hdc, APP_NAME, -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, g_subtitleFont);
    SetTextColor(hdc, RGB(220, 255, 255));
    subtitleRect.left = 34;
    subtitleRect.top = 60;
    subtitleRect.right = headerRect.right - 16;
    subtitleRect.bottom = 86;
    DrawTextW(hdc, APP_TAGLINE, -1, &subtitleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
    SetBkMode(hdc, oldBkMode);
    EndPaint(hwnd, &ps);
}

static void InitializeApplicationState(void)
{
    INITCOMMONCONTROLSEX commonControls;

    LoadSettings(&g_settings);
    EnsureIdleMinutesRange();
    g_appIcon = LoadAppIcon();

    ZeroMemory(&commonControls, sizeof(commonControls));
    commonControls.dwSize = sizeof(commonControls);
    commonControls.dwICC = ICC_UPDOWN_CLASS;
    InitCommonControlsEx(&commonControls);
}

static void CleanupApplicationState(void)
{
    if (g_appIcon != NULL) {
        DestroyIcon(g_appIcon);
        g_appIcon = NULL;
    }
    if (g_singleInstanceMutex != NULL) {
        CloseHandle(g_singleInstanceMutex);
        g_singleInstanceMutex = NULL;
    }
}

static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
        g_mainWindow = hwnd;
        CreateControls(hwnd);
        LayoutControls(hwnd);

        g_trayMenu = CreatePopupMenu();
        AppendMenuW(g_trayMenu, MF_STRING, ID_TRAY_OPEN, L"Abrir");
        AppendMenuW(g_trayMenu, MF_STRING, ID_TRAY_TOGGLE, g_settings.enabled ? L"Deshabilitar" : L"Habilitar");
        AppendMenuW(g_trayMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(g_trayMenu, MF_STRING, ID_TRAY_EXIT, L"Salir");

        ZeroMemory(&g_trayIconData, sizeof(g_trayIconData));
        g_trayIconData.cbSize = sizeof(g_trayIconData);
        g_trayIconData.hWnd = hwnd;
        g_trayIconData.uID = 1;
        g_trayIconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        g_trayIconData.uCallbackMessage = WMAPP_TRAYICON;
        g_trayIconData.hIcon = g_appIcon;
        wcsncpy(g_trayIconData.szTip, APP_NAME, (sizeof(g_trayIconData.szTip) / sizeof(g_trayIconData.szTip[0])) - 1);
        Shell_NotifyIconW(NIM_ADD, &g_trayIconData);

        SetTimer(hwnd, ID_TIMER, 1000, NULL);
        SetEnabledState(g_settings.enabled, FALSE);
        UpdateIdleLabel();
        return 0;

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO *minMaxInfo = (MINMAXINFO *)lParam;
        minMaxInfo->ptMinTrackSize.x = WINDOW_MIN_WIDTH;
        minMaxInfo->ptMinTrackSize.y = WINDOW_MIN_HEIGHT;
        return 0;
    }

    case WM_SIZE:
        LayoutControls(hwnd);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_ENABLE_CHECK:
            if (HIWORD(wParam) == BN_CLICKED) {
                SetEnabledState(SendMessageW(g_enabledCheck, BM_GETCHECK, 0, 0) == BST_CHECKED, TRUE);
            }
            return 0;

        case ID_IDLE_EDIT:
            if (HIWORD(wParam) == EN_CHANGE && !g_isUpdatingIdleInput) {
                UpdateIdleThresholdFromInput(TRUE);
            }
            return 0;

        case ID_MINIMIZE_BUTTON:
            HideToTray();
            return 0;

        case ID_EXIT_BUTTON:
        case ID_TRAY_EXIT:
            ExitApplication();
            return 0;

        case ID_TRAY_OPEN:
            RestoreFromTray();
            return 0;

        case ID_TRAY_TOGGLE:
            SetEnabledState(!g_settings.enabled, TRUE);
            return 0;
        }
        break;

    case WM_TIMER:
        if (wParam == ID_TIMER) {
            UpdateIdleLabel();
        }
        return 0;

    case WMAPP_TRAYICON:
        if (lParam == WM_LBUTTONDBLCLK) {
            RestoreFromTray();
        } else if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
            ShowTrayMenu(hwnd);
        }
        return 0;

    case WM_DRAWITEM:
        DrawButton((LPDRAWITEMSTRUCT)lParam);
        return TRUE;

    case WM_CTLCOLORSTATIC:
        return HandleControlColor((HDC)wParam, (HWND)lParam);

    case WM_CTLCOLOREDIT:
        SetBkColor((HDC)wParam, APP_COLOR_SURFACE);
        SetTextColor((HDC)wParam, APP_COLOR_NAVY);
        return (LRESULT)g_surfaceBrush;

    case WM_CTLCOLORBTN:
        SetBkMode((HDC)wParam, TRANSPARENT);
        return (LRESULT)g_backgroundBrush;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        PaintWindow(hwnd);
        return 0;

    case WM_CLOSE:
        if (!g_isExiting && g_settings.minimizeToTrayOnClose) {
            HideToTray();
            return 0;
        }
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER);
        Shell_NotifyIconW(NIM_DELETE, &g_trayIconData);
        if (g_trayMenu != NULL) {
            DestroyMenu(g_trayMenu);
            g_trayMenu = NULL;
        }
        DisableKeepAwake();
        SaveSettings(&g_settings);
        DestroyUiResources();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine, int showCommand)
{
    WNDCLASSW windowClass;
    HWND windowHandle;
    MSG message;

    (void)previousInstance;
    (void)commandLine;

    g_instance = instance;
    g_singleInstanceMutex = CreateMutexW(NULL, TRUE, APP_MUTEX_NAME);
    if (g_singleInstanceMutex == NULL) {
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(NULL, L"BluePulse ya esta en ejecucion.", APP_NAME, MB_OK | MB_ICONINFORMATION);
        CleanupApplicationState();
        return 0;
    }

    InitializeApplicationState();

    ZeroMemory(&windowClass, sizeof(windowClass));
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = L"BluePulseMainWindow";
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = g_backgroundBrush;
    windowClass.hIcon = g_appIcon;

    if (!RegisterClassW(&windowClass)) {
        CleanupApplicationState();
        return 1;
    }

    windowHandle = CreateWindowExW(0, windowClass.lpszClassName, APP_NAME, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, instance, NULL);
    if (windowHandle == NULL) {
        CleanupApplicationState();
        return 1;
    }

    SendMessageW(windowHandle, WM_SETICON, ICON_BIG, (LPARAM)g_appIcon);
    SendMessageW(windowHandle, WM_SETICON, ICON_SMALL, (LPARAM)g_appIcon);

    ShowWindow(windowHandle, showCommand);
    UpdateWindow(windowHandle);

    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    CleanupApplicationState();
    return (int)message.wParam;
}
