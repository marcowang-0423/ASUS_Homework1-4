#include "WmiUtils.h"
#include <comdef.h>
#include <Wbemidl.h>
#include <strsafe.h>

#pragma comment(lib, "wbemuuid.lib")

void ReadBiosInfo(HWND hWnd)
{
    HRESULT hr;

    // 1. 初始化這個執行緒的 COM 環境
    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"CoInitializeEx 失敗", L"WMI Error", MB_ICONERROR);
        return;
    }

    // 2. 設定 process 預設安全層級（整個 process 只會生效第一次）
    hr = CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL);

    if (FAILED(hr) && hr != RPC_E_TOO_LATE)
    {
        MessageBoxW(hWnd, L"CoInitializeSecurity 失敗", L"WMI Error", MB_ICONERROR);
        CoUninitialize();
        return;
    }

    // 3. 建立 WbemLocator，用來連到指定的 WMI namespace
    IWbemLocator* pLoc = NULL;
    hr = CoCreateInstance(
        CLSID_WbemLocator, 0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"CoCreateInstance(WbemLocator) 失敗", L"WMI Error", MB_ICONERROR);
        CoUninitialize();
        return;
    }

    // 4. 連到 ROOT\CIMV2（Win32_BIOS 所在的 namespace）
    IWbemServices* pSvc = NULL;
    hr = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL, NULL, 0, NULL, 0, 0, &pSvc);

    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"ConnectServer 失敗", L"WMI Error", MB_ICONERROR);
        pLoc->Release();
        CoUninitialize();
        return;
    }

    // 5. 針對這條連線設定認證層級
    hr = CoSetProxyBlanket(
        pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE);

    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"CoSetProxyBlanket 失敗", L"WMI Error", MB_ICONERROR);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return;
    }

    // 6. 送出 WQL 查詢
    IEnumWbemClassObject* pEnumerator = NULL;
    hr = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT Manufacturer, SMBIOSBIOSVersion, ReleaseDate "
            "FROM Win32_BIOS"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &pEnumerator);

    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"ExecQuery 失敗", L"WMI Error", MB_ICONERROR);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return;
    }

    // 7. 取出第一筆（也是唯一一筆）結果
    IWbemClassObject* pClassObj = NULL;
    ULONG uReturn = 0;
    wchar_t msg[256] = L"";

    hr = pEnumerator->Next(WBEM_INFINITE, 1, &pClassObj, &uReturn);

    if (uReturn != 0)
    {
        VARIANT vtManufacturer, vtVersion, vtDate;

        pClassObj->Get(L"Manufacturer", 0, &vtManufacturer, 0, 0);
        pClassObj->Get(L"SMBIOSBIOSVersion", 0, &vtVersion, 0, 0);
        pClassObj->Get(L"ReleaseDate", 0, &vtDate, 0, 0);

        StringCchPrintfW(
            msg, 256,
            L"Manufacturer: %s\nBIOS Version: %s\nRelease Date: %s",
            vtManufacturer.bstrVal, vtVersion.bstrVal, vtDate.bstrVal);

        VariantClear(&vtManufacturer);
        VariantClear(&vtVersion);
        VariantClear(&vtDate);
        pClassObj->Release();
    }
    else
    {
        StringCchCopyW(msg, 256, L"查不到 Win32_BIOS 的資料");
    }

    MessageBoxW(hWnd, msg, L"BIOS Info", MB_OK);

    // 8. 收尾
    pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
}

void ReadAtkDriverVersion(HWND hWnd)
{
    HRESULT hr;

    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"CoInitializeEx 失敗", L"WMI Error", MB_ICONERROR);
        return;
    }

    hr = CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL);

    if (FAILED(hr) && hr != RPC_E_TOO_LATE)
    {
        MessageBoxW(hWnd, L"CoInitializeSecurity 失敗", L"WMI Error", MB_ICONERROR);
        CoUninitialize();
        return;
    }

    IWbemLocator* pLoc = NULL;
    hr = CoCreateInstance(
        CLSID_WbemLocator, 0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"CoCreateInstance(WbemLocator) 失敗", L"WMI Error", MB_ICONERROR);
        CoUninitialize();
        return;
    }

    IWbemServices* pSvc = NULL;
    hr = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL, NULL, 0, NULL, 0, 0, &pSvc);

    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"ConnectServer 失敗", L"WMI Error", MB_ICONERROR);
        pLoc->Release();
        CoUninitialize();
        return;
    }

    hr = CoSetProxyBlanket(
        pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE);

    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"CoSetProxyBlanket 失敗", L"WMI Error", MB_ICONERROR);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return;
    }

    // 用 LIKE 篩出裝置名稱含有 "ASUS System Control Interface" 的驅動
    IEnumWbemClassObject* pEnumerator = NULL;
    hr = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT DeviceName, DriverVersion, DriverDate "
            "FROM Win32_PnPSignedDriver "
            "WHERE DeviceName LIKE \"%ASUS System Control Interface%\""),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &pEnumerator);

    if (FAILED(hr))
    {
        MessageBoxW(hWnd, L"ExecQuery 失敗", L"WMI Error", MB_ICONERROR);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return;
    }

    IWbemClassObject* pClassObj = NULL;
    ULONG uReturn = 0;
    wchar_t msg[256] = L"";

    hr = pEnumerator->Next(WBEM_INFINITE, 1, &pClassObj, &uReturn);

    if (uReturn != 0)
    {
        VARIANT vtName, vtVersion, vtDate;

        pClassObj->Get(L"DeviceName", 0, &vtName, 0, 0);
        pClassObj->Get(L"DriverVersion", 0, &vtVersion, 0, 0);
        pClassObj->Get(L"DriverDate", 0, &vtDate, 0, 0);

        StringCchPrintfW(
            msg, 256,
            L"Device: %s\nDriver Version: %s\nDriver Date: %s",
            vtName.bstrVal, vtVersion.bstrVal, vtDate.bstrVal);

        VariantClear(&vtName);
        VariantClear(&vtVersion);
        VariantClear(&vtDate);
        pClassObj->Release();
    }
    else
    {
        StringCchCopyW(msg, 256, L"查不到 ASUS System Control Interface 的驅動資訊");
    }

    MessageBoxW(hWnd, msg, L"ATK Driver Info", MB_OK);

    pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
}