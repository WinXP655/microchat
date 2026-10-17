#include "debug.h"
#include "microchat.h"
#include <stdio.h>

void ShowDebugInfo(HWND hWnd) {
	(void)hWnd;
#ifdef _DEBUG
	wchar_t buffer[2048];
	int pos = 0;
	int n;

#define APPEND(...) \
	do { \
		n = swprintf(buffer + pos, sizeof(buffer) / sizeof(wchar_t) - pos, __VA_ARGS__); \
		if (n < 0) goto done; \
		pos += n; \
	} while (0)

	wchar_t exe_path[MAX_PATH];
	GetModuleFileNameW(NULL, exe_path, MAX_PATH);
	APPEND(L"Executable Path: %ls\n\n", exe_path);

	OSVERSIONINFOEXW osvi = {0};
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
	GetVersionExW((OSVERSIONINFOW*)&osvi);

	SYSTEM_INFO si;
	GetNativeSystemInfo(&si);
	const wchar_t* arch = L"Unknown";
	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) arch = L"x64";
	else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) arch = L"x86";
	else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM) arch = L"ARM";
	else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) arch = L"ARM64";

	APPEND(L"OS Version: %d.%d\n", osvi.dwMajorVersion, osvi.dwMinorVersion);
	APPEND(L"OS Build: %d\n", osvi.dwBuildNumber);
	APPEND(L"OS Architecture: %ls\n", arch);

	APPEND(L"\nis_running: %d\n", is_running);
	APPEND(L"is_server: %d\n", is_server);
	APPEND(L"computer_name: %ls\n", computer_name);
	APPEND(L"server_ip: %ls\n", server_ip);
	APPEND(L"peer_ip: %ls\n", peer_ip);
	APPEND(L"peer_name: %ls\n", peer_name);

done:
	MessageBoxW(hWnd, buffer, L"MicroChat Debug", MB_OK);
#undef APPEND
#endif
}