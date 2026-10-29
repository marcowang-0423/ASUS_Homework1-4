---
name: csharp-code-review-guidelines
description: "C# / WPF code review checklist for reviewing C# and XAML diffs, PRs, or commits. Checks correctness, resource management (IDisposable), async/await safety, WPF UI-thread and binding rules, event-subscription leaks, P/Invoke, untrusted input, concurrency, performance, and error handling. Use when reviewing .cs, .csproj, or .xaml files."
---

# C# Code Review 指南（僅針對 Code Diff）

## 範圍

本次 review **只關注 PR / diff 中新增或修改的程式碼**。

請遵守以下原則：

- 只評論 **新增或修改的程式碼**
- 不評論未改動的程式碼
- 除非 diff 造成潛在問題，否則不要討論舊程式碼
- 重點在 **變更造成的影響**

## 語言版本限制

WPF app 為 **.NET Framework 4.8、`LangVersion` 7.3**（以實際 `.csproj` 為準）。建議的修正必須能在 C# 7.3 編譯，**不可**建議：

- `await using` / `IAsyncDisposable`（C# 8）
- nullable reference types（`string?`）、switch expression、`using` declaration（`using var x = ...;`）、index / range（`^1`、`..`）
- `record`、`init`、target-typed `new()`

需要時改用 `using (...) { }`、傳統 `switch` statement 等 7.3 寫法。

---

# Review 優先順序

審查時請依照以下優先順序：

1. Correctness
2. Safety — 資源、不可信任輸入、P/Invoke
3. Concurrency — 包含 WPF UI thread
4. Error Handling
5. Performance
6. Maintainability

優先關注 **邏輯錯誤與潛在 bug**，而不是單純的程式風格問題。

## 嚴重度原則

- **High**：在正式環境中可能造成 crash、資料損毀、安全漏洞或資源洩漏。
- **Medium**：特定條件下行為錯誤、錯誤被悄悄忽略、可能造成間歇性問題的 thread-safety 疑慮。
- **Low**：風格、可讀性、功能正確但不慣用的寫法。

每個 High / Medium finding 都必須能寫出具體的 **Trigger**（什麼輸入或狀態 → 什麼錯誤結果）。寫不出來的，降級或移除。

---

# Diff Review 檢查清單

## 1. 正確性 (Correctness)

確認修改沒有引入邏輯錯誤。

檢查：

- API 是否正確使用
- Null reference 風險
- 不完整的條件判斷
- 邏輯改動是否影響既有功能

特別注意：

- `null` handling
- `ArgumentException` / `InvalidOperationException`
- 邊界條件 (edge cases)

**Severity thresholds:**
- **High**: null dereference on a code path reachable in production, wrong cast (`InvalidCastException`), logic inversion
- **Medium**: edge case not handled, API misuse that may fail silently, incorrect enum comparison

---

## 2. 資源管理 (Resource Safety)

雖然 C# 有 GC，但仍需檢查資源釋放。

檢查：

- 持有 **OS handle / unmanaged 資源** 的 `IDisposable`（`FileStream`、`Socket`、`Process`、`Bitmap`、`NamedPipe*Stream`、`Timer`、`SafeHandle`）是否用 `using` 或在所有路徑 `Dispose()`
- 只持有 managed memory 的 `IDisposable`（`MemoryStream`、`Task`、`CancellationTokenSource` 未使用 timer 時）不 Dispose 通常無害，不需要報告

避免：

- 長時間持有大物件（大型 `byte[]`、`BitmapSource`）

**Severity thresholds:**
- **High**: handle-owning `IDisposable` created repeatedly (per call / per frame / per connection) and never disposed
- **Medium**: handle-owning `IDisposable` disposed only on the success path (missing in exception path), one-off handle leak
- **Low**: managed-only `IDisposable` not disposed

---

## 3. 事件訂閱造成的記憶體洩漏 (Event Subscription Leaks)

本專案的 Manager 大量使用 **`public static event`**（例如 `GlideXDeviceManager.DeviceStatusChanged`、`LinkDeviceManager.DeviceListChanged`、`DialogHelper.OnDialogClosedEvent`）。static event 會一直持有訂閱者，View / ViewModel 若訂閱後沒解除，永遠不會被 GC 回收。

檢查：

- 新增的 `+=` 是否有對應的 `-=`（在 `Unloaded`、`Closed`、`Dispose`、`OnNavigatedFrom` 等對應時機）
- 訂閱是否可能被重複執行（每次 `Loaded` 都 `+=`）導致 handler 被呼叫多次
- lambda 訂閱（`+= (s, e) => ...`）無法 `-=`，若訂閱者生命週期短於發布者則需改成具名方法

**Severity thresholds:**
- **High**: short-lived object (window, page, per-device ViewModel) subscribes to a static / long-lived event and never unsubscribes — leak grows with every open/close
- **Medium**: handler subscribed multiple times (duplicate side effects), lambda subscription that cannot be removed on a medium-lived object

---

## 4. 非同步與 Task 使用 (Async Safety)

若 diff 涉及 async/await：

檢查：

- `async void`（除非 event handler）
- `.Result` 或 `.Wait()` 造成 deadlock
- 未 await 的 Task
- async method 是否真正需要 async

建議：

- 使用 `async Task`
- 避免 blocking async code

**Severity thresholds:**
- **High**: `async void` (non-event-handler) — unobserved exceptions crash the process, `.Result` or `.Wait()` on UI thread — deadlock
- **Medium**: unawaited `Task` (fire-and-forget without explicit intent or exception handling), `async void` event handler with no `try/catch`, `async` method that never awaits

---

## 5. WPF UI Thread 與 Binding

WPF 元件只能在建立它的 UI thread 上存取。

檢查：

- 在背景 thread（RPC callback、`Task.Run`、`System.Threading.Timer`、Manager 的 static event handler）中是否直接存取 UI 元件或 `ObservableCollection` 綁定的集合 → 需透過 `Dispatcher.Invoke` / `Dispatcher.BeginInvoke`
- `Dispatcher.Invoke`（同步）是否可能在持有 lock 時呼叫，或 UI thread 同時在等待該背景 thread → deadlock；非必要時優先 `BeginInvoke`
- 綁定到 UI 的屬性變更時是否觸發 `PropertyChanged`，`nameof()` 是否指向正確屬性
- XAML binding 路徑是否對應 ViewModel 中實際存在的 public 屬性（打錯不會報錯，只會悄悄失效）
- `DispatcherTimer` 與 `System.Threading.Timer` 是否用對（前者在 UI thread，後者不是）

**Severity thresholds:**
- **High**: UI element or UI-bound collection modified from a non-UI thread (`InvalidOperationException` crash), synchronous `Dispatcher.Invoke` that can deadlock with the UI thread
- **Medium**: missing `PropertyChanged` (UI shows stale data), wrong binding path, long-running work on the UI thread inside `Dispatcher.Invoke`

XAML 的 `DynamicResource`（FontSize / Color）規則見 `PROJECT-MAP.md` §Code Review 的 high-risk patterns 文件。

---

## 6. P/Invoke 與原生模組互通 (Interop)

本專案透過 `DllImport` 呼叫 `user32.dll` 與原生模組（例如 RPC client 的 `RpcFuncEntry`、`GlideXDisplayArrangement.dll`）。

檢查：

- `CallingConvention` 是否與原生端宣告一致（原生 export 多為 `Cdecl`）
- 參數型別與大小是否對應（`int` vs `IntPtr`、`bool` vs `BOOL`、`CharSet`、struct `StructLayout` / `Pack`），x64 / ARM64 都要正確
- 原生端配置的記憶體由誰釋放，是否有對應的 free 呼叫
- 傳給原生端的 callback delegate 是否被保存為欄位，避免被 GC 回收後原生端呼叫到已回收的 delegate
- 需要 `GetLastError` 時是否設定 `SetLastError = true` 並用 `Marshal.GetLastWin32Error()`

**Severity thresholds:**
- **High**: calling convention / parameter size mismatch, callback delegate not kept alive, buffer size passed to native smaller than actual write
- **Medium**: native allocation not freed, missing `SetLastError` when the error code is used

---

## 7. 不可信任輸入 (Untrusted Input)

來自 service RPC、手機端、網路、檔案、registry 的資料都視為不可信任。

檢查：

- **路徑穿越**：外部提供的檔名 / 路徑（FileTransfer、DragDrop）在組合前是否用 `Path.GetFileName` 取出檔名，或在 `Path.GetFullPath` 後確認仍在目標目錄內
- **反序列化**：`JsonConvert.DeserializeObject` 不可設定 `TypeNameHandling` 為 `None` 以外的值；不可使用 `BinaryFormatter`；反序列化結果可能為 `null` 或欄位缺漏
- **命令執行**：外部字串是否被拼進 `Process.Start` 的檔名或參數
- 外部提供的數量 / 長度是否驗證上限後才用來配置陣列

**Severity thresholds:**
- **High**: path traversal allowing write outside the target directory, `TypeNameHandling` other than `None` on external data, `BinaryFormatter` on external data, external string injected into `Process.Start`
- **Medium**: deserialization result used without null check, unbounded allocation from an external count

---

## 8. 多執行緒安全 (Concurrency)

檢查：

- shared state 是否安全
- race condition
- lock 使用是否合理
- concurrent collection 是否需要

注意：

- `lock`
- `SemaphoreSlim`
- `ConcurrentDictionary`

思考：

- 新增程式碼是否破壞 thread safety？

**Severity thresholds:**
- **High**: shared state modified without lock, race condition on a frequently-accessed path
- **Medium**: `SemaphoreSlim` not released in `finally`, potential deadlock due to nested locks, missing `ConcurrentDictionary` when accessed from multiple threads

（`lock` 區塊內無法 `await`，compiler 會擋；需要跨 `await` 的互斥請用 `SemaphoreSlim.WaitAsync` + `finally { Release(); }`。）

---

## 9. 錯誤處理 (Error Handling)

檢查：

- 是否忽略 exception
- try/catch 是否過大範圍
- 是否吞掉 exception

建議：

- 適當 logging
- 保留 stack trace

**Severity thresholds:**
- **High**: empty `catch` that swallows a failure the caller must know about (e.g. save / connect / RPC result reported as success)
- **Medium**: empty `catch` without comment or logging, `throw ex;` instead of `throw;` (loses stack trace), overly broad `catch (Exception)` that should target specific exception types
- **Low**: intentional empty `catch` with a comment explaining why

---

## 10. 效能 (Performance)

評估修改是否造成效能問題。

檢查：

- 不必要 allocation
- LINQ 在 hot path
- 過多 boxing/unboxing
- 重複計算

避免：

- 在 loop 內建立物件
- 不必要的 `ToList()`
- heavy LINQ in hot paths

**Severity thresholds:**
- **High**: O(n²) or worse in hot path, blocking call (I/O, RPC, `Thread.Sleep`, `.Result`) on UI thread
- **Medium**: unnecessary allocation per frame/tick in streaming loop, repeated LINQ materialization (`ToList()`) on same data

---

## 11. 可維護性 (Maintainability)

檢查：

- method 是否過長
- 命名是否清楚
- 是否重複邏輯

避免：

- magic number
- 複雜巢狀 if

建議：

- Extract method
- 使用 constants

**Severity thresholds:**
- **Medium**: duplicated logic that will diverge (copy-paste bug risk)
- **Low**: magic number, unclear naming, deeply nested conditionals

---

# 專案高風險模式 (Codebase-Specific Patterns)

**檢查 `.github/PROJECT-MAP.md` §Code Review 是否有 project-specific high-risk patterns 文件。若有，必須一併載入並套用。** 若該文件與本 skill 的嚴重度不同，以該文件為準。

---

# 回覆格式

當發現問題時，請使用以下格式：

Issue:
問題簡述

Location:
`path/to/File.cs:line`

Severity:
Low / Medium / High

Trigger:
什麼輸入或狀態會導致什麼錯誤結果（High / Medium 必填）

Explanation:
為什麼這是問題

Suggestion:
建議修正方式（必須符合 C# 7.3）

Commit message 的格式與檢查由 reviewer agent 依 `PROJECT-MAP.md` §Commit Message Convention 處理，本 skill 不另行定義。

---

# Review 原則

Reviewer 應該：

- 專注在 **本次 diff 的影響**
- 提供 **具體可執行的建議**
- 避免只做風格 nitpick
- 保持專業與建設性
