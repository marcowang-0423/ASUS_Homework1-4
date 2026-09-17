// IpcUtils.h
#pragma once
#include <windows.h>

// 找到另一個執行中的 Homework 視窗（自動排除自己），找不到回傳 NULL
HWND FindOtherHomeworkWindow(HWND hSelf);