// Module: Networking (network.c)
// Part of MicroChat: https://github.com/WinXP655/microchat
#include "microchat.h"

SOCKET client_socket = INVALID_SOCKET;
HANDLE receive_thread = NULL;
wchar_t server_ip[16] = L"";
wchar_t peer_ip[16] = L"";
wchar_t peer_name[256] = L"";
wchar_t computer_name[256] = L"";

bool is_server = false;
volatile LONG is_running = TRUE;

bool InitializeNetwork(bool server_mode, HINSTANCE hInstance, int nCmdShow) {
	// Moved getting local computer name to InitializeNetwork since it didn't had sense in wWinMain.
	GetLocalComputerName();

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		ShowError(L"WSAStartup failed", WSAGetLastError());
		return false;
	}

	bool result = server_mode ? StartServer(hInstance, nCmdShow) : StartClient(hInstance, nCmdShow);

	if (!result) {
		WSACleanup();
		return false;
	}

	// _beginthreadex allows us to wait for thread, own it, close its handle and etc. unlike _beginthread, where it is very primitive (no security, thread owning).
	unsigned int threadID;
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, ReceiveMessages, NULL, 0, &threadID);

	if (hThread == NULL) {
		MessageBoxW(NULL, L"Failed to start receive thread.", L"MicroChat", MB_OK);
		CleanupAndExit();
		return false;
	}

	receive_thread = hThread;

	return true;
}

bool StartServer(HINSTANCE hInstance, int nCmdShow) {
	SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == INVALID_SOCKET) {
		ShowError(L"Socket failed", WSAGetLastError());
		return false;
	}

	struct sockaddr_in server_addr = {0};
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		ShowError(L"Bind failed", WSAGetLastError());
		closesocket(server_fd);
		return false;
	}

	if (listen(server_fd, 1) == SOCKET_ERROR) {
		closesocket(server_fd);
		return false;
	}

	// Get IP before creating thread, so the value for dialog will be ready.
	GetDefaultIP(server_ip, sizeof(server_ip) / sizeof(wchar_t));
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ShowServerIpMessage, NULL, 0, NULL);

	while (1) {
		struct sockaddr_in client_addr;
		int addr_len = sizeof(client_addr);
		SOCKET temp_client = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
		if (temp_client == INVALID_SOCKET) {
			DWORD err = WSAGetLastError();
			if (err == WSAEINTR || err == WSAENOTSOCK) {
				break;
			}
			Sleep(100);
			continue;
		}

		struct timeval tv;
		tv.tv_sec  = NET_TIMEOUT_SEC;
		tv.tv_usec = NET_TIMEOUT_USEC;
		setsockopt(temp_client, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));

		client_socket = temp_client;

		// Convert IP from char* to wchar_t
		char ip_utf8[16];
		strncpy(ip_utf8, inet_ntoa(client_addr.sin_addr), 15);
		ip_utf8[15] = '\0';
		MultiByteToWideChar(CP_UTF8, 0, ip_utf8, -1, peer_ip, sizeof(peer_ip) / sizeof(wchar_t));

		break;
	}

	closesocket(server_fd);

	// Receive peer name (UTF-8 -> wchar_t)
	char peer_name_utf8[256];
	int recv_len = recv(client_socket, peer_name_utf8, sizeof(peer_name_utf8) - 1, 0);
	if (recv_len <= 0) {
		ShowError(L"Peer name failed", WSAGetLastError());
		closesocket(client_socket);
		client_socket = INVALID_SOCKET;
		return false;
	}

	peer_name_utf8[recv_len] = '\0';
	MultiByteToWideChar(CP_UTF8, 0, peer_name_utf8, -1, peer_name, sizeof(peer_name) / sizeof(wchar_t));
	if (peer_name[0] == L'\0') wcscpy(peer_name, L"<Unknown>");

	// Send computer name (wchar_t -> UTF-8)
	char computer_name_utf8[256];
	WideCharToMultiByte(CP_UTF8, 0, computer_name, -1, computer_name_utf8, sizeof(computer_name_utf8), NULL, NULL);
	send(client_socket, computer_name_utf8, strlen(computer_name_utf8) + 1, 0);

	ShowMainWindow(hInstance, nCmdShow);

	wchar_t sys_msg[512];
	swprintf(sys_msg, sizeof(sys_msg) / sizeof(wchar_t), L"%ls connected from %ls.", peer_name, peer_ip);
	AddMessage(sys_msg);
	return true;
}

bool StartClient(HINSTANCE hInstance, int nCmdShow) {
	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket == INVALID_SOCKET) {
		ShowError(L"Socket failed", WSAGetLastError());
		return false;
	}

	struct sockaddr_in server_addr = { 0 };
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);

	// Convert server_ip (wchar_t) to char* for inet_addr
	char server_ip_utf8[16];
	WideCharToMultiByte(CP_UTF8, 0, server_ip, -1, server_ip_utf8, sizeof(server_ip_utf8), NULL, NULL);
	server_addr.sin_addr.s_addr = inet_addr(server_ip_utf8);

	if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		ShowError(L"Connect failed", WSAGetLastError());
		closesocket(client_socket);
		client_socket = INVALID_SOCKET;
		return false;
	}

	// Calling getsockname() after connect modify socket type from "soft" to "hard" bind on Vista+ systems.
	// On pre-Vista systems it also disable "Weak Host Model" (which can display random network interface instead real).
	struct sockaddr_in server_info;
	int len = sizeof(server_info);
	if (getsockname(client_socket, (struct sockaddr*)&server_info, &len) != 0) {
		wcscpy(peer_ip, L"<Unknown>");
	} else {
		char ip_utf8[16];
		strncpy(ip_utf8, inet_ntoa(server_info.sin_addr), 15);
		ip_utf8[15] = '\0';
		MultiByteToWideChar(CP_UTF8, 0, ip_utf8, -1, peer_ip, sizeof(peer_ip) / sizeof(wchar_t));
	}

	// Send computer name (wchar_t -> UTF-8)
	char computer_name_utf8[256];
	WideCharToMultiByte(CP_UTF8, 0, computer_name, -1, computer_name_utf8, sizeof(computer_name_utf8), NULL, NULL);
	send(client_socket, computer_name_utf8, strlen(computer_name_utf8) + 1, 0);

	struct timeval tv;
	tv.tv_sec = NET_TIMEOUT_SEC;
	tv.tv_usec = NET_TIMEOUT_USEC;
	setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));

	// Receive peer name (UTF-8 -> wchar_t)
	char peer_name_utf8[256];
	int recv_len = recv(client_socket, peer_name_utf8, sizeof(peer_name_utf8) - 1, 0);
	if (recv_len <= 0) {
		ShowError(L"Peer name failed", WSAGetLastError());
		closesocket(client_socket);
		client_socket = INVALID_SOCKET;
		return false;
	}

	peer_name_utf8[recv_len] = '\0';
	MultiByteToWideChar(CP_UTF8, 0, peer_name_utf8, -1, peer_name, sizeof(peer_name) / sizeof(wchar_t));
	if (peer_name[0] == L'\0') wcscpy(peer_name, L"<Unknown>");

	ShowMainWindow(hInstance, nCmdShow);

	wchar_t sys_msg[512];
	swprintf(sys_msg, sizeof(sys_msg) / sizeof(wchar_t), L"Connected to %ls.", peer_name);
	AddMessage(sys_msg);
	return true;
}

// This thread runs asynchronously and pushes updates direcly to UI via AddMessage.
// Make sure UI handles are still valid at moment. Not so good practice, but acceptable.
unsigned int __stdcall ReceiveMessages(void* arg) {
	(void)arg;
	char buffer[BUFFER_SIZE];

	while (InterlockedCompareExchange(&is_running, 1, 1) != 0) {
		int bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

		if (bytes == SOCKET_ERROR) {
			int err = WSAGetLastError();
			if (err == WSAETIMEDOUT) continue;

			wchar_t connlost_err[512];
			swprintf(connlost_err, sizeof(connlost_err) / sizeof(wchar_t), L"Connection with remote computer lost. Error: %d.", err);

			UIAddMessage(connlost_err);
			InterlockedExchange(&is_running, 0);
			break;
		}

		if (bytes == 0) {
			UIAddMessage(L"Remote computer has closed the connection.");
			InterlockedExchange(&is_running, 0);
			break;
		}

		// Since UTF-8 can be more than 1-2 bytes per char, we need to trim carefully.
		int b = TrimIncompleteUTF8(buffer, bytes);
		buffer[b] = '\0';

		// Convert received buffer to Unicode for display.
		wchar_t wbuffer[BUFFER_SIZE];
		if (MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wbuffer, BUFFER_SIZE) <= 0) continue;

		MessageBeep(MB_OK);
		UIAddMessage(wbuffer);
	}

	return 0;
}

void SendCurrentMessage(HWND hWnd) {
	// Counting characters including formatting.
	int msglen = GetWindowTextLengthW(hEdit);
	int maxallowed = (BUFFER_SIZE - 1) - wcslen(computer_name) - 4;
	if (maxallowed < 1) maxallowed = 1;

	if (msglen > maxallowed) {
		wchar_t toolong_err[512] = L"Message is too long to send.";
		AddMessage(toolong_err);
		return;
	}
	
	wchar_t buffer[BUFFER_SIZE];
	int text_len = GetWindowTextW(hEdit, buffer, BUFFER_SIZE - 1);
	buffer[text_len] = L'\0';

	wchar_t* trimmed = Trim(buffer);
	if (*trimmed != L'\0') {
		wchar_t full_msg[BUFFER_SIZE + 128];
		swprintf(full_msg, sizeof(full_msg) / sizeof(wchar_t), L"%ls: %ls", computer_name, trimmed);
		AddMessage(full_msg);

		// Convert to UTF-8 for transmission
		char utf8_buffer[BUFFER_SIZE + 128];
		int msg_len = WideCharToMultiByte(CP_UTF8, 0, full_msg, -1, utf8_buffer, sizeof(utf8_buffer), NULL, NULL);
		if (msg_len <= 1) return;   // Do not send garbage when failed to convert

		int sent = send(client_socket, utf8_buffer, msg_len - 1, 0);
		if (sent == SOCKET_ERROR) {
			wchar_t error_msg[512];
			swprintf(error_msg, sizeof(error_msg) / sizeof(wchar_t), L"Send failed: %d", WSAGetLastError());
			AddMessage(error_msg);
		}
	}

	SetWindowTextW(hEdit, L"");

	// Remove any lingering Enter key messages.
	MSG nextMsg;
	while (PeekMessage(&nextMsg, hWnd, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)) {
		if (nextMsg.message == WM_KEYDOWN && nextMsg.wParam == VK_RETURN) continue;
		DispatchMessage(&nextMsg);
	}
}

void Disconnect(void) {
	wchar_t leave_msg[512];
	swprintf(leave_msg, sizeof(leave_msg) / sizeof(wchar_t), L"%ls left the chat.", computer_name);
	AddMessage(leave_msg);

	if (client_socket != INVALID_SOCKET) {
		// Convert wchar_t to UTF-8 for network
		char utf8_msg[512];
		WideCharToMultiByte(CP_UTF8, 0, leave_msg, -1, utf8_msg, sizeof(utf8_msg), NULL, NULL);
		send(client_socket, utf8_msg, strlen(utf8_msg), 0);
	}

	CleanupAndExit();
}