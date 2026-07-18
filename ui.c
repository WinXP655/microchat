#include "microchat.h"

HWND hEdit = NULL;
HWND hSendBtn = NULL;
HWND hMsgDisplay = NULL;
HWND hWndGlobal = NULL;
WNDPROC oldEditProc = NULL;
HFONT hFont = NULL;
HFONT hFontBold = NULL;

void ShowMainWindow(HINSTANCE hInstance, int nCmdShow) {
	WNDCLASSW wc = {0};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"MicroChatWndClass";
	wc.hIcon = LoadIconW(NULL, IDI_APPLICATION); // Generic application icon from user32.dll
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	RegisterClassW(&wc);

	wchar_t title[256];
	swprintf(title, sizeof(title) / sizeof(wchar_t), L"MicroChat - %ls", peerName);
	title[sizeof(title) / sizeof(wchar_t) - 1] = L'\0';
	
	HWND hWnd = CreateWindowW(L"MicroChatWndClass", title,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME | WS_MAXIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 520, 320,
		NULL, NULL, hInstance, NULL);

	// Here using ExitProcess because this event is critical.
	// CreateWindow failing is very rare but non-zero chance.
	if (!hWnd) {
		MessageBoxW(NULL, L"Failed to create main window.", L"MicroChat", MB_OK);
		ExitProcess(1);
	}

	hWndGlobal = hWnd;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
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
			hFont = CreateFontW(
				-12, 0, 0, 0, FW_NORMAL,
				FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
				L"MS Shell Dlg"
			);

			hFontBold = CreateFontW(
				-12, 0, 0, 0, FW_BOLD,
				FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
				L"MS Shell Dlg"
			);

			hMsgDisplay = CreateWindowW(
				L"EDIT", L"",
				WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
				ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
				0, 0, 520, 240,
				hWnd, (HMENU)ID_MSG_DISPLAY, NULL, NULL
			);

			hEdit = CreateWindowW(
				L"EDIT", L"", 
				WS_CHILD | WS_VISIBLE | WS_BORDER |
				ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL,
				0, 240, 430, 30,
				hWnd, (HMENU)ID_EDIT, NULL, NULL);

			hSendBtn = CreateWindowW(
				L"BUTTON", L"Send", 
				WS_CHILD | WS_VISIBLE |
				BS_PUSHBUTTON,
				430, 240, 60, 40,
				hWnd, (HMENU)ID_SEND, NULL, NULL);

			SendMessageW(hMsgDisplay, WM_SETFONT, (WPARAM)hFont, TRUE);
			SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
			SendMessageW(hSendBtn, WM_SETFONT, (WPARAM)hFontBold, TRUE);

			// Zeroing error code and replacing default key handler. Needed to handle custom ENTER and CTRL+A functions
			SetLastError(0);
			oldEditProc = (WNDPROC)SetWindowLongPtrW(hEdit, GWLP_WNDPROC, (LONG_PTR)EditProc);
			if (!oldEditProc && GetLastError() != 0) {
				AddMessage(L"Warning: Edit subclass setup failed. Enter and Ctrl+A may not work as expected.");
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

			CleanupAndExit();
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
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK EditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_GETDLGCODE) {
		return DLGC_WANTALLKEYS | CallWindowProc(oldEditProc, hWnd, uMsg, wParam, lParam);
	} else if (uMsg == WM_KEYDOWN) {
		// EDIT control do not have native Select All.
		// This is a Windows limitation - needed to handle it manually.
		if (wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
			SendMessageW(hWnd, EM_SETSEL, 0, -1);
			return 0;
		}

		// Acting like a normal messengers - Send on Enter, Shift+Enter for new line.
		if (wParam == VK_RETURN) {
			if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
				return CallWindowProcW(oldEditProc, hWnd, uMsg, wParam, lParam);
			} else {
				PostMessageW(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(ID_SEND, 0), 0);
				return 0;
			}
		}
	}

	return CallWindowProc(oldEditProc, hWnd, uMsg, wParam, lParam);
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
					wchar_t ip[16];
					GetDlgItemTextW(hwnd, IDC_IP, ip, sizeof(ip) / sizeof(wchar_t));

					// Trim leading spaces
					wchar_t* p = ip;
					while (*p == L' ') p++;

					// Trim trailing spaces
					int len = wcslen(p);
					while (len > 0 && p[len - 1] == L' ') {
						p[len - 1] = L'\0';
						len--;
					}

					if (wcslen(p) == 0) {
						MessageBoxW(hwnd, L"Please enter a server IP.", L"MicroChat", MB_OK);
						SetFocus(GetDlgItem(hwnd, IDC_IP));
						return TRUE;
					}

					// Validate IP format
					int octets[4];
					int valid = (swscanf(p, L"%d.%d.%d.%d", &octets[0], &octets[1], &octets[2], &octets[3]) == 4);

					if (valid) {
						for (int i = 0; i < 4; i++) {
							if (octets[i] < 0 || octets[i] > 255) {
								valid = 0;
								break;
							}
						}
					}

					if (!valid) {
						MessageBoxW(hwnd, L"Invalid IP address.\nExample: 192.168.1.100", L"MicroChat", MB_OK);
						SetFocus(GetDlgItem(hwnd, IDC_IP));
						return TRUE;
					}

					wcscpy(serverIp, p);
					serverIp[sizeof(serverIp) / sizeof(wchar_t) - 1] = L'\0';
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

// This is a purely informational dialog - it shows server IP,
// so user can share it with others. It is not required for core logic.
// Because MicroChat do not have gethostbyname fallback (outdated method), UDP hack will return 0.0.0.0.
// Do not try to connect to 0.0.0.0 since it always fails (works only as source, not destination).
DWORD WINAPI ShowServerIPMessage(LPVOID lpParam) {
	(void)lpParam;

	wchar_t buffer[512];
	swprintf(buffer, sizeof(buffer) / sizeof(wchar_t),
		L"Host IP: %ls\n"
		L"Share this IP with others for connection.",
		serverIp);
	buffer[sizeof(buffer) / sizeof(wchar_t) - 1] = L'\0';

	MessageBoxW(NULL, buffer, L"MicroChat", MB_OK);
	return 0;
}