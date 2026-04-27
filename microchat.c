// MicroChat Framework by WinXP655
// Distributed under MIT License

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <stdbool.h>

#define PORT 1723
#define BUFFER_SIZE 4096
#define ID_EDIT 101
#define ID_SEND 102
#define IDC_IP 1001
#define ID_MSG_DISPLAY 105

SOCKET client_socket = INVALID_SOCKET;
HWND hEdit, hSendBtn, hMsgDisplay;
int is_running = 1;
char server_ip[16] = "";
char computer_name[MAX_COMPUTERNAME_LENGTH + 1];
char peer_name[MAX_COMPUTERNAME_LENGTH + 1];
char peer_ip[16] = "";
bool is_server = false;
WNDPROC g_oldEditProc = NULL;
HWND hwnd_global = NULL;
HFONT hFont = NULL;
HFONT hFontBold = NULL;

void ReceiveMessages(void* arg);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK ConnectDialogProc(HWND, UINT, WPARAM, LPARAM);
bool InitializeNetwork(bool server_mode, HINSTANCE hInstance, int nCmdShow);
void AddMessage(const char* msg);
void ShowMainWindow(HINSTANCE hInstance, int nCmdShow);
void GetLocalComputerName();
void CleanupAndExit(HWND hWnd);
DWORD WINAPI ShowServerIPMessage(LPVOID lpParam);
void SendCurrentMessage(HWND hWnd);
void Disconnect();

bool GetDefaultIP(char *ip_buffer, size_t size) {
	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == INVALID_SOCKET) return false;

	struct sockaddr_in remote = {0};
	remote.sin_family = AF_INET;
	remote.sin_port = htons(53);
	remote.sin_addr.s_addr = inet_addr("8.8.8.8");

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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	(void)hPrevInstance;
	(void)lpCmdLine;
	GetLocalComputerName();

	int mode = MessageBox(NULL,
		"Start MicroChat as:\nYes - Chat Host\nNo - Chat Client\nCancel - Exit",
		"MicroChat",
		MB_YESNOCANCEL);

	if (mode == IDCANCEL) return 0;
	is_server = (mode == IDYES);

	if (is_server) {
		if(!InitializeNetwork(true, hInstance, nCmdShow)) return 0;
	} else {
		INT_PTR dlg = DialogBoxParam(hInstance, MAKEINTRESOURCE(1), NULL, ConnectDialogProc, 0);
		
		if (dlg == -1) {
			MessageBox(NULL, "Could not load connection dialog.", "MicroChat", MB_OK);
			return 0;
		}
		
		if (dlg != IDOK) return 0;
		if(!InitializeNetwork(false, hInstance, nCmdShow)) return 0;
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

void GetLocalComputerName() {
	DWORD size = sizeof(computer_name);
	GetComputerNameA(computer_name, &size);
}

void ShowMainWindow(HINSTANCE hInstance, int nCmdShow) {
	WNDCLASS wc = {0};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = "MicroChatWndClass";
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	RegisterClass(&wc);

	char title[256];
	snprintf(title, sizeof(title), "%s", peer_name);
	title[sizeof(title) - 1] = '\0';
	
	HWND hWnd = CreateWindow("MicroChatWndClass", title,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME | WS_MAXIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 520, 320,
		NULL, NULL, hInstance, NULL);
	if (!hWnd) {
		MessageBox(NULL, "Failed to create main window.", "MicroChat", MB_OK);
		ExitProcess(1);
	}

	hwnd_global = hWnd;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
}

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

LRESULT CALLBACK EditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_GETDLGCODE) {
		return DLGC_WANTALLKEYS | CallWindowProc(g_oldEditProc, hWnd, uMsg, wParam, lParam);
	} else if (uMsg == WM_KEYDOWN) {
		if (wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
			SendMessage(hWnd, EM_SETSEL, 0, -1);
			return 0;
		}

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

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_CREATE: {
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

			// Replacing default key handler. Needed to handle custom ENTER functions
			// Before EDIT will even create new line.
			SetLastError(0);
			g_oldEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)EditProc);
			if (!g_oldEditProc && GetLastError() != 0) {
				AddMessage("Warning: edit subclass setup failed. Enter and Ctrl+A may not work as expected.");
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

		case WM_CLOSE:
			Disconnect();
			return 0;

		case WM_DESTROY: {
			DeleteObject(hFont);
			DeleteObject(hFontBold);
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

void Disconnect() {
	char leave_msg[256];
	snprintf(leave_msg, sizeof(leave_msg), "%s left the chat.", computer_name);
	if (client_socket != INVALID_SOCKET) send(client_socket, leave_msg, strlen(leave_msg), 0);
	AddMessage(leave_msg);
	CleanupAndExit(hwnd_global);
}

void CleanupAndExit(HWND hWnd) {
	(void)hWnd;
	is_running = 0;

	// Giving some time for ReceiveMessages to leave recv().
	Sleep(50);
	
	if (client_socket != INVALID_SOCKET) {
		shutdown(client_socket, SD_BOTH);
		closesocket(client_socket);
		client_socket = INVALID_SOCKET;
	}
	
	WSACleanup();
	PostQuitMessage(0);
}

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

		struct sockaddr_in server_addr = { 0 };
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

		GetDefaultIP(server_ip, sizeof(server_ip));
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ShowServerIPMessage, NULL, 0, NULL);

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
		// On pre-Vista systems it also disable "Weak Host Model".
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

	// _beginthread() uses CRT, critical if it will be run on non-NT systems
	// and also CreateThread is not working on Windows 9x systems because not initialized by CRT
	uintptr_t thread_handle = _beginthread(ReceiveMessages, 0, NULL);
	if (thread_handle == (uintptr_t)-1L) {
		MessageBox(NULL, "Failed to start receive thread. MicroChat will close.", "MicroChat", MB_OK);
		CleanupAndExit(hwnd_global);
		return false;
	}
	return true;
}

void ReceiveMessages(void* arg) {
	(void)arg;
	char buffer[BUFFER_SIZE];
	while (is_running) {
		int bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
		if (!is_running) break;

		if (bytes == SOCKET_ERROR) {
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
}

void SendCurrentMessage(HWND hWnd) {
	char buffer[BUFFER_SIZE];
	int text_len = GetWindowTextA(hEdit, buffer, BUFFER_SIZE - 1);
	buffer[text_len] = '\0';

	while (text_len > 0 && (buffer[text_len - 1] == '\r' || buffer[text_len - 1] == '\n')) buffer[--text_len] = '\0';

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

void AddMessage(const char* msg) {
	if (!hMsgDisplay || !IsWindow(hMsgDisplay) || !msg || !*msg) return;
	int len = GetWindowTextLength(hMsgDisplay);
	
	SendMessage(hMsgDisplay, EM_SETSEL, len, len);

	// Using EM_REPLACESEL instead SetWindowText to add string without overwriting entire buffer.
	if (len > 0) SendMessage(hMsgDisplay, EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
	SendMessage(hMsgDisplay, EM_REPLACESEL, FALSE, (LPARAM)msg);
	SendMessage(hMsgDisplay, WM_VSCROLL, SB_BOTTOM, 0);
}
