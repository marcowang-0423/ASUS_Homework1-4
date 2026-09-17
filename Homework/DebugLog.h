// DebugLog.h
#pragma once
#include <windows.h>

// 把最後一次的 API 錯誤碼轉成文字，輸出到 VS 主控台，並寫入 log 檔
void LogLastError(LPCWSTR functionName);
