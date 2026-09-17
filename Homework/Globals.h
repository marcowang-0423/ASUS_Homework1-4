// Globals.h
#pragma once
#include <windows.h>
#include <TraceLoggingProvider.h>

// szWindowClass 實際上是在 Homework.cpp 裡定義的，
// 這裡只是宣告「這個變數存在，連結時去別的檔案找」
extern WCHAR szWindowClass[]; 

// HW2
extern HWND hEditRed, hEditGreen, hEditBlue;
TRACELOGGING_DECLARE_PROVIDER(g_hProvider);
