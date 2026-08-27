// MicroChat Framework by WinXP655
// Minimalistic chat framework written on pure C.
// Official repository: https://github.com/WinXP655/microchat.
// MIT License - free to use, modify and distribute.

// -------------------------------------------------

#include "microchat.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
	(void)hPrevInstance;
	(void)lpCmdLine;

	// We get local computer name first, so we will already have it when it will be used.
	GetLocalComputerName();

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