#pragma once

// =====================================================
// 控制項 ID
// =====================================================

// ---- HW1：顏色設定 ----
#define IDC_EDIT_RED      1001
#define IDC_EDIT_GREEN    1002
#define IDC_EDIT_BLUE     1003
#define IDC_BUTTON_APPLY  1004

// ---- HW3：IPC ----
#define ID_EDIT_TEXT      1005
#define ID_BTN_SEND       1006
#define ID_BTN_EVENT      1007

// =====================================================
// WM_COPYDATA 用的識別碼（dwData）
// =====================================================
#define COPYDATA_TEXT_MSG    1   // 純文字訊息（HW3 基本題）
#define COPYDATA_STRUCT_MSG  2   // struct 訊息（HW3 進階題，int/WCHAR/bool）

// =====================================================
// 共用檔案路徑
// =====================================================
#define HOMEWORK_INI_PATH  L"C:\\ProgramData\\Homework.ini"
#define HOMEWORK_LOG_PATH  L"C:\\ProgramData\\Homework.log"