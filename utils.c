#include "microchat.h"

void GetLocalComputerName(void) {
	DWORD size = sizeof(computer_name) / sizeof(wchar_t);
	GetComputerNameW(computer_name, &size);
}

bool GetDefaultIP(wchar_t *ip_buffer, size_t size) {
	// Note: this hack works only on Windows 2000 and higher.

	SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == INVALID_SOCKET) return false;

	struct sockaddr_in remote = {0};
	remote.sin_family = AF_INET;
	remote.sin_port = htons(53);
	remote.sin_addr.s_addr = inet_addr("8.8.8.8"); // Google DNS, but works with any reachable IP.

	// UDP hack: connect() on UDP keep address in memory...
	if (connect(s, (struct sockaddr*)&remote, sizeof(remote)) != 0) {
		closesocket(s);
		return false;
	}

	// ... and then getsockname() return local IP address which will be displayed.
	struct sockaddr_in local;
	int len = sizeof(local);
	if (getsockname(s, (struct sockaddr*)&local, &len) != 0) {
		closesocket(s);
		return false;
	}

	closesocket(s);

	char ip_utf8[16];
	strncpy(ip_utf8, inet_ntoa(local.sin_addr), 15);
	ip_utf8[15] = '\0';
	MultiByteToWideChar(CP_UTF8, 0, ip_utf8, -1, ip_buffer, size);

	return true;
}

void AddMessage(const wchar_t* msg) {
	if (!hMsgDisplay || !msg || !*msg) return;

	const wchar_t* to_display = msg;
	wchar_t truncated[BUFFER_SIZE];

	if (wcslen(msg) >= BUFFER_SIZE) {
		wcsncpy(truncated, msg, BUFFER_SIZE - 20);
		truncated[BUFFER_SIZE - 20] = L'\0';
		wcscat(truncated, L"... [truncated]");
		to_display = truncated;
	}

	int len = GetWindowTextLengthW(hMsgDisplay);
	SendMessageW(hMsgDisplay, EM_SETSEL, len, len);

	// Using EM_REPLACESEL instead SetWindowText to add string without overwriting entire buffer.
	if (len > 0) SendMessageW(hMsgDisplay, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
	SendMessageW(hMsgDisplay, EM_REPLACESEL, FALSE, (LPARAM)to_display);
	SendMessageW(hMsgDisplay, WM_VSCROLL, SB_BOTTOM, 0);
}

void CleanupAndExit(void) {
	is_running = 0;

	if (client_socket != INVALID_SOCKET) {
		shutdown(client_socket, SD_BOTH);
		closesocket(client_socket);
		client_socket = INVALID_SOCKET;
	}

	// WaitFotSingleObject can be used on recv thread without hang
	// in this order: shutdown - wait - close socket
	if (receive_thread != NULL) {
		WaitForSingleObject(receive_thread, 3000);
		CloseHandle(receive_thread);
		receive_thread = NULL;
	}

	if (client_socket != INVALID_SOCKET) {
		closesocket(client_socket);
		client_socket = INVALID_SOCKET;
	}

	WSACleanup();
	PostQuitMessage(0);
}

void ShowError(const wchar_t* msg, DWORD err) {
	wchar_t buffer[512];
	swprintf(buffer, sizeof(buffer) / sizeof(wchar_t), L"%ls: %lu", msg, err);
	MessageBoxW(NULL, buffer, L"MicroChat", MB_OK);
}