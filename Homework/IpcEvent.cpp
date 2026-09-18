#include "IpcEvent.h"
#include "APPDef.h"
#include "Globals.h"
#include "DebugLog.h"
#include <strsafe.h>

DWORD WINAPI WaitEventThread(LPVOID lpParam)
{
    HWND hMainWnd = (HWND)lpParam;

    while (true)
    {
        DWORD waitResult = WaitForSingleObject(hEvent, INFINITE);
        if (waitResult == WAIT_OBJECT_0)
        {
            PostMessageW(hMainWnd, WM_APP_EVENT_TRIGGERED, 0, 0);
        }
    }

    return 0;
}

void RaiseOtherEvent(HWND hTarget)
{
    DWORD targetPid = 0;
    GetWindowThreadProcessId(hTarget, &targetPid);

    WCHAR targetEventName[64];
    StringCchPrintfW(targetEventName, 64, HOMEWORK_EVENT_NAME_FMT, targetPid);

    HANDLE hTargetEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, targetEventName);
    if (hTargetEvent)
    {
        SetEvent(hTargetEvent);
        CloseHandle(hTargetEvent);
    }
    else
    {
        LogLastError(L"OpenEventW");
    }
}