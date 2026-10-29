---
name: cpp-code-review-guidelines
description: "C++ code review checklist for reviewing C/C++ diffs, PRs, or commits. Checks correctness, memory safety, object lifetime, threading, untrusted input, performance, error handling, and maintainability. Use when reviewing .c, .cc, .cpp, .cxx, .h, .hpp files."
---

# Cpp Code Review 指南（僅針對 Code Diff）

## 範圍

本次 review **只關注 PR / diff 中新增或修改的程式碼**。

請遵守以下原則：

- 只評論 **新增或修改的程式碼**
- 不評論未改動的程式碼
- 除非 diff 造成潛在問題，否則不要討論舊程式碼
- 重點在 **變更造成的影響**

---

# Review 優先順序

審查時請依照以下優先順序：

1. 正確性 (Correctness)
2. 安全性 (Safety) — 記憶體、物件生命週期、不可信任輸入
3. 多執行緒與同步 (Concurrency)
4. 錯誤處理 (Error Handling)
5. 效能 (Performance)
6. 可維護性 (Maintainability)

優先關注 **邏輯問題與潛在 bug**，而不是單純的程式風格問題。

## 嚴重度原則

- **High**：在正式環境中可能造成 crash、資料損毀、安全漏洞或資源洩漏。
- **Medium**：特定條件下行為錯誤、錯誤被悄悄忽略、可能造成間歇性問題的 thread-safety 疑慮。
- **Low**：風格、可讀性、功能正確但不慣用的寫法。

每個 High / Medium finding 都必須能寫出具體的 **Trigger**（什麼輸入或狀態 → 什麼錯誤結果）。寫不出來的，降級或移除。

---

# Diff Review 檢查清單

## 1. 正確性 (Correctness)

確認本次修改沒有引入邏輯錯誤。

檢查：

- 邏輯是否正確
- 是否有遺漏 edge case
- 是否錯誤使用 API
- 是否可能破壞原本行為

建議思考：

- 這個修改是否會影響現有功能？
- 是否有未處理的條件？

**Severity thresholds:**
- **High**: null dereference possible, wrong cast type, **unintentional** fall-through in `switch` (intentional fall-through with a comment or `[[fallthrough]]` is fine), logic inversion
- **Medium**: edge case not handled, API misuse that may fail silently, incorrect enum/flag value

---

## 2. 記憶體安全 (Memory Safety)

特別是 C/C++ 程式碼，需檢查：

- 記憶體洩漏
- double free
- dangling pointer / use-after-free
- 不安全的 pointer 使用
- buffer 長度：`memcpy`、`wcscpy`、`strcpy`、`sprintf`、`wcscat` 的目的 buffer 是否足夠；優先使用 `_s` 版本並傳入 `_countof()`

優先建議使用：

- RAII
- smart pointer (`std::unique_ptr`, `std::shared_ptr`)，COM 物件使用 `CComPtr`

**Severity thresholds:**
- **High**: memory leak on every execution path, double free, use-after-free, dangling pointer dereference, buffer overflow
- **Medium**: memory leak on error path only, raw `new` without matching `delete` in non-RAII context, unsafe cast that may truncate data

---

## 3. 物件生命週期與非同步執行 (Object Lifetime)

本專案大量使用 `CreateThread` / `std::thread`、callback 與 timer。物件被銷毀時，仍在執行或排隊中的 thread / callback 會存取已釋放的記憶體。

檢查：

- 物件的解構 / `Stop()` / `Release()` 是否先 **停止並等待** 自己建立的 worker thread（`WaitForSingleObject` / `join`），再釋放成員
- 傳給 thread / callback 的 `this` 或 context pointer，在 callback 執行時是否一定還活著
- 註冊的 callback 是否在物件銷毀前反註冊
- Win32 handle（thread、event、file、mapping）是否在所有路徑上 `CloseHandle`
- COM 物件的 `AddRef` / `Release` 是否成對；是否混用 raw pointer 與 `CComPtr` 造成多 / 少 Release

**Severity thresholds:**
- **High**: object destroyed while its thread/callback/timer can still run (use-after-free), callback context freed before unregistration, COM over-`Release`
- **Medium**: handle leak on error path, handle leaked on every call to an infrequent function, thread not joined on shutdown (may hang or crash at exit)

---

## 4. 不可信任輸入 (Untrusted Input)

來自手機、網路封包、RPC、共享記憶體、檔案、registry 的資料都視為不可信任。（driver 的 IOCTL 驗證由 `virtual-driver-review-guidelines` 負責。）

檢查：

- 封包 / 訊息中的 **長度、數量、offset 欄位** 是否在使用前驗證上限，並和實際收到的 buffer 大小比較
- 長度計算是否可能 **integer overflow**（`count * size`、`offset + len`）或 **有號 / 無號混用**（負值轉成巨大的 `size_t`）
- 字串是否保證 null-terminated 才當成 C string 使用
- 外部提供的路徑是否可能穿越目錄（`..\`）
- 外部字串是否被拼進命令列、format string（`printf(userStr)`）

**Severity thresholds:**
- **High**: externally controlled length/offset used for copy or indexing without bound check, integer overflow in allocation size, format string from external input
- **Medium**: missing validation that causes wrong behavior but not memory corruption (e.g. unchecked enum value from peer falls into a default branch incorrectly)

---

## 5. 多執行緒與同步 (Concurrency)

若 diff 涉及多執行緒：

檢查：

- race condition
- deadlock
- 缺少同步機制
- shared state 是否安全

思考：

- 新增的程式碼是否破壞 thread safety？

**Severity thresholds:**
- **High**: shared state accessed without lock, `EnterCriticalSection` without matching `LeaveCriticalSection` on all paths (including early returns), TOCTOU race
- **Medium**: lock held across blocking call, potential deadlock due to lock ordering, missing synchronization on infrequent path

---

## 6. 錯誤處理 (Error Handling)

確認錯誤情況有被正確處理。

檢查：

- return value 是否被檢查
- 是否忽略錯誤
- failure path 是否完整

**Severity thresholds:**
- **High**: unchecked `HRESULT` whose result is then used (possible null / invalid object), ignored failure that leads to crash later
- **Medium**: unchecked `HRESULT` where the result is not used afterwards, missing error check on file/network operation, failure path leaks resource

`CreateThread` / `CreateEvent` 回傳值與 `HRESULT` 的專案規則以 `PROJECT-MAP.md` §Code Review 的 high-risk patterns 文件為準。

---

## 7. 效能影響 (Performance)

評估這次修改是否影響效能。

檢查：

- 不必要的記憶體配置
- loop 內昂貴操作
- 不必要的 copy
- 阻塞操作出現在 hot path

**Severity thresholds:**
- **High**: O(n²) or worse in hot path, blocking call on UI/render thread
- **Medium**: unnecessary allocation per frame in render/streaming loop, large object copied instead of moved/referenced

---

## 8. 可維護性 (Maintainability)

確認新程式碼未增加維護成本。

檢查：

- 命名是否清楚
- 函式是否過長
- 是否出現重複邏輯

避免：

- magic number
- 過於複雜的邏輯

**Severity thresholds:**
- **Medium**: duplicated logic that will diverge (copy-paste bug risk)
- **Low**: magic number, unclear naming, overly complex expression

---

# 專案高風險模式 (Codebase-Specific Patterns)

**檢查 `.github/PROJECT-MAP.md` §Code Review 是否有 project-specific high-risk patterns 文件。若有，必須一併載入並套用。** 該文件包含本專案特有的危險模式（如特定的 IPC、locking、callback 慣例），generic checklist 無法涵蓋。若該文件與本 skill 的嚴重度不同，以該文件為準。

---

# 回覆格式

當發現問題時，請使用以下格式：

Issue:
問題簡述

Location:
`path/to/file.cpp:line`

Severity:
Low / Medium / High

Trigger:
什麼輸入或狀態會導致什麼錯誤結果（High / Medium 必填）

Explanation:
為什麼這是問題

Suggestion:
建議的修正方式

Commit message 的格式與檢查由 reviewer agent 依 `PROJECT-MAP.md` §Commit Message Convention 處理，本 skill 不另行定義。

---

# Review 原則

Reviewer 應該：

- 專注在 **本次 diff 的影響**
- 提供 **具體可執行的建議**
- 避免只做風格 nitpick
- 保持專業與建設性
