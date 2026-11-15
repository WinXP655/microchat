// MicroChat by WinXP655
// Minimal chat framework written on pure C.
// Official repository: https://github.com/WinXP655/microchat.
// MIT License - free to use, modify and distribute.

// -------------------------------------------------

#include "microchat.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
	(void)hPrevInstance;
	(void)lpCmdLine;

	SetUnhandledExceptionFilter(CrashHandler);

	int mode = MessageBoxW(NULL,
		L"Start MicroChat as:\n"
		L"Yes - Chat Host\n"
		L"No - Chat Client\n"
		L"Cancel - Exit",
		L"MicroChat",
		MB_YESNOCANCEL);

	if (mode == IDCANCEL) return 0;
	is_server = (mode == IDYES);

	if (is_server) {
		if (!InitializeNetwork(true, hInstance, nCmdShow)) return 0;
	} else {
		INT_PTR dlg = DialogBoxParamW(hInstance, MAKEINTRESOURCEW(1), NULL, ConnectDialogProc, 0);

		// -1 = dialog failed to create.
		if (dlg == -1) {
			MessageBoxW(NULL, L"Could not load connection dialog.", L"MicroChat", MB_OK);
			return 0;
		}

		if (dlg != IDOK) return 0;
		if (!InitializeNetwork(false, hInstance, nCmdShow)) return 0;
	}

	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ExceptionInfo) {
	DWORD code = 0;
	void* address = NULL;

	if (ExceptionInfo && ExceptionInfo->ExceptionRecord) {
		code = ExceptionInfo->ExceptionRecord->ExceptionCode;
		address = ExceptionInfo->ExceptionRecord->ExceptionAddress;
	}

	wchar_t msg[128]; 
	swprintf(msg, sizeof(msg) / sizeof(wchar_t), L"Critical error!\nCode: 0x%08lX\nAddress: %p", code, address);
	MessageBoxW(NULL, msg, L"MicroChat", MB_OK);

	// Write log file
	HANDLE hFile = CreateFileW(L"crashlog.txt", FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		time_t now = time(NULL);
		struct tm* t = localtime(&now);
		char time_str[32];
		strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

		char buf[128]; 
		int len = snprintf(buf, sizeof(buf), "[%s] Crash: 0x%08lX at %p\r\n", time_str, code, address);
		if (len > 0 && len < (int)sizeof(buf)) {
			DWORD written;
			WriteFile(hFile, buf, len, &written, NULL);
		}
		CloseHandle(hFile);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}