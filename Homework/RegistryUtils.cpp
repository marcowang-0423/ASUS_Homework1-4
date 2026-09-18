#include "RegistryUtils.h"
#include <strsafe.h>

#pragma comment(lib, "advapi32.lib")   // Reg* 系列函式住在 advapi32.dll

void WriteHomeworkValue(HWND hWnd)
{
    HKEY hKey;
    LONG result = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\ASUS\\Homework",
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        NULL,
        &hKey,
        NULL);

    if (result == ERROR_SUCCESS)
    {
        const wchar_t* value = L"Hello from Homework";
        RegSetValueExW(
            hKey,
            L"TestValue",
            0,
            REG_SZ,
            (const BYTE*)value,
            (DWORD)((wcslen(value) + 1) * sizeof(wchar_t)));

        RegCloseKey(hKey);
        MessageBoxW(hWnd, L"寫入成功", L"Write here", MB_OK);
    }
    else
    {
        wchar_t msg[128];
        StringCchPrintfW(
            msg, 128,
            L"RegCreateKeyExW 失敗，error = %ld", result);
        MessageBoxW(hWnd, msg, L"Write here", MB_ICONERROR);
    }
}

void ReadOOBEStatus(HWND hWnd)
{
    HKEY hKey;
    LONG openResult = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\ASUS\\ASUS System Control Interface"
        L"\\AsusSoftwareManager",
        0,
        KEY_READ,
        &hKey);

    if (openResult != ERROR_SUCCESS)
    {
        wchar_t msg[128];
        StringCchPrintfW(
            msg, 128,
            L"RegOpenKeyExW 失敗，error = %ld", openResult);
        MessageBoxW(hWnd, msg, L"Read here", MB_ICONERROR);
        return;
    }

    DWORD type = 0;
    DWORD dataSize = 0;

    LONG queryResult = RegQueryValueExW(
        hKey, L"OOBEStatus", NULL,
        &type, NULL, &dataSize);

    if (queryResult == ERROR_SUCCESS)
    {
        BYTE* buffer = new BYTE[dataSize];

        RegQueryValueExW(
            hKey, L"OOBEStatus", NULL,
            &type, buffer, &dataSize);

        wchar_t msg[128];
        if (type == REG_DWORD)
        {
            DWORD value = *(DWORD*)buffer;
            StringCchPrintfW(
                msg, 128, L"OOBEStatus = %lu", value);
        }
        else if (type == REG_SZ)
        {
            StringCchPrintfW(
                msg, 128, L"OOBEStatus = %s",
                (wchar_t*)buffer);
        }

        MessageBoxW(hWnd, msg, L"Read here", MB_OK);
        delete[] buffer;
    }
    else
    {
        wchar_t msg[128];
        StringCchPrintfW(
            msg, 128,
            L"RegQueryValueExW 失敗，error = %ld", queryResult);
        MessageBoxW(hWnd, msg, L"Read here", MB_ICONERROR);
    }

    RegCloseKey(hKey);
}