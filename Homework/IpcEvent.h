#pragma once
#include <windows.h>

DWORD WINAPI WaitEventThread(LPVOID lpParam);
void RaiseOtherEvent(HWND hTarget);