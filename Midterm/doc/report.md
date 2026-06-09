# 學習筆記：行程監視器 (Process Monitor) 的系統程式原理

---

## 目錄

1. [前言](#前言)
2. [/proc 檔案系統深度解析](#proc-檔案系統深度解析)
3. [行程的生命週期與狀態](#行程的生命週期與狀態)
4. [本專案使用的系統呼叫 (System Calls)](#本專案使用的系統呼叫)
5. [訊號處理 (Signal Handling)](#訊號處理)
6. [記憶體管理概觀](#記憶體管理概觀)
7. [行程排程 (Process Scheduling)](#行程排程)
8. [跨平台實作的差異](#跨平台實作的差異)
9. [效能考量與最佳化](#效能考量與最佳化)
10. [參考文獻](#參考文獻)

---

## 前言

本專案開發一個跨平台的行程監視器 (Process Monitor)，類似於簡化版的 `htop` 或 `top` 指令。透過讀取作業系統提供的行程資訊介面，即時顯示系統資源使用情況與執行中行程的詳細資料。

選擇此主題的原因有三：
1. **實用性** — 行程監控是系統管理與除錯的核心技能
2. **教育性** — 深入理解 `/proc`、行程狀態、訊號等系統程式基礎概念
3. **跨平台挑戰** — 比較 Linux、macOS、Windows 三種平台的實作差異

### 專案結構

```
Midterm/
├── README.md                    # 使用說明文件
├── doc/
│   └── report.md                # 本學習筆記
└── src/
    ├── process-monitor.sh       # Linux 版本 (bash)
    ├── process-monitor.ps1      # Windows 版本 (PowerShell)
    ├── process-monitor-mac.sh   # macOS 版本 (bash)
    └── tests/
        └── run_tests.sh         # 自動化測試
```

---

## /proc 檔案系統深度解析

### 什麼是 /proc？

`/proc` 是一個**虛擬檔案系統 (Virtual Filesystem)**，存在於記憶體中而非磁碟上。它是 Linux 核心提供給使用者空間 (userspace) 的介面，讓一般程式可以透過標準的檔案讀取操作 (`open`, `read`) 來獲取核心內部的資訊。

```
$ mount | grep proc
proc on /proc type proc (rw,nosuid,nodev,noexec,relatime)
```

### 本專案讀取的 /proc 檔案

| 檔案路徑 | 內容 | 本專案用途 |
|----------|------|-----------|
| `/proc/stat` | CPU 時間統計 (user, system, idle, iowait...) | 計算 CPU 使用率 |
| `/proc/meminfo` | 記憶體使用狀況 (total, free, available, swap) | 顯示記憶體與 swap 使用量 |
| `/proc/loadavg` | 系統負載 (1/5/15 分鐘平均) | 顯示 Load Average |
| `/proc/uptime` | 系統開機時間 | 顯示 Uptime |
| `/proc/[pid]/stat` | 行程統計 (PID, 狀態, CPU time, RSS...) | 各行程的 CPU 時間與記憶體 |
| `/proc/[pid]/comm` | 行程名稱 | 顯示行程名稱 |
| `/proc/[pid]/cmdline` | 完整指令列 | 顯示行程命令 |
| `/proc/[pid]/status` | 行程詳細狀態 (UID, VmRSS...) | 取得行程使用者 |

### CPU 使用率計算原理

Linux 的 `/proc/stat` 第一行記錄了 CPU 在各狀態累積的 jiffy 數：

```
cpu  用户态  nice态 系统态 空闲态 iowait irq softirq steal guest guest_nice
```

要計算 CPU 使用率，必須**取兩個時間點之間的差值**：

```bash
# 第一次讀取
read_cpu_line_1  →  total_1, idle_1
sleep 0.15       # 等待一小段時間
# 第二次讀取
read_cpu_line_2  →  total_2, idle_2

CPU使用率 = 100 × (total_diff - idle_diff) / total_diff
```

其中 `total_diff = total_2 - total_1`，`idle_diff = idle_2 - idle_1`。

### 記憶體計算：MemAvailable vs MemFree

- **MemFree**: 完全未被使用的記憶體（低水位線，實務上意義不大）
- **MemAvailable**: 核心估算的可用記憶體，包含可回收的 page cache 與 buffers（更貼近實際可用量）

本專案使用 `MemAvailable` 計算已用記憶體：

```
used_mem = MemTotal - MemAvailable
```

### 行程 CPU 時間

從 `/proc/[pid]/stat` 讀取四個 CPU 時間欄位：

| 欄位 | 意義 |
|------|------|
| utime (欄位 14) | 使用者態 CPU 時間 |
| stime (欄位 15) | 核心態 CPU 時間 |
| cutime (欄位 16) | 子行程使用者態 CPU 時間 |
| cstime (欄位 17) | 子行程核心態 CPU 時間 |

總 CPU 時間 = utime + stime + cutime + cstime（單位：jiffies）

---

## 行程的生命週期與狀態

### Linux 行程狀態

從 `/proc/[pid]/stat` 的第三欄位（state）可以讀取行程狀態：

| 狀態碼 | 說明 | 系統呼叫/事件 |
|--------|------|-------------|
| **R** (Running) | 正在執行或在 runqueue 中等待 | 被排程器選中 |
| **S** (Sleeping) | 可中斷的睡眠（等待事件） | `wait_for_completion()`, `schedule_timeout()` |
| **D** (Disk Sleep) | 不可中斷的睡眠（等待 I/O） | 等待磁碟/網路 I/O 完成 |
| **Z** (Zombie) | 已結束但父行程尚未回收 | `exit()`, 父行程未呼叫 `wait()` |
| **T** (Stopped) | 暫停執行 | 收到 `SIGSTOP`/`SIGTSTP` 訊號 |
| **I** (Idle) | 閒置核心執行緒 | 核心內部使用 |

### fork() + exec() 模式

Linux 建立新行程的標準模式（shell 執行指令時使用）：

```
父行程 ──fork()──▶ 子行程 (複製父行程)
                        │
                        └──exec()──▶ 載入新程式覆蓋記憶體空間
```

- `fork()`: 複製父行程的記憶體空間、檔案描述子等（採用 Copy-On-Write 最佳化）
- `exec()`: 用新程式的程式碼與資料替換目前行程的記憶體內容

---

## 本專案使用的系統呼叫

雖然本專案以 bash 實作（透過 shell 內建指令間接呼叫系統呼叫），但底層涉及以下系統呼叫：

| 系統呼叫 | 用途 | 對應的 shell 操作 |
|---------|------|------------------|
| `open()` / `read()` / `close()` | 讀取 `/proc` 檔案 | bash 的 `<` 重導向、`$(<file)` |
| `kill(pid, sig)` | 傳送訊號給行程 | `kill` 內建指令 |
| `nanosleep()` | 精確休眠 | `sleep` 指令 |
| `getent()` | 查詢使用者名稱 | `getent passwd` |
| `stat()` | 檢查 `/proc/[pid]` 目錄是否存在 | `[ -d /proc/123 ]` |
| `waitpid()` | (間接) 回收殭屍行程 | 由 init 行程處理 |

### kill 訊號傳送機制

當使用者執行 `kill -9 1234` 時，背後執行的步驟：

1. bash 呼叫 `kill(2)` 系統呼叫，傳入 pid=1234, sig=SIGKILL(9)
2. 核心檢查權限（是否為行程擁有者或 root）
3. 核心將 SIGKILL 訊號加入目標行程的 pending signal 佇列
4. 目標行程被排程時，核心檢查 pending signals
5. SIGKILL 不可被捕捉或忽略，核心強制終止該行程
6. 行程變為 Zombie 狀態，由父行程（或 init）回收

---

## 訊號處理

### 本專案中使用的訊號

```bash
trap 'echo -e "\nExiting."; exit 0' INT TERM
```

這行程式碼告訴 bash：當收到 `SIGINT` (Ctrl+C) 或 `SIGTERM` (kill 預設訊號) 時，執行清理動作後安全退出。

### 常用訊號對照表

| 訊號 | 數字 | 預設行為 | 可否捕捉 |
|------|------|---------|---------|
| SIGHUP | 1 | Terminate | 可 |
| SIGINT | 2 | Terminate (Ctrl+C) | 可 |
| SIGQUIT | 3 | Core dump | 可 |
| SIGKILL | 9 | Terminate (強制) | **不可** |
| SIGTERM | 15 | Terminate (預設) | 可 |
| SIGSTOP | 19 | Stop | **不可** |
| SIGTSTP | 20 | Stop (Ctrl+Z) | 可 |
| SIGCONT | 18 | Continue | 可 |

### 為什麼 kill -9 是「最後手段」

`SIGKILL` 直接由核心強制終止行程，行程沒有任何機會：
- 釋放資源（檔案描述子、共享記憶體等由核心回收，但應用層資源可能遺失）
- 寫入日誌
- 通知其他服務
- 完成進行中的交易

因此建議先嘗試 `SIGTERM` (kill 不加 -9)，若無效再使用 `SIGKILL`。

---

## 記憶體管理概觀

### RSS (Resident Set Size)

從 `/proc/[pid]/stat` 第 24 欄位讀取的 `rss` 值，單位是**記憶體頁面 (pages)**：

```bash
# Linux 頁面大小通常是 4KB
rss_kb = rss_pages × 4
rss_mb = rss_kb / 1024
```

RSS 表示該行程**實際佔用**的實體記憶體，但不包含：
- 被 swap 出去的頁面
- 共享函式庫中已被其他行程載入的部分（會重複計算）

### 虛擬記憶體架構

```
應用程式視角                        實體記憶體
┌──────────────┐                  ┌──────────┐
│   Virtual    │  ── MMU ──▶     │ Physical │
│   Address    │  (頁表轉換)       │  Memory  │
│   Space      │                  │          │
│   (4GB/32bit │                  │          │
│    或超大)   │                  └──────────┘
│              │                       │
│   ┌──────┐   │                  ┌────▼─────┐
│   │ stack│   │                  │   Swap   │
│   ├──────┤   │   (page fault    │   Partition│
│   │ mmap │   │    → 載入)       │ or File   │
│   ├──────┤   │                  └──────────┘
│   │ heap │   │
│   ├──────┤   │
│   │ .bss │   │
│   ├──────┤   │
│   │ .data│   │
│   ├──────┤   │
│   │ .text│   │
│   └──────┘   │
└──────────────┘
```

---

## 行程排程

### CFS (Completely Fair Scheduler)

Linux 2.6.23+ 使用 CFS 作為預設排程器。核心概念：

- 每個行程有一個 **vruntime**（虛擬執行時間）
- 排程器永遠選擇 **vruntime 最小**的行程執行
- vruntime 的增長速度與行程的 nice 值相關（nice 值越高，vruntime 增長越快 → 分配到較少 CPU 時間）

### Load Average 的意義

```
$ cat /proc/loadavg
1.23 0.89 0.67 3/456 12345
 │    │    │    │  │    └─ 最新建立的 PID
 │    │    │    │  └─ 正在執行的行程數/總行程數
 │    │    │    └─ 15分鐘平均負載
 │    │    └─ 5分鐘平均負載
 │    └─ 1分鐘平均負載
```

Load Average 代表的不是單純的 CPU 使用率，而是**平均活躍行程數**（包含等待 CPU、等待 I/O 的行程）：

- Load = CPU 核心數 → 系統剛好滿載
- Load > CPU 核心數 → 有行程在排隊等待
- Load < CPU 核心數 → CPU 有空閒

---

## 跨平台實作的差異

### 系統資訊介面比較

| 功能 | Linux | macOS | Windows |
|------|-------|-------|---------|
| 行程列表 | `/proc/[pid]/` + `/proc/[pid]/stat` | `ps -eo` (BSD 風格) | `Get-Process` (PowerShell) |
| CPU 使用率 | `/proc/stat` (jiffy 計數器) | `top -l 1` 輸出解析 | `Get-Counter '\Processor(_Total)\% Processor Time'` |
| 記憶體資訊 | `/proc/meminfo` | `vm_stat` + `sysctl hw.memsize` | `Get-CimInstance Win32_OperatingSystem` |
| 系統負載 | `/proc/loadavg` | `sysctl vm.loadavg` | `Get-Counter '\System\Processor Queue Length'` |
| 開機時間 | `/proc/uptime` | `sysctl kern.boottime` | `Win32_OperatingSystem.LastBootUpTime` |
| 終止行程 | `kill -9 <pid>` (SIGKILL) | `kill -9 <pid>` (同 Linux) | `Stop-Process -Id <pid>` |
| Swap 資訊 | `/proc/meminfo` (SwapTotal/SwapFree) | `sysctl vm.swapusage` | `Get-CimInstance Win32_PageFileUsage` |

### 關鍵設計差異

1. **Linux** — 一切皆檔案：所有系統資訊透過 `/proc` 虛擬檔案系統暴露。優點是任何能讀檔的語言都能存取，無需特殊 API。缺點是解析格式各異，缺乏統一的結構化輸出。

2. **macOS** — BSD 血統：繼承 BSD 的 `/proc` 實作較不完整（許多資訊被 macOS 移除或限制）。需仰賴 `sysctl`、`vm_stat`、`ps` 等命令取得系統資訊。這使得相同功能的腳本需要較多平台分支。

3. **Windows** — API 導向：不提供 `/proc` 等價物。透過 WMI (Windows Management Instrumentation) 或 PowerShell cmdlets 取得結構化的系統資訊。優點是回傳結構化物件，缺點是命令較冗長且啟動開銷較大。

---

## 效能考量與最佳化

### 讀取 /proc 的開銷

`/proc` 的內容由核心即時生成，不需磁碟 I/O：

```
讀取 /proc/[pid]/stat 的開銷約 1-5µs（微秒）
掃描 300 個行程約需 1-3ms
```

對每秒更新 2 次的監控工具而言，這幾乎不影響系統效能。

### 本專案的效能策略

1. **減少子行程 spawn** — 盡量使用 bash 內建指令（`read`, `<`）而非外部命令（`awk`, `grep` 僅在必要時使用）
2. **批次讀取** — 一次讀取整個目錄列表，而非逐個檢查
3. **可調整的更新頻率** — 提供 `-i` 參數讓使用者根據需求調整

---

## 參考文獻

1. Bovet, D. P., & Cesati, M. (2005). *Understanding the Linux Kernel* (3rd ed.). O'Reilly Media.
2. Kerrisk, M. (2010). *The Linux Programming Interface*. No Starch Press.
3. Linux Kernel Documentation — `Documentation/filesystems/proc.txt`
4. McKusick, M. K., Neville-Neil, G. V., & Watson, R. N. M. (2014). *The Design and Implementation of the FreeBSD Operating System* (2nd ed.). Addison-Wesley.
5. Russinovich, M., Solomon, D., & Ionescu, A. (2012). *Windows Internals* (6th ed.). Microsoft Press.
6. Linux Man Pages: `proc(5)`, `kill(2)`, `signal(7)`, `fork(2)`, `exec(3)`
