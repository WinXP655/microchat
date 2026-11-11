// Module: Utilities/Shared (utils.c)
// Part of MicroChat: https://github.com/WinXP655/microchat
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
	static volatile LONG already_exiting = 0;
	if (InterlockedExchange(&already_exiting, 1)) return;

	InterlockedExchange(&is_running, 0);

	if (client_socket != INVALID_SOCKET) {
		shutdown(client_socket, SD_BOTH);
	}

	// WaitFotSingleObject can be used on recv thread without hang
	// in this order: shutdown - wait - close socket
	if (receive_thread != NULL) {
		WaitForSingleObject(receive_thread, THREAD_WAIT_MS);
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

wchar_t* Trim(wchar_t* s) {
	if (!s) return NULL;

	// Leading spaces
	while (*s == L' ' || *s == L'\t' || *s == L'\r' || *s == L'\n') s++;

	// Trailing spaces
	wchar_t* end = s + wcslen(s);
	while (end > s && (*(end - 1) == L' ' || *(end - 1) == L'\t' || *(end - 1) == L'\r' || *(end - 1) == L'\n')) {
		end--;
	}

	*end = L'\0';

	return s;
}

int TrimIncompleteUTF8(const char* buffer, int bytes) {
	int pos = bytes;

	while (pos > 0 && ((unsigned char)buffer[pos - 1] & 0xC0) == 0x80) {
		pos--;
	}

	if (pos == 0) return 0;

	unsigned char lead = (unsigned char)buffer[pos - 1];
	int need = 0;

	if ((lead & 0x80) == 0x00) need = 0;
	else if ((lead & 0xE0) == 0xC0) need = 1;
	else if ((lead & 0xF0) == 0xE0) need = 2;
	else if ((lead & 0xF8) == 0xF0) need = 3;
	else return pos - 1;

	int have = bytes - pos;

	if (have < need) {
		return pos - 1;
	}

	return bytes;
}