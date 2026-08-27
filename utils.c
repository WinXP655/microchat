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
	connect(s, (struct sockaddr*)&remote, sizeof(remote));

	// ... and then getsockname() return local IP address which will be displayed.
	struct sockaddr_in local;
	int len = sizeof(local);
	getsockname(s, (struct sockaddr*)&local, &len);

	closesocket(s);

	char ip_utf8[16];
	strncpy(ip_utf8, inet_ntoa(local.sin_addr), 15);
	ip_utf8[15] = '\0';
	MultiByteToWideChar(CP_UTF8, 0, ip_utf8, -1, ip_buffer, size);

	return true;
}

void AddMessage(const wchar_t* msg) {
	if (!hMsgDisplay || !msg || !*msg) return;
	int len = GetWindowTextLengthW(hMsgDisplay);
	
	if (len > BUFFER_SIZE) {
		AddMessage(L"Message is too long for display.");
		return;
	}

	SendMessageW(hMsgDisplay, EM_SETSEL, len, len);

	// Using EM_REPLACESEL instead SetWindowText to add string without overwriting entire buffer.
	if (len > 0) SendMessageW(hMsgDisplay, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
	SendMessageW(hMsgDisplay, EM_REPLACESEL, FALSE, (LPARAM)msg);
	SendMessageW(hMsgDisplay, WM_VSCROLL, SB_BOTTOM, 0);
}

void CleanupAndExit(void) {
	is_running = 0;

	if (client_socket != INVALID_SOCKET) {
		shutdown(client_socket, SD_BOTH);
		closesocket(client_socket);
		client_socket = INVALID_SOCKET;
	}

	// WaitForSingleObject on a thread with a running recv is a bad idea.
	// It causes deadlock since GUI waits for last data.
	if (receive_thread != NULL) {
		CloseHandle(receive_thread);
		receive_thread = NULL;
	}

	WSACleanup();
	PostQuitMessage(0);
}

void ShowError(const wchar_t* msg, DWORD err) {
	wchar_t buffer[512];
	swprintf(buffer, 512, L"%ls: %lu", msg, err);
	MessageBoxW(NULL, buffer, L"MicroChat", MB_OK);
}