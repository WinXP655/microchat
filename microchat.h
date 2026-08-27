#ifndef MICROCHAT_H
#define MICROCHAT_H

#include <winsock2.h>
#include <windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <process.h>

// Defines
#define ID_EDIT 101
#define ID_SEND 102
#define ID_MSG_DISPLAY 103
#define IDC_IP 1001
#define PORT 1723
#define BUFFER_SIZE 8192

// Global variables
extern bool is_server;
extern int is_running;
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
void GetLocalComputerName(void);
bool InitializeNetwork(bool server_mode, HINSTANCE hInstance, int nCmdShow);
bool GetDefaultIP(wchar_t* ip_buffer, size_t size);
void AddMessage(const wchar_t* msg);
void CleanupAndExit(void);
void Disconnect(void);
void ShowError(const wchar_t* msg, DWORD err);
unsigned int __stdcall ReceiveMessages(void* arg);
void SendCurrentMessage(HWND hWnd);
void ShowMainWindow(HINSTANCE hInstance, int nCmdShow);
INT_PTR CALLBACK ConnectDialogProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI ShowServerIpMessage(LPVOID lpParam);
void ShowDebugInfo(HWND hWnd);

#endif