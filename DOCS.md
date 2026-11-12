# MicroChat - Technical Architecture
This document describes the internal implementation, threading model, and protocol of MicroChat. For user-facing features, build instructions, and history, please refer to `README.md` and `CHANGELOG.md`.

## Code Organization
The project is written in pure C and strictly split into modules to separate concerns. It compiles as a single portable `.exe` with no external dependencies beyond standard Win32 API.

| File | Responsibility |
| :--- | :--- |
| `main.c` | Entry point (`wWinMain`), `CrashHandler`, mode selection (Host/Client). |
| `network.c` | WinSock initialization, server/client logic, receive thread, sending. |
| `ui.c` | Window class, controls, message pump, dialogs, UI proxy. |
| `utils.c` | Shared helpers (IP detection, string trimming, clean exit). |
| `debug.c` | Debug-only features (crash test, GDI count, internal state). |

## Threading and Synchronization
MicroChat uses a minimalistic, yet thread-safe model. There are two primary execution threads:

1.  **UI Thread (Main):** Owns the window (`hWndGlobal`), handles user input, and manages the message loop.
2.  **Receive Thread:** Created via `_beginthreadex`. It blocks on `recv()` to read network data and push it to the UI.

**Synchronization mechanisms:**
- `is_running` is a `volatile LONG` accessed exclusively via `InterlockedCompareExchange` / `InterlockedExchange`. This ensures the Receive Thread exits safely when the UI Thread signals a shutdown.
- **UI Proxy:** The Receive Thread **never touches HWND directly**. It uses `UIAddMessage()`, which duplicates the string (`_wcsdup`) and posts it via `PostMessage(WM_APP_ADDMSG, ...)`. The UI Thread frees the memory. This prevents race conditions and deadlocks.

## Network Protocol
Unlike QuickChat, MicroChat uses an extremely barebones, stateless protocol. There is no handshake marker, no obfuscation, and no length prefix.

### 1. Handshake
- **Client:** Connects to `PORT` (1723). Sends its `computer_name` encoded in **UTF-8, including the null terminator (`\0`)**.
- **Server:** Waits for connection. Reads the client's name (expecting the null terminator). Replies with its own `computer_name` in UTF-8 with `\0`.

### 2. Data Transfer
- Messages are encoded in **UTF-8 without the null terminator**.
- Format: `"<computer_name>: <message>"`.
- TCP boundaries are treated as message boundaries (one `send` = one message). 
- `TrimIncompleteUTF8()` is used before `MultiByteToWideChar` to prevent message loss if a multi-byte character is split across TCP packets.

## Shutdown & Cleanup Sequence
A proper shutdown is critical to prevent deadlocks. The sequence in `CleanupAndExit()` is strictly defined:

1. `already_exiting` flag is checked and set via `InterlockedExchange` (prevents double-cleanup from `WM_CLOSE` and `WM_DESTROY`).
2. `is_running` is set to `0`.
3. `shutdown(client_socket, SD_BOTH)` is called to signal the socket.
4. `WaitForSingleObject(receive_thread, THREAD_WAIT_MS)` waits for the thread to exit.
5. `closesocket(client_socket)` is called **only after** the thread has finished.
6. `WSACleanup()` and `PostQuitMessage()` finalize the process.

## Built-in Safeguards
- **`SO_RCVTIMEO`:** The socket is configured with a 5-second timeout. This is the **most crucial fix** in the project. It ensures `recv()` periodically wakes up to check `is_running`, preventing infinite blocks.
- **`CrashHandler`:** Set via `SetUnhandledExceptionFilter`. Catches Access Violations, logs the error code and address to `crashlog.txt`, and prevents silent silent process termination.

## Debug Mode (`_DEBUG`)
Debug features are strictly isolated. In Release builds, all debug functions become empty stubs (used `#ifdef _DEBUG`).
- **Hotkeys:** `Ctrl+Shift+D` (Internal Stats), `Ctrl+Shift+C` (Crash), `Ctrl+Shift+G` (GDI Stats).
- **Requirements:** Debug build must be compiled with `build.bat /dbg` to retain symbols for analysis.

## Limitations by Design
- **No protocol validation:** The server will accept any incoming data. It trusts the client to send a valid UTF-8 string. (This is why some heuristic antiviruses may flag it as a potential backdoor).
- **Blocking UI on send:** `SendCurrentMessage` calls `send()` directly in the UI thread. While fast on LAN, a dead socket might freeze the UI briefly until the `SO_SNDTIMEO` (if set) or OS TCP timeout triggers.
- **1:1 Only:** The server handles exactly one client. There is no multi-client list or broadcast.
