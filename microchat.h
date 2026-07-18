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
extern bool isServer;
extern int isRunning;
extern SOCKET clientSocket;
extern HANDLE receiveThread;
extern wchar_t serverIp[16];
extern wchar_t peerIp[16];
extern wchar_t peerName[256];
extern wchar_t computerName[256];
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
DWORD WINAPI ShowServerIPMessage(LPVOID lpParam);

#endif