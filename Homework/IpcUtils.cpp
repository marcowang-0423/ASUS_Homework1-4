// IpcUtils.cpp
#include "IpcUtils.h"
#include "Globals.h"

struct FindWindowParam
{
    HWND hSelf;
    HWND hFound;
};

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    FindWindowParam* p = (FindWindowParam*)lParam;

    if (hwnd == p->hSelf)
        return TRUE;

    wchar_t className[256];
    GetClassNameW(hwnd, className, 256);

    if (wcscmp(className, szWindowClass) == 0)
    {
        p->hFound = hwnd;
        return FALSE;
    }
    return TRUE;
}

HWND FindOtherHomeworkWindow(HWND hSelf)
{
    FindWindowParam param = { hSelf, NULL };
    EnumWindows(EnumWindowsProc, (LPARAM)&param);
    return param.hFound;
}