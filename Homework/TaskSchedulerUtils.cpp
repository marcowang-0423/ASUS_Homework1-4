#include "TaskSchedulerUtils.h"
#include <comdef.h>
#include <taskschd.h>
#include <strsafe.h>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

void RegisterLogonTask(HWND hWnd)
{
    HRESULT hr;

    // Step1：初始化 COM
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"CoInitializeEx 失敗", L"Task Scheduler Error", MB_ICONERROR);
        return;
    }

    hr = CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, 0, NULL);

    if (FAILED(hr) && hr != RPC_E_TOO_LATE)
    {
        MessageBoxW(hWnd, L"CoInitializeSecurity 失敗", L"Task Scheduler Error", MB_ICONERROR);
        CoUninitialize();
        return;
    }

    // Step2：建立 ITaskService 並連線
    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL,
        CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);

    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"CoCreateInstance(TaskScheduler) 失敗", L"Task Scheduler Error", MB_ICONERROR);
        CoUninitialize();
        return;
    }

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"ITaskService::Connect \u5931\u6557", L"Task Scheduler Error", MB_ICONERROR);
        pService->Release();
        CoUninitialize();
        return;
    }

    // Step3：拿到根目錄的 Task Folder
    ITaskFolder* pRootFolder = NULL;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"GetFolder 失敗", L"Task Scheduler Error", MB_ICONERROR);
        pService->Release();
        CoUninitialize();
        return;
    }

    const wchar_t* wszTaskName = L"Homework Logon Trigger Task";

    // 如果同名工作已存在，先刪除，避免重複註冊時噴錯
    pRootFolder->DeleteTask(_bstr_t(wszTaskName), 0);

    // Step4：建立 Task 定義，設定作者跟 Settings
    ITaskDefinition* pTask = NULL;
    hr = pService->NewTask(0, &pTask);
    pService->Release();

    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"NewTask 失敗", L"Task Scheduler Error", MB_ICONERROR);
        pRootFolder->Release();
        CoUninitialize();
        return;
    }

    IRegistrationInfo* pRegInfo = NULL;
    if (SUCCEEDED(pTask->get_RegistrationInfo(&pRegInfo)))
    {
        pRegInfo->put_Author(_bstr_t(L"Marco2_Wang"));
        pRegInfo->Release();
    }

    ITaskSettings* pSettings = NULL;
    if (SUCCEEDED(pTask->get_Settings(&pSettings)))
    {
        pSettings->put_StartWhenAvailable(VARIANT_TRUE);
        pSettings->Release();
    }

    // Step5：加入 Logon Trigger（登入時觸發）
    ITriggerCollection* pTriggerCollection = NULL;
    hr = pTask->get_Triggers(&pTriggerCollection);
    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"get_Triggers 失敗", L"Task Scheduler Error", MB_ICONERROR);
        pTask->Release();
        pRootFolder->Release();
        CoUninitialize();
        return;
    }

    // 先跟 collection 要一個型別是
    // TASK_TRIGGER_LOGON 的 ITrigger，才有東西可以 QueryInterface
    ITrigger* pTrigger = NULL;
    hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
    pTriggerCollection->Release();

    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"Create Trigger 失敗", L"Task Scheduler Error", MB_ICONERROR);
        pTask->Release();
        pRootFolder->Release();
        CoUninitialize();
        return;
    }

    ILogonTrigger* pLogonTrigger = NULL;
    if (SUCCEEDED(pTrigger->QueryInterface(IID_ILogonTrigger, (void**)&pLogonTrigger)))
    {
        pLogonTrigger->put_Id(_bstr_t(L"Trigger1"));
        pLogonTrigger->Release();
    }
    pTrigger->Release();

    // Step6：加入 Action（登入後要執行的程式：notepad.exe）
    IActionCollection* pActionCollection = NULL;
    hr = pTask->get_Actions(&pActionCollection);
    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"get_Actions 失敗", L"Task Scheduler Error", MB_ICONERROR);
        pTask->Release();
        pRootFolder->Release();
        CoUninitialize();
        return;
    }

    IAction* pAction = NULL;
    hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
    pActionCollection->Release();

    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"Create Action 失敗", L"Task Scheduler Error", MB_ICONERROR);
        pTask->Release();
        pRootFolder->Release();
        CoUninitialize();
        return;
    }

    IExecAction* pExecAction = NULL;
    if (SUCCEEDED(pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction)))
    {
        pExecAction->put_Path(_bstr_t(L"C:\\Windows\\System32\\notepad.exe"));
        pExecAction->Release();
    }
    pAction->Release();

    // Step7：把整份 Task 定義註冊（存檔）到 Task Scheduler
    IRegisteredTask* pRegisteredTask = NULL;
    hr = pRootFolder->RegisterTaskDefinition(
        _bstr_t(wszTaskName),
        pTask,
        TASK_CREATE_OR_UPDATE,
        _variant_t(L"Builtin\\Administrators"),
        _variant_t(),
        TASK_LOGON_GROUP,
        _variant_t(L""),
        &pRegisteredTask);

    if (SUCCEEDED(hr))
    {
        MessageBoxW(hWnd, L"工作已成功註冊到 Task Scheduler", L"Task Scheduler", MB_OK);
        pRegisteredTask->Release();
    }
    else
    {
        wchar_t msg[128];
        StringCchPrintfW(msg, 128, L"RegisterTaskDefinition 失敗，hr = 0x%08X", hr);
        MessageBoxW(hWnd, msg, L"Task Scheduler Error", MB_ICONERROR);
    }

    pTask->Release();
    pRootFolder->Release();
    CoUninitialize();
}