// MicroChat Framework by WinXP655
// Minimalistic chat framework written on pure C.
// Official repository: https://github.com/WinXP655/microchat.
// MIT License - free to use, modify and distribute.

// -------------------------------------------------

// === 1. Headers ===
#include <winsock2.h>
#include <windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <process.h>

// === 2. Defines ===
#define PORT 1723
#define BUFFER_SIZE 4096
#define IDC_IP 1001
#define ID_SEND 102
#define ID_EDIT 101
#define ID_MSG_DISPLAY 105

// === 3. Global variables ===

// ----- Control flags -----
bool is_server = false;
int is_running = 1;

// ----- Network state -----
SOCKET client_socket = INVALID_SOCKET;
HANDLE g_hReceiveThread = NULL;
char server_ip[16] = "";
char peer_ip[16] = "";
char peer_name[MAX_COMPUTERNAME_LENGTH + 1];
char computer_name[MAX_COMPUTERNAME_LENGTH + 1];

// ----- UI handles -----
HWND hEdit = NULL;
HWND hSendBtn = NULL;
HWND hMsgDisplay = NULL;
HWND hwnd_global = NULL;

// ----- UI resources -----
WNDPROC g_oldEditProc = NULL;
HFONT hFont = NULL;
HFONT hFontBold = NULL;

// === 4. Function prototypes ===
void GetLocalComputerName();
bool InitializeNetwork(bool server_mode, HINSTANCE hInstance, int nCmdShow);
INT_PTR CALLBACK ConnectDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CleanupAndExit();
DWORD WINAPI ShowServerIPMessage(LPVOID lpParam);
void ShowMainWindow(HINSTANCE hInstance, int nCmdShow);
unsigned int __stdcall ReceiveMessages(void* arg);

// === 5. Entry point ===
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	(void)hPrevInstance;
	(void)lpCmdLine;

	// We get local computer name first, so we will already have it when it will be needed.
	GetLocalComputerName();

	int mode = MessageBox(NULL,
		"Start MicroChat as:\n"
		"Yes - Chat Host\n"
		"No - Chat Client\n"
		"Cancel - Exit",
		"MicroChat",
		MB_YESNOCANCEL);

	if (mode == IDCANCEL) return 0;
	is_server = (mode == IDYES);

	if (is_server) {
		if (!InitializeNetwork(true, hInstance, nCmdShow)) return 0;
	} else {
		INT_PTR dlg = DialogBoxParam(hInstance, MAKEINTRESOURCE(1), NULL, ConnectDialogProc, 0);

		// -1 means dialog failed to create.
		if (dlg == -1) {
			MessageBox(NULL, "Could not load connection dialog.", "MicroChat", MB_OK);
			return 0;
		}

		if (dlg != IDOK) return 0;
		if (!InitializeNetwork(false, hInstance, nCmdShow)) return 0;
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

// === 6. Helper functions ===
bool GetDefaultIP(char *ip_buffer, size_t size) {
	// Note: this hack works only on Windows 2000 and higher.
	// On 9x and NT 4.0, it always return 0.0.0.0 as server IP.

	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == INVALID_SOCKET) return false;

	struct sockaddr_in remote = {0};
	remote.sin_family = AF_INET;
	remote.sin_port = htons(53);
	remote.sin_addr.s_addr = inet_addr("8.8.8.8"); // Google DNS, but works with any reachable IP.

	// UDP hack: connect() on UDP keep address in memory...
	connect(s, (struct sockaddr*)&remote, sizeof(remote));

	// ... and then getsockname() return local IP address which will be displayed.
	struct sockaddr_in local;
	int len = sizeof(local);
	getsockname(s, (struct sockaddr*)&local, &len);

	closesocket(s);

	strncpy(ip_buffer, inet_ntoa(local.sin_addr), size - 1);
	ip_buffer[size - 1] = '\0';

	return true;
}

void GetLocalComputerName() {
	DWORD size = sizeof(computer_name);
	GetComputerName(computer_name, &size);
}

void AddMessage(const char* msg) {
	if (!hMsgDisplay || !IsWindow(hMsgDisplay) || !msg || !*msg) return;
	int len = GetWindowTextLength(hMsgDisplay);

	SendMessage(hMsgDisplay, EM_SETSEL, len, len);

	// Using EM_REPLACESEL instead SetWindowText to add string without overwriting entire buffer.
	if (len > 0) SendMessage(hMsgDisplay, EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
	SendMessage(hMsgDisplay, EM_REPLACESEL, FALSE, (LPARAM)msg);
	SendMessage(hMsgDisplay, WM_VSCROLL, SB_BOTTOM, 0);
}

void Disconnect() {
	char leave_msg[256];
	snprintf(leave_msg, sizeof(leave_msg), "%s left the chat.", computer_name);

	AddMessage(leave_msg);
	if (client_socket != INVALID_SOCKET) send(client_socket, leave_msg, strlen(leave_msg), 0);

	CleanupAndExit(hwnd_global);
}

void CleanupAndExit() {
	is_running = 0;

	if (client_socket != INVALID_SOCKET) {
		shutdown(client_socket, SD_BOTH);
		closesocket(client_socket);
		client_socket = INVALID_SOCKET;
	}

	// WaitForSingleObject on a thread with a running recv is a bad idea.
	// It causes deadlock since GUI waits for last data.
	if (g_hReceiveThread != NULL) {
		CloseHandle(g_hReceiveThread);
		g_hReceiveThread = NULL;
	}

	WSACleanup();
	PostQuitMessage(0);
}

// === 7. Network core ===
bool InitializeNetwork(bool server_mode, HINSTANCE hInstance, int nCmdShow) {
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		char buffer[256];
		snprintf(buffer, sizeof(buffer), "WSAStartup error: %lu.", GetLastError());
		buffer[sizeof(buffer) - 1] = '\0';
		MessageBox(NULL, buffer, "MicroChat", MB_OK);
		ExitProcess(1);
	}

	if (server_mode) {
		SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (server_fd == INVALID_SOCKET) {
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "Socket error: %d.", WSAGetLastError());
			buffer[sizeof(buffer) - 1] = '\0';
			MessageBox(NULL, buffer, "MicroChat", MB_OK);
			WSACleanup();
			return false;
		}

		struct sockaddr_in server_addr = {0};
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = INADDR_ANY;
		server_addr.sin_port = htons(PORT);

		if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "Bind error: %d.", WSAGetLastError());
			buffer[sizeof(buffer) - 1] = '\0';
			MessageBox(NULL, buffer, "MicroChat", MB_OK);
			closesocket(server_fd);
			WSACleanup();
			return false;
		}

		if (listen(server_fd, 1) == SOCKET_ERROR) {
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "Listen error: %d.", WSAGetLastError());
			buffer[sizeof(buffer) - 1] = '\0';
			MessageBox(NULL, buffer, "MicroChat", MB_OK);
			closesocket(server_fd);
			WSACleanup();
			return false;
		}

		// Get IP before creating thread, so the value for dialog will be ready.
		GetDefaultIP(server_ip, sizeof(server_ip));

		// Windows 9x do not allow to start threads with NULL as thread ID (invalid parameter error), so using dummy value.
		// Windows NT do not require thread ID, so it can be NULL.
		DWORD dummyThreadID;
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ShowServerIPMessage, NULL, 0, &dummyThreadID);
		(void)dummyThreadID;

		while (1) {
			struct sockaddr_in client_addr;
			int addr_len = sizeof(client_addr);
			SOCKET temp_client = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
			if (temp_client == INVALID_SOCKET) {
				char buffer[256];
				snprintf(buffer, sizeof(buffer), "Accept error: %d.", WSAGetLastError());
				buffer[sizeof(buffer) - 1] = '\0';
				MessageBox(NULL, buffer, "MicroChat", MB_OK);
				closesocket(server_fd);
				WSACleanup();
				return false;
			}

			client_socket = temp_client;
			strcpy(peer_ip, inet_ntoa(client_addr.sin_addr));
			break;
		}

		closesocket(server_fd);

		int recv_len = recv(client_socket, peer_name, sizeof(peer_name) - 1, 0);
		if (recv_len == SOCKET_ERROR || recv_len == 0) {
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "Peer name error: %d.", WSAGetLastError());
			buffer[sizeof(buffer) - 1] = '\0';
			MessageBox(NULL, buffer, "MicroChat", MB_OK);
			closesocket(client_socket);
			WSACleanup();
			return false;
		}

		peer_name[recv_len] = '\0';
		if (peer_name[0] == '\0') strcpy(peer_name, "<Unknown>");

		send(client_socket, computer_name, strlen(computer_name) + 1, 0);
		ShowMainWindow(hInstance, nCmdShow);

		char sys_msg[256];
		snprintf(sys_msg, sizeof(sys_msg), "%s connected from %s.", peer_name, peer_ip);
		sys_msg[sizeof(sys_msg) - 1] = '\0';
		AddMessage(sys_msg);
	} else {
		client_socket = socket(AF_INET, SOCK_STREAM, 0);
		if (client_socket == INVALID_SOCKET) {
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "Socket error: %d.", WSAGetLastError());
			buffer[sizeof(buffer) - 1] = '\0';
			MessageBox(NULL, buffer, "MicroChat", MB_OK);
			WSACleanup();
			ExitProcess(1);
		}

		struct sockaddr_in server_addr = { 0 };
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(PORT);
		server_addr.sin_addr.s_addr = inet_addr(server_ip);

		if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "Connect error: %d.", WSAGetLastError());
			buffer[sizeof(buffer) - 1] = '\0';
			MessageBox(NULL, buffer, "MicroChat", MB_OK);

			closesocket(client_socket);
			WSACleanup();
			ExitProcess(1);
		}

		// Calling getsockname() after connect modify socket type from "soft" to "hard" bind on Vista+ systems.
		// On pre-Vista systems it also disable "Weak Host Model" (which can display random network interface instead real).
		struct sockaddr_in server_info;
		int len = sizeof(server_info);
		getsockname(client_socket, (struct sockaddr*)&server_info, &len);
		strcpy(peer_ip, inet_ntoa(server_info.sin_addr));

		send(client_socket, computer_name, strlen(computer_name)+1, 0);
		int recv_len = recv(client_socket, peer_name, sizeof(peer_name) - 1, 0);
		peer_name[recv_len] = '\0';
		if (peer_name[0] == '\0') strcpy(peer_name, "<Unknown>");

		ShowMainWindow(hInstance, nCmdShow);

		char sys_msg[256];
		snprintf(sys_msg, sizeof(sys_msg), "Connected to %s.", peer_name);
		sys_msg[sizeof(sys_msg) - 1] = '\0';
		AddMessage(sys_msg);
	}

	// _beginthreadex allows us to wait for thread, own it, close its handle and etc. unlike _beginthread, where it is very primitive (no security, thread owning).
	unsigned int threadID;
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, ReceiveMessages, NULL, 0, &threadID);

	if (hThread == NULL) {
		MessageBox(NULL, "Failed to start receive thread. MicroChat cannot continue.", "MicroChat", MB_OK);
		CleanupAndExit(hwnd_global);
		return false;
	}

	g_hReceiveThread = hThread;

	return true;
}

// This thread runs asynchronously and pushes updates direcly to UI via AddMessage.
// Make sure UI handles are still valid at moment. Not so good practice, but acceptable.
unsigned int __stdcall ReceiveMessages(void* arg) {
	(void)arg;
	char buffer[BUFFER_SIZE];

	while (is_running) {
		if (!is_running) break;
		int bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

		if (bytes == SOCKET_ERROR) {
			// SOCKET_ERROR = -1
			char msg[256] = "Connection with remote computer lost.";
			AddMessage(msg);
			is_running = 0;
			break;
		} else if (bytes == 0) {
			char msg[256] = "Remote computer has closed the connection.";
			AddMessage(msg);
			is_running = 0;
			break;
		}

		buffer[bytes] = '\0';
		MessageBeep(MB_OK);
		AddMessage(buffer);
	}

	return 0;
}

void SendCurrentMessage(HWND hWnd) {
	char buffer[BUFFER_SIZE];
	int text_len = GetWindowText(hEdit, buffer, BUFFER_SIZE - 1);
	buffer[text_len] = '\0';

	// Trim leading spaces, tabs, carets and newlines from start.
	char* start = buffer;
	while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') start++;
	if (start != buffer) {
		char* dst = buffer;
		while ((*dst++ = *start++));
	}

	text_len = strlen(buffer);

	// Trim trailing spaces, tabs, carets and newlines from end.
	while (text_len > 0 && (buffer[text_len - 1] == '\r' || buffer[text_len - 1] == '\n' || buffer[text_len - 1] == '\t' || buffer[text_len - 1] == ' ')) buffer[--text_len] = '\0';

	if (text_len > 0) {
		char full_msg[BUFFER_SIZE + 128];
		snprintf(full_msg, sizeof(full_msg), "%s: %s", computer_name, buffer);
		full_msg[sizeof(full_msg) - 1] = '\0';
		AddMessage(full_msg);

		int sent = send(client_socket, full_msg, strlen(full_msg), 0);
		if (sent == SOCKET_ERROR) {
			char buffer[64];
			snprintf(buffer, sizeof(buffer), "Send failed: %d", WSAGetLastError());
			buffer[sizeof(buffer) - 1] = '\0';
			AddMessage(buffer);
		}
	}

	SetWindowTextA(hEdit, "");

	MSG nextMsg;
	while (PeekMessage(&nextMsg, hWnd, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)) {
		if (nextMsg.message == WM_KEYDOWN && nextMsg.wParam == VK_RETURN) continue;
		DispatchMessage(&nextMsg);
	}
}

// === 8. UI / Window procedures ===
INT_PTR CALLBACK ConnectDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	(void)lParam;

	switch (msg) {
		case WM_INITDIALOG:
			SetFocus(GetDlgItem(hwnd, IDC_IP));
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK: {
					char ip[16];
					GetDlgItemText(hwnd, IDC_IP, ip, sizeof(ip));
					if (ip[0] == '\0') {
						MessageBox(hwnd, "Please enter a server IP.", "MicroChat", MB_OK);
						return TRUE;
					}

					if (inet_addr(ip) == INADDR_NONE) {
						MessageBox(hwnd, "Invalid IP address.\nExample: 192.168.1.100", "MicroChat", MB_OK);
						return TRUE;
					}

					strncpy(server_ip, ip, sizeof(server_ip) - 1);
					server_ip[sizeof(server_ip) - 1] = '\0';
					EndDialog(hwnd, IDOK);
					return TRUE;
				}

				case IDCANCEL:
					EndDialog(hwnd, IDCANCEL);
					return TRUE;
			}
			return TRUE;
	}

	return FALSE;
}

LRESULT CALLBACK EditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_GETDLGCODE) {
		return DLGC_WANTALLKEYS | CallWindowProc(g_oldEditProc, hWnd, uMsg, wParam, lParam);
	} else if (uMsg == WM_KEYDOWN) {
		// EDIT control do not have native Select All.
		// This is a Windows limitation - needed to handle it manually.
		if (wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
			SendMessage(hWnd, EM_SETSEL, 0, -1);
			return 0;
		}

		// Acting like a normal messengers - Send on Enter, Shift+Enter for new line.
		if (wParam == VK_RETURN) {
			if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
				return CallWindowProc(g_oldEditProc, hWnd, uMsg, wParam, lParam);
			} else {
				PostMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(ID_SEND, 0), 0);
				return 0;
			}
		}
	}

	return CallWindowProc(g_oldEditProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_CREATE: {
			// MS Shell Dlg is a font alias that resolves to the system default font:
			//   Western - Microsoft Sans Serif.
			//   Japanese - MS UI Gothic.
			//   Korean - Gulim.
			//   Chinese - SimSun.
			//
			// Font size: -12 is not 12pt - it is 9pt (minus sign means "height in device units").
			// In practice, this often renders as 8pt Microsoft Sans Serif on standard DPI.
			// To force a specific font size, use a positive font size.
			hFont = CreateFont(
				-12, 0, 0, 0, FW_NORMAL,
				FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
				"MS Shell Dlg"
			);

			hFontBold = CreateFont(
				-12, 0, 0, 0, FW_BOLD,
				FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
				"MS Shell Dlg"
			);

			hMsgDisplay = CreateWindow(
				"EDIT", "",
				WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
				ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
				0, 0, 520, 240,
				hWnd, (HMENU)ID_MSG_DISPLAY, NULL, NULL
			);

			hEdit = CreateWindow(
				"EDIT", "", 
				WS_CHILD | WS_VISIBLE | WS_BORDER |
				ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL,
				0, 240, 430, 30,
				hWnd, (HMENU)ID_EDIT, NULL, NULL);

			hSendBtn = CreateWindow(
				"BUTTON", "Send", 
				WS_CHILD | WS_VISIBLE |
				BS_PUSHBUTTON,
				430, 240, 60, 40,
				hWnd, (HMENU)ID_SEND, NULL, NULL);

			SendMessage(hMsgDisplay, WM_SETFONT, (WPARAM)hFont, TRUE);
			SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
			SendMessage(hSendBtn, WM_SETFONT, (WPARAM)hFontBold, TRUE);

			// Zeroing error code and replacing default key handler. Needed to handle custom ENTER and CTRL+A functions
			SetLastError(0);
			g_oldEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)EditProc);
			if (!g_oldEditProc && GetLastError() != 0) {
				AddMessage("Warning: Edit subclass setup failed. Enter and Ctrl+A may not work as expected.");
			}
			return 0;
		}

		case WM_COMMAND: {
			if (LOWORD(wParam) == ID_SEND) {
				SendCurrentMessage(hWnd);
				SetFocus(hEdit);
			}
			return 0;
		}

		case WM_CLOSE: {
			Disconnect();
			return 0;
		}

		case WM_DESTROY: {
			// First cleanup fonts to prevent GDI leak.
			DeleteObject(hFont);
			DeleteObject(hFontBold);

			// Then set its handles to NULL.
			hFont = NULL;
			hFontBold = NULL;

			CleanupAndExit(hWnd);
			return 0;
		}
		
		case WM_SIZE: {
			int w = LOWORD(lParam);
			int h = HIWORD(lParam);

			int edit_height = 30;
			int send_width = 60;

			SetWindowPos(hMsgDisplay, NULL, 0, 0, w, h - edit_height, SWP_NOZORDER);
			SetWindowPos(hEdit, NULL, 0, h - edit_height, w - send_width, edit_height, SWP_NOZORDER);
			SetWindowPos(hSendBtn, NULL, w - send_width, h - edit_height, send_width, edit_height, SWP_NOZORDER);
			return 0;
		}
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void ShowMainWindow(HINSTANCE hInstance, int nCmdShow) {
	WNDCLASS wc = {0};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = "MicroChatWndClass";
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION); // Generic application icon from user32.dll
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	RegisterClass(&wc);

	char title[256];
	snprintf(title, sizeof(title), "MicroChat - %s", peer_name);
	title[sizeof(title) - 1] = '\0';
	
	HWND hWnd = CreateWindow("MicroChatWndClass", title,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME | WS_MAXIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 520, 320,
		NULL, NULL, hInstance, NULL);

	// Here using ExitProcess because this event is critical.
	// CreateWindow failing is very rare but non-zero chance.
	if (!hWnd) {
		MessageBox(NULL, "Failed to create main window.", "MicroChat", MB_OK);
		ExitProcess(1);
	}

	hwnd_global = hWnd;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
}

// This is a purely informational dialog - it shows server IP
// so user can share it with others. It is not required for core logic.
// Because MicroChat do not have gethostbyname fallback (outdated method), UDP hack will return 0.0.0.0.
// Do not try to connect to 0.0.0.0 since it always fails (works only as source, not destination).
DWORD WINAPI ShowServerIPMessage(LPVOID lpParam) {
	(void)lpParam;

	char buffer[256];
	snprintf(buffer, sizeof(buffer), 
		"Server IP: %s\n"
		"Share with users to connect to server.", 
		server_ip);
	buffer[sizeof(buffer) - 1] = '\0';

	MessageBox(NULL, buffer, "MicroChat", MB_OK);
	return 0;
}