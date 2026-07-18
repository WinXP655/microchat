#include "microchat.h"

SOCKET clientSocket = INVALID_SOCKET;
HANDLE receiveThread = NULL;
wchar_t serverIp[16] = L"";
wchar_t peerIp[16] = L"";
wchar_t peerName[256] = L"";
wchar_t computerName[256] = L"";

bool isServer = false;
int isRunning = 1;

bool InitializeNetwork(bool server_mode, HINSTANCE hInstance, int nCmdShow) {
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		ShowError(L"WSAStartup failed", WSAGetLastError());
		ExitProcess(1);
	}

	if (server_mode) {
		SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (server_fd == INVALID_SOCKET) {
			ShowError(L"Socket failed", WSAGetLastError());
			WSACleanup();
			return false;
		}

		struct sockaddr_in server_addr = {0};
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = INADDR_ANY;
		server_addr.sin_port = htons(PORT);

		if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
			ShowError(L"Bind failed", WSAGetLastError());
			closesocket(server_fd);
			WSACleanup();
			return false;
		}

		if (listen(server_fd, 1) == SOCKET_ERROR) {
			ShowError(L"Listen failed", WSAGetLastError());
			closesocket(server_fd);
			WSACleanup();
			return false;
		}

		// Get IP before creating thread, so the value for dialog will be ready.
		GetDefaultIP(serverIp, sizeof(serverIp) / sizeof(wchar_t));

		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ShowServerIPMessage, NULL, 0, NULL);

		while (1) {
			struct sockaddr_in client_addr;
			int addr_len = sizeof(client_addr);
			SOCKET temp_client = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
			if (temp_client == INVALID_SOCKET) {
				ShowError(L"Listen failed", WSAGetLastError());
				closesocket(server_fd);
				WSACleanup();
				return false;
			}

			clientSocket = temp_client;

			// Convert IP from char* to wchar_t
			char ip_utf8[16];
			strncpy(ip_utf8, inet_ntoa(client_addr.sin_addr), 15);
			ip_utf8[15] = '\0';
			MultiByteToWideChar(CP_UTF8, 0, ip_utf8, -1, peerIp, sizeof(peerIp) / sizeof(wchar_t));

			break;
		}

		closesocket(server_fd);

		// Receive peer name (UTF-8 -> wchar_t)
		char peerName_utf8[256];
		int recv_len = recv(clientSocket, peerName_utf8, sizeof(peerName_utf8) - 1, 0);
		if (recv_len == SOCKET_ERROR || recv_len == 0) {
			ShowError(L"Peer name failed", WSAGetLastError());
			closesocket(clientSocket);
			WSACleanup();
			return false;
		}

		peerName_utf8[recv_len] = '\0';
		MultiByteToWideChar(CP_UTF8, 0, peerName_utf8, -1, peerName, sizeof(peerName) / sizeof(wchar_t));
		if (peerName[0] == L'\0') wcscpy(peerName, L"<Unknown>");

		// Send computer name (wchar_t -> UTF-8)
		char computerName_utf8[256];
		WideCharToMultiByte(CP_UTF8, 0, computerName, -1, computerName_utf8, sizeof(computerName_utf8), NULL, NULL);
		send(clientSocket, computerName_utf8, strlen(computerName_utf8) + 1, 0);

		ShowMainWindow(hInstance, nCmdShow);

		wchar_t sys_msg[512];
		swprintf(sys_msg, sizeof(sys_msg) / sizeof(wchar_t), L"%ls connected from %ls.", peerName, peerIp);
		AddMessage(sys_msg);
	} else {
		clientSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (clientSocket == INVALID_SOCKET) {
			ShowError(L"Socket failed", WSAGetLastError());
			WSACleanup();
			ExitProcess(1);
		}

		struct sockaddr_in server_addr = { 0 };
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(PORT);

		// Convert serverIp (wchar_t) to char* for inet_addr
		char serverIp_utf8[16];
		WideCharToMultiByte(CP_UTF8, 0, serverIp, -1, serverIp_utf8, sizeof(serverIp_utf8), NULL, NULL);
		server_addr.sin_addr.s_addr = inet_addr(serverIp_utf8);

		if (connect(clientSocket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
			ShowError(L"Connect failed", WSAGetLastError());
			closesocket(clientSocket);
			WSACleanup();
			ExitProcess(1);
		}

		// Calling getsockname() after connect modify socket type from "soft" to "hard" bind on Vista+ systems.
		// On pre-Vista systems it also disable "Weak Host Model" (which can display random network interface instead real).
		struct sockaddr_in server_info;
		int len = sizeof(server_info);
		getsockname(clientSocket, (struct sockaddr*)&server_info, &len);

		char ip_utf8[16];
		strncpy(ip_utf8, inet_ntoa(server_info.sin_addr), 15);
		ip_utf8[15] = '\0';
		MultiByteToWideChar(CP_UTF8, 0, ip_utf8, -1, peerIp, sizeof(peerIp) / sizeof(wchar_t));

		// Send computer name (wchar_t -> UTF-8)
		char computerName_utf8[256];
		WideCharToMultiByte(CP_UTF8, 0, computerName, -1, computerName_utf8, sizeof(computerName_utf8), NULL, NULL);
		send(clientSocket, computerName_utf8, strlen(computerName_utf8) + 1, 0);

		// Receive peer name (UTF-8 -> wchar_t)
		char peerName_utf8[256];
		int recv_len = recv(clientSocket, peerName_utf8, sizeof(peerName_utf8) - 1, 0);
		peerName_utf8[recv_len] = '\0';
		MultiByteToWideChar(CP_UTF8, 0, peerName_utf8, -1, peerName, sizeof(peerName) / sizeof(wchar_t));
		if (peerName[0] == L'\0') wcscpy(peerName, L"<Unknown>");

		ShowMainWindow(hInstance, nCmdShow);

		wchar_t sys_msg[512];
		swprintf(sys_msg, sizeof(sys_msg) / sizeof(wchar_t), L"Connected to %ls.", peerName);
		AddMessage(sys_msg);
	}

	// _beginthreadex allows us to wait for thread, own it, close its handle and etc. unlike _beginthread, where it is very primitive (no security, thread owning).
	unsigned int threadID;
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, ReceiveMessages, NULL, 0, &threadID);

	if (hThread == NULL) {
		MessageBoxW(NULL, L"Failed to start receive thread.", L"MicroChat", MB_OK);
		CleanupAndExit();
		return false;
	}

	receiveThread = hThread;

	return true;
}

// This thread runs asynchronously and pushes updates direcly to UI via AddMessage.
// Make sure UI handles are still valid at moment. Not so good practice, but acceptable.
unsigned int __stdcall ReceiveMessages(void* arg) {
	(void)arg;
	char buffer[BUFFER_SIZE];

	while (isRunning) {
		if (!isRunning) break;
		int bytes = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);

		if (bytes == SOCKET_ERROR) {
			wchar_t msg[256] = L"Connection with remote computer lost.";
			AddMessage(msg);
			isRunning = 0;
			break;
		} else if (bytes == 0) {
			wchar_t msg[256] = L"Remote computer has closed the connection.";
			AddMessage(msg);
			isRunning = 0;
			break;
		}

		buffer[bytes] = '\0';

		// Convert received buffer to Unicode for display
		wchar_t wbuffer[BUFFER_SIZE];
		MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wbuffer, BUFFER_SIZE);

		MessageBeep(MB_OK);
		AddMessage(wbuffer);
	}

	return 0;
}

void SendCurrentMessage(HWND hWnd) {
	// Counting characters including formatting.
	int msglen = GetWindowTextLengthW(hEdit);
	int maxallowed = (BUFFER_SIZE - 1) - wcslen(computerName) - 4;
	if (msglen > maxallowed) {
		wchar_t toolong_err[512] = L"Message is too long to send.";
		AddMessage(toolong_err);
		return;
	}
	
	wchar_t buffer[BUFFER_SIZE];
	int text_len = GetWindowTextW(hEdit, buffer, BUFFER_SIZE - 1);
	buffer[text_len] = L'\0';

	// Trim leading spaces, tabs, carets and newlines from start.
	wchar_t* start = buffer;
	while (*start == L' ' || *start == L'\t' || *start == L'\r' || *start == L'\n') start++;
	if (start != buffer) {
		wchar_t* dst = buffer;
		while ((*dst++ = *start++));
	}

	text_len = wcslen(buffer);

	// Trim trailing spaces, tabs, carets and newlines from end.
	while (text_len > 0 && (buffer[text_len - 1] == L'\r' || buffer[text_len - 1] == L'\n' || buffer[text_len - 1] == L'\t' || buffer[text_len - 1] == L' ')) buffer[--text_len] = L'\0';

	if (text_len > 0) {
		wchar_t full_msg[BUFFER_SIZE + 128];
		swprintf(full_msg, sizeof(full_msg) / sizeof(wchar_t), L"%ls: %ls", computerName, buffer);
		AddMessage(full_msg);

		// Convert to UTF-8 for transmission
		char utf8_buffer[BUFFER_SIZE + 128];
		WideCharToMultiByte(CP_UTF8, 0, full_msg, -1, utf8_buffer, sizeof(utf8_buffer), NULL, NULL);

		int sent = send(clientSocket, utf8_buffer, strlen(utf8_buffer), 0);
		if (sent == SOCKET_ERROR) {
			wchar_t error_msg[64];
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

void Disconnect() {
	wchar_t leave_msg[512];
	swprintf(leave_msg, sizeof(leave_msg) / sizeof(wchar_t), L"%ls left the chat.", computerName);

	AddMessage(leave_msg);
	if (clientSocket != INVALID_SOCKET) {
		// Convert wchar_t to UTF-8 for network
		char utf8_msg[512];
		WideCharToMultiByte(CP_UTF8, 0, leave_msg, -1, utf8_msg, sizeof(utf8_msg), NULL, NULL);
		send(clientSocket, utf8_msg, strlen(utf8_msg), 0);
	}

	CleanupAndExit();
}