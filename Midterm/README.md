# Process Monitor — 跨平台行程監視器

> 系統程式期中專案 | 114 學年下學期 | 溫禮安

---

## 專案簡介

一個輕量級的**跨平台終端機行程監視器**，可以即時查看系統資源使用狀況與執行中行程的詳細資訊。支援 **Linux**、**macOS**、**Windows** 三個平台。

### 功能特色

- 即時顯示 CPU 使用率、記憶體使用量、Swap 使用量、系統負載、開機時間
- 依 CPU 時間 / 記憶體用量 / PID / 名稱排序行程
- 依名稱篩選行程
- 終止指定行程（kill）
- 彩色輸出（高負載紅色、中負載黃色、正常綠色）
- 鍵盤互動操作（可隨時切換排序欄位）
- 可自訂更新頻率

### 畫面預覽

```
╔══════════════════════════════════════════════════════════════╗
║  Process Monitor — myhost — 2026-06-09 14:30:00
╠══════════════════════════════════════════════════════════════╣
║  CPU:  12.5%  Mem: 3890 / 15912 MB ( 24.4%)  Swap: 512 / 2048 MB
║  Load: 1.23 0.89 0.67  Uptime: 3d 12h 34m
╠══════════════════════════════════════════════════════════════╣
║ PID      USER       CPU TIME   MEM(MB)  NAME            COMMAND
╠══════════════════════════════════════════════════════════════╣
║ 1234     biana      0h15m32s   1024.5   firefox         /usr/lib/firefox/firefox
║ 5678     biana      0h08m21s    512.3   gnome-shell     /usr/bin/gnome-shell
║ ...
╚══════════════════════════════════════════════════════════════╝
[q]uit [c]pu-sort [m]em-sort [p]id-sort [n]ame-sort   interval=2s
```

---

## 快速開始

### 下載專案

```bash
# 複製專案
git clone https://github.com/yourusername/process-monitor.git
cd process-monitor/Midterm

# 或直接下載原始碼後解壓縮
unzip process-monitor.zip
cd Midterm
```

### Linux

```bash
# 賦予執行權限
chmod +x src/process-monitor.sh

# 基本使用
./src/process-monitor.sh

# 顯示前 10 名行程，按記憶體排序，每秒更新
./src/process-monitor.sh -n 10 -s mem -i 1

# 只顯示 firefox 相關行程
./src/process-monitor.sh -f firefox

# 終止 PID 1234 的行程
./src/process-monitor.sh -k 1234

# 無彩色輸出（適合寫入檔案）
./src/process-monitor.sh -c -n 5 -i 100

# 查看完整說明
./src/process-monitor.sh -h
```

### macOS

```bash
# 賦予執行權限
chmod +x src/process-monitor-mac.sh

# 使用方式與 Linux 版本相同
./src/process-monitor-mac.sh
./src/process-monitor-mac.sh -n 10 -s mem
./src/process-monitor-mac.sh -f Terminal
./src/process-monitor-mac.sh -h
```

> **注意**: macOS 版本使用 BSD 風格的 `ps`、`vm_stat`、`sysctl` 指令，功能與 Linux 版本一致。

### Windows

```powershell
# 開啟 PowerShell (以系統管理員身分執行以獲取完整資訊)
# 若遇到執行原則限制，先執行：
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass

# 基本使用
.\src\process-monitor.ps1

# 顯示前 10 名行程，按記憶體排序
.\src\process-monitor.ps1 -TopCount 10 -SortBy mem

# 只顯示 chrome 相關行程
.\src\process-monitor.ps1 -FilterName chrome

# 終止 PID 1234 的行程
.\src\process-monitor.ps1 -KillPid 1234

# 查看完整說明
.\src\process-monitor.ps1 -Help
```

---

## 參數說明 (Linux / macOS)

| 參數 | 說明 | 預設值 |
|------|------|--------|
| `-n <N>` | 顯示前 N 個行程 | 20 |
| `-s <欄位>` | 排序欄位：`cpu`, `mem`, `pid`, `name` | cpu |
| `-f <名稱>` | 依名稱篩選行程（不區分大小寫） | 無 |
| `-k <PID>` | 終止指定 PID 的行程後退出 | 無 |
| `-i <秒>` | 更新間隔（秒），可為小數 | 2 |
| `-c` | 停用彩色輸出 | 彩色 |
| `-h` | 顯示使用說明 | — |
| `-v` | 顯示版本資訊 | — |

### 互動按鍵 (監控進行中)

| 按鍵 | 功能 |
|------|------|
| `q` | 退出 |
| `c` | 依 CPU 時間排序 |
| `m` | 依記憶體用量排序 |
| `p` | 依 PID 排序 |
| `n` | 依名稱排序 |

## 參數說明 (Windows)

| 參數 | 說明 | 預設值 |
|------|------|--------|
| `-TopCount <N>` | 顯示前 N 個行程 | 20 |
| `-SortBy <欄位>` | 排序欄位：`cpu`, `mem`, `pid`, `name` | cpu |
| `-FilterName <名稱>` | 依名稱篩選行程 | 無 |
| `-KillPid <PID>` | 終止指定 PID 的行程後退出 | 無 |
| `-Interval <秒>` | 更新間隔（秒） | 2 |
| `-NoColor` | 停用彩色輸出 | 彩色 |
| `-Help` | 顯示使用說明 | — |

---

## 執行測試

```bash
# 賦予執行權限
chmod +x src/tests/run_tests.sh

# 執行測試（僅適用於 Linux）
./src/tests/run_tests.sh
```

測試涵蓋：
- 說明參數 (`-h`, `-v`)
- 無效參數錯誤處理
- Kill 模式（傳送不存在的 PID）
- 各種排序模式 (`-s cpu`, `-s mem`, `-s pid`)
- 名稱篩選 (`-f`)
- 彩色/無彩色模式
- 不同顯示數量限制 (`-n`)

---

## 專案結構

```
Midterm/
├── README.md                    # 本使用說明
├── doc/
│   └── report.md                # 學習筆記：系統程式原理詳解
└── src/
    ├── process-monitor.sh       # Linux 版本 (bash)
    ├── process-monitor.ps1      # Windows 版本 (PowerShell)
    ├── process-monitor-mac.sh   # macOS 版本 (bash)
    └── tests/
        └── run_tests.sh         # 自動化測試腳本
```

---

## 系統需求

### Linux
- Linux kernel 2.6+（支援 `/proc` 檔案系統）
- bash 4.0+
- `bc`（用於浮點數計算）
- `getent`（用於使用者名稱查詢）

### macOS
- macOS 10.12+
- bash 3.2+（系統內建）
- `bc`（系統內建）

### Windows
- Windows 10+ / Windows Server 2016+
- PowerShell 5.1+ 或 PowerShell Core 6+

---

## 技術原理

本專案透過直接讀取作業系統提供的行程資訊介面來取得即時系統狀態：

- **Linux**: 讀取 `/proc` 虛擬檔案系統 (`/proc/stat`, `/proc/meminfo`, `/proc/[pid]/stat` 等)
- **macOS**: 使用 BSD 風格的 `ps`、`vm_stat`、`sysctl` 指令
- **Windows**: 使用 PowerShell 的 `Get-Process`、`Get-Counter`、`Get-CimInstance` cmdlets

詳細的系統程式原理（/proc 解析、fork/exec、訊號處理、行程排程等）請參閱 [`doc/report.md`](doc/report.md)。

---

## 授權條款

MIT License
