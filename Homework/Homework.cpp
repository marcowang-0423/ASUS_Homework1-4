// Homework.cpp : 定義應用程式的進入點。
//

#include "framework.h"
#include "Homework.h"
#include <strsafe.h>

#define MAX_LOADSTRING 100

// 全域變數:
HINSTANCE hInst;                                // 目前執行個體
WCHAR szTitle[MAX_LOADSTRING];                  // 標題列文字
WCHAR szWindowClass[MAX_LOADSTRING];            // 主視窗類別名稱

// 這個程式碼模組所包含之函式的向前宣告:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此放置程式碼。

    // 將全域字串初始化
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_HOMEWORK, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 執行應用程式初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_HOMEWORK));

    MSG msg;

    // 主訊息迴圈:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}


// 把最後一次的 API 錯誤碼轉成文字，輸出到 VS 主控台，並寫入 log 檔
void LogLastError(LPCWSTR functionName)
{
    DWORD errorCode = GetLastError();

    // 用 FormatMessage 把數字錯誤碼轉成人看得懂的文字（作業 HINT 1, 2 就是在講這個）
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

    // ---- 需求 1: 輸出到 VS 主控台 ----
    OutputDebugStringW(logMsg);
    OutputDebugStringW(L"\n");

    // ---- 進階需求: 寫入 log 檔 C:\ProgramData\Homework.log ----
    HANDLE hFile = CreateFileW(
        L"C:\\ProgramData\\Homework.log",
        FILE_APPEND_DATA,
        FILE_SHARE_READ,
        nullptr,
        OPEN_ALWAYS,          // 檔案不存在就建立，存在就從尾端接著寫
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



//
//  函式: MyRegisterClass()
//
//  用途: 註冊視窗類別。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HOMEWORK));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_HOMEWORK);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函式: InitInstance(HINSTANCE, int)
//
//   用途: 儲存執行個體控制代碼並且建立主視窗
//
//   註解:
//
//        在這個函式中，我們將執行個體控制代碼儲存在全域變數中，
//        並建立及顯示主程式視窗。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 將執行個體控制代碼儲存在全域變數中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  函式: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  用途: 處理主視窗的訊息。
//
//  WM_COMMAND  - 處理應用程式功能表
//  WM_PAINT    - 繪製主視窗
//  WM_DESTROY  - 張貼結束訊息然後傳回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        {
            HWND hButton = CreateWindowW(
                L"BUTTON",              // 控制項類型
                L"BUTTON",            // Button 上顯示的文字
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                100, 100,               // x, y
                120, 40,                // width, height
                hWnd,                   // parent window
                (HMENU)1,               // Button ID
                hInst,
                nullptr
            );

            // HW1 Debug message 
            HWND hFail = CreateWindowW(NULL, NULL, NULL, NULL, NULL,
                NULL, NULL, NULL, NULL, NULL, NULL);
            if (hFail == NULL)
            {
                LogLastError(L"CreateWindowW");
            }

            
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 剖析功能表選取項目:
            switch (wmId)
            {
            case 1:
                MessageBoxW(
                    hWnd,
                    L"Context",
                    L"Title",
                    MB_OK
                );
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;

        if (pDIS->CtlID == 1)   // 對應建立按鈕時給的 (HMENU)1
        {
            // 背景填黑
            HBRUSH hBrushBlack = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(pDIS->hDC, &pDIS->rcItem, hBrushBlack);
            DeleteObject(hBrushBlack);

            // 取得按鈕文字
            wchar_t text[256];
            GetWindowTextW(pDIS->hwndItem, text, 256);

            // 白字
            SetTextColor(pDIS->hDC, RGB(255, 255, 255));
            SetBkMode(pDIS->hDC, TRANSPARENT);
            DrawTextW(pDIS->hDC, text, -1, &pDIS->rcItem,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // 按下時畫白色外框，做出「按下去」的視覺回饋
            if (pDIS->itemState & ODS_SELECTED)
            {
                FrameRect(pDIS->hDC, &pDIS->rcItem, (HBRUSH)GetStockObject(WHITE_BRUSH));
            }

            // 取得焦點時畫虛線框
            if (pDIS->itemState & ODS_FOCUS)
            {
                DrawFocusRect(pDIS->hDC, &pDIS->rcItem);
            }

            return TRUE;   // 一定要回傳 TRUE，表示你自己處理完繪製了
        }
    }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此新增任何使用 hdc 的繪圖程式碼...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// [關於] 方塊的訊息處理常式。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
