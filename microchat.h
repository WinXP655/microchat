// Header for MicroChat source files.
// Official repository: https://github.com/WinXP655/microchat.
// Distributed under MIT license.
#ifndef MICROCHAT_H
#define MICROCHAT_H

// Target platform
#define _WIN32_WINNT 0x0500

// Includes
#include <winsock2.h>
#include <windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <process.h>
#include <wchar.h>

// Defines
#define ID_EDIT 101
#define ID_SEND 102
#define ID_MSG_DISPLAY 103
#define IDC_IP 1001
#define PORT 1723
#define NET_TIMEOUT_SEC 5
#define NET_TIMEOUT_USEC 0
#define BUFFER_SIZE 8192
#define THREAD_WAIT_MS 3000
#define DEFAULT_WIN_WIDTH  520
#define DEFAULT_WIN_HEIGHT 320
#define WM_APP_ADDMSG (WM_APP + 1)

// Global variables
extern bool is_server;
extern volatile LONG is_running;
extern SOCKET client_socket;
extern HANDLE receive_thread;
extern wchar_t server_ip[16];
extern wchar_t peer_ip[16];
extern wchar_t peer_name[256];
extern wchar_t computer_name[256];
extern HWND hEdit;
extern HWND hSendBtn;
extern HWND hMsgDisplay;
extern HWND hWndGlobal;
extern WNDPROC oldEditProc;
extern HFONT hFont;
extern HFONT hFontBold;

// Prototypes

// utils.c
void GetLocalComputerName(void);
bool GetDefaultIP(wchar_t* ip_buffer, size_t size);
void AddMessage(const wchar_t* msg);
void CleanupAndExit(void);
void ShowError(const wchar_t* msg, DWORD err);
wchar_t* Trim(wchar_t* s);
int TrimIncompleteUTF8(const char* buffer, int bytes);

// network.c
bool InitializeNetwork(bool server_mode, HINSTANCE hInstance, int nCmdShow);
bool StartServer(HINSTANCE hInstance, int nCmdShow);
bool StartClient(HINSTANCE hInstance, int nCmdShow);
unsigned int __stdcall ReceiveMessages(void* arg);
void SendCurrentMessage(HWND hWnd);
void Disconnect(void);

// ui.c
void ShowMainWindow(HINSTANCE hInstance, int nCmdShow);
BOOL ValidateAndSetIP(HWND hwnd);
INT_PTR CALLBACK ConnectDialogProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CreateAppFonts(HWND hWnd);
void CreateAppControls(HWND hWnd);
void ResizeMainWindow(HWND hWnd, int width, int height);
void CleanupGdi(void);
LRESULT CALLBACK EditProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI ShowServerIpMessage(LPVOID lpParam);
void UIAddMessage(const wchar_t* msg);

// debug.c
void ShowDebugInfo(HWND hWnd);

#endif