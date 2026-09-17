// ColorSettings.cpp
#include "ColorSettings.h"
#include "Globals.h"
#include "APPDef.h"
#include <strsafe.h>

void LoadColorSettings()
{
    wchar_t savedRed[16], savedGreen[16], savedBlue[16];
    GetPrivateProfileStringW(L"Color", L"Red", L"0x00", savedRed, 16, HOMEWORK_INI_PATH);
    GetPrivateProfileStringW(L"Color", L"Green", L"0x00", savedGreen, 16, HOMEWORK_INI_PATH);
    GetPrivateProfileStringW(L"Color", L"Blue", L"0x00", savedBlue, 16, HOMEWORK_INI_PATH);

    SetWindowTextW(hEditRed, savedRed);
    SetWindowTextW(hEditGreen, savedGreen);
    SetWindowTextW(hEditBlue, savedBlue);
}

void SaveColorSettings()
{
    wchar_t redText[16], greenText[16], blueText[16];
    GetWindowTextW(hEditRed, redText, 16);
    GetWindowTextW(hEditGreen, greenText, 16);
    GetWindowTextW(hEditBlue, blueText, 16);

    wchar_t debugMsg[128];
    StringCchPrintfW(debugMsg, 128, L"Apply clicked: R=%s, G=%s, B=%s\n",
        redText, greenText, blueText);
    OutputDebugStringW(debugMsg);

    WritePrivateProfileStringW(L"Color", L"Red", redText, HOMEWORK_INI_PATH);
    WritePrivateProfileStringW(L"Color", L"Green", greenText, HOMEWORK_INI_PATH);
    WritePrivateProfileStringW(L"Color", L"Blue", blueText, HOMEWORK_INI_PATH);

    OutputDebugStringW(L"Settings saved to Homework.ini\n");

    TraceLoggingWrite(
        g_hProvider,
        "ApplyClicked",
        TraceLoggingWideString(redText, "Red"),
        TraceLoggingWideString(greenText, "Green"),
        TraceLoggingWideString(blueText, "Blue")
    );
}