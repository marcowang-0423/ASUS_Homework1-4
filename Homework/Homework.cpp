// Homework.cpp : 定義應用程式的進入點。
//

#include "framework.h"
#include "Homework.h"
#include "APPDef.h"
#include "DebugLog.h"
#include "ColorSettings.h"
#include "IpcUtils.h"
#include "IpcEvent.h"
#include "RegistryUtils.h"
#include "WmiUtils.h"
#include "TaskSchedulerUtils.h"
#include <strsafe.h>
#include <TraceLoggingProvider.h>
#pragma comment(lib, "advapi32.lib")

#define MAX_LOADSTRING 100


  

// 全域變數:
HINSTANCE hInst;                                // 目前執行個體
WCHAR szTitle[MAX_LOADSTRING];                  // 標題列文字
WCHAR szWindowClass[MAX_LOADSTRING];            // 主視窗類別名稱

HWND hEditRed, hEditGreen, hEditBlue;           // 存輸入框的 handle
HWND hEdit, hBtnSend, hBtnEvent;                // HW3 IPC

HANDLE hEvent = NULL;       // Processevent handle
WCHAR  g_myEventName[64];   // Event name



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


// TraceView
TRACELOGGING_DECLARE_PROVIDER(g_hProvider);

TRACELOGGING_DEFINE_PROVIDER(
    g_hProvider,
    "HomeworkProvider",   // Provider 名字
    (0x23b2e3de, 0xe5bd, 0x4c1d, 0x99, 0xe0, 0x40, 0xdd, 0xb3, 0x20, 0x63, 0x4a)  // 換成你剛剛產生的 GUID
); // 23b2e3de-e5bd-4c1d-99e0-40ddb320634a






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
                20, 20,               // x, y
                120, 40,                // width, height
                hWnd,                   // parent window
                (HMENU)1,               // Button ID
                hInst,
                nullptr
            );

            //// HW1 Debug message 
            //HWND hFail = CreateWindowW(NULL, NULL, NULL, NULL, NULL,
            //    NULL, NULL, NULL, NULL, NULL, NULL);
            //if (hFail == NULL)
            //{
            //    LogLastError(L"CreateWindowW");
            //}

            //TraceLoggingRegister(g_hProvider);



            // "Simple >>" 標籤（先做成單純的靜態文字，不做展開/收合功能）
            CreateWindowW(
                L"STATIC", L"Simple >>",
                WS_VISIBLE | WS_CHILD,
                20, 70, 80, 20,
                hWnd, nullptr, hInst, nullptr
            );

            // ---- Red ----
            CreateWindowW(
                L"STATIC", L"Red",
                WS_VISIBLE | WS_CHILD,
                20, 110, 40, 20,
                hWnd, nullptr, hInst, nullptr
            );
            hEditRed = CreateWindowW(
                L"EDIT", L"0x00",
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
                70, 108, 60, 22,
                hWnd, (HMENU)IDC_EDIT_RED, hInst, nullptr
            );

            // ---- Green ----
            CreateWindowW(
                L"STATIC", L"Green",
                WS_VISIBLE | WS_CHILD,
                160, 110, 40, 20,
                hWnd, nullptr, hInst, nullptr
            );
            hEditGreen = CreateWindowW(
                L"EDIT", L"0x00",
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
                210, 108, 60, 22,
                hWnd, (HMENU)IDC_EDIT_GREEN, hInst, nullptr
            );

            // ---- Blue ----
            CreateWindowW(
                L"STATIC", L"Blue",
                WS_VISIBLE | WS_CHILD,
                300, 110, 40, 20,
                hWnd, nullptr, hInst, nullptr
            );
            hEditBlue = CreateWindowW(
                L"EDIT", L"0x00",
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
                350, 108, 60, 22,
                hWnd, (HMENU)IDC_EDIT_BLUE, hInst, nullptr
            );

            // ---- Apply 按鈕 ----
            CreateWindowW(
                L"BUTTON", L"Apply",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                20, 150, 80, 40,
                hWnd, (HMENU)IDC_BUTTON_APPLY, hInst, nullptr
            );

            // 讀取 ini 檔，把上次存的值填回輸入框；如果 ini 檔不存在或沒有這個 key，就用預設值 "0x00"
            LoadColorSettings();

            //HW3 IPC send message
            hEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
                20, 250, 200, 25, hWnd, (HMENU)ID_EDIT_TEXT, hInst, NULL);


            hBtnSend = CreateWindowW(L"BUTTON", L"Send message", WS_CHILD | WS_VISIBLE,
                20, 290, 150, 40, hWnd, (HMENU)ID_BTN_SEND, hInst, NULL);    

            hBtnEvent = CreateWindowW(L"BUTTON", L"Raise event", WS_CHILD | WS_VISIBLE,
                130, 150, 150, 40, hWnd, (HMENU)ID_BTN_EVENT, hInst, NULL);  


            //HW3 Raise event

            DWORD myPid = GetCurrentProcessId();
            StringCchPrintfW(g_myEventName, 64, HOMEWORK_EVENT_NAME_FMT, myPid);

            hEvent = CreateEventW(
                NULL,             // 預設安全性，不給子行程繼承
                FALSE,            // auto-reset
                FALSE,            // 初始狀態：non-signaled（還沒人觸發過）
                g_myEventName );  // 自己專屬的名字，例如 "HomeworkSyncEvent_12345"

            if (hEvent == NULL)
            {
                LogLastError(L"CreateEventW");
            }

            CreateThread(NULL, 0, WaitEventThread, hWnd, 0, NULL);

            //// HW4 Registry
            //WriteHomeworkValue(hWnd);
            //ReadOOBEStatus(hWnd);

            //// HW4 WMI：讀取 BIOS 資訊與 ATK 驅動版本
            //ReadBiosInfo(hWnd);
            //ReadAtkDriverVersion(hWnd);

            //// HW4 Register the task
            RegisterLogonTask(hWnd);

            
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 剖析功能表選取項目:
            switch (wmId)
            {

            case IDC_BUTTON_APPLY:
                SaveColorSettings();
                break;
            case ID_BTN_SEND:
                {
                    // 找對方視窗
                    HWND hTarget = FindOtherHomeworkWindow(hWnd);
                    if (hTarget)
                    {
                        // 把 Edit 欄位內容包成 COPYDATASTRUCT
                        wchar_t buf[256];
                        GetWindowTextW(hEdit, buf, 256);

                        // Send struct
                        //MyData data;
                        //data.number = 42;          // demo 用的數字
                        //wcscpy_s(data.text, buf);  // 把 Edit 欄位的文字塞進 struct
                        //data.flag = true;          // demo 用的 bool

                        //COPYDATASTRUCT cds;
                        //cds.dwData = COPYDATA_STRUCT_MSG;   // 注意：識別碼換成 struct 那個
                        //cds.cbData = sizeof(MyData);        // 大小直接用 sizeof，不用自己算 byte 數
                        //cds.lpData = &data;

                        COPYDATASTRUCT cds;
                        cds.dwData = COPYDATA_TEXT_MSG;
                        cds.cbData = (DWORD)((wcslen(buf) + 1) * sizeof(wchar_t));
                        cds.lpData = buf;

                        // 送出
                        SendMessageW(hTarget, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&cds);
                    }
                    else
                    {
                        MessageBoxW(hWnd, L"找不到另一個執行中的 Homework", L"提示", MB_OK);
                    }
                }
                break;
            case ID_BTN_EVENT:
                {
                    HWND hTarget = FindOtherHomeworkWindow(hWnd);
                    if (hTarget)
                    {
                        SaveColorSettings();       // 1. 先把目前設定寫進 ini
                        RaiseOtherEvent(hTarget);  // 2. 再通知對方「可以來讀了」
                    }
                    else
                    {
                        MessageBoxW(hWnd, L"找不到另一個執行中的 Homework", L"提示", MB_OK);
                    }
                }
                break;
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
    case WM_COPYDATA:
        {
            PCOPYDATASTRUCT pCDS = (PCOPYDATASTRUCT)lParam;
            if (pCDS->dwData == COPYDATA_TEXT_MSG)
            {
                wchar_t* pText = (wchar_t*)pCDS->lpData;
                MessageBoxW(hWnd, pText, L"收到訊息", MB_OK);
            }
            else if (pCDS->dwData == COPYDATA_STRUCT_MSG)
            {
                MyData* pData = (MyData*)pCDS->lpData;

                wchar_t msg[256];
                StringCchPrintfW(msg, 256, L"number = %d\ntext = %s\nflag = %s",
                    pData->number, pData->text, pData->flag ? L"true" : L"false");

                MessageBoxW(hWnd, msg, L"收到 struct", MB_OK);
            }
            return TRUE;
        }
    case WM_APP_EVENT_TRIGGERED:
        LoadColorSettings();
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
        TraceLoggingUnregister(g_hProvider);
        PostQuitMessage(0);
        CloseHandle(hEvent);
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
