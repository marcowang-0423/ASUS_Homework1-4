// DebugLog.cpp
#include "DebugLog.h"
#include "APPDef.h"
#include <strsafe.h>

void LogLastError(LPCWSTR functionName)
{
    DWORD errorCode = GetLastError();

    LPWSTR messageBuffer = nullptr;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&messageBuffer,
        0,
        nullptr
    );

    wchar_t logMsg[512];
    StringCchPrintfW(logMsg, 512, L"[%s] failed. ErrorCode: %lu, Message: %s",
        functionName, errorCode,
        messageBuffer ? messageBuffer : L"Unknown error");

    OutputDebugStringW(logMsg);
    OutputDebugStringW(L"\n");

    HANDLE hFile = CreateFileW(
        HOMEWORK_LOG_PATH,
        FILE_APPEND_DATA,
        FILE_SHARE_READ,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile != INVALID_HANDLE_VALUE)
    {
        char narrowBuf[512];
        WideCharToMultiByte(CP_UTF8, 0, logMsg, -1, narrowBuf, 512, nullptr, nullptr);

        DWORD written;
        WriteFile(hFile, narrowBuf, (DWORD)strlen(narrowBuf), &written, nullptr);
        WriteFile(hFile, "\r\n", 2, &written, nullptr);

        CloseHandle(hFile);
    }
    else
    {
        OutputDebugStringW(L"寫入 log 檔失敗，可能是權限問題，試試看用系統管理員身分執行 VS\n");
    }

    if (messageBuffer)
    {
        LocalFree(messageBuffer);
    }
}