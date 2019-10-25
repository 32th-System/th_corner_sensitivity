#define PI 3.141592653589793115997963468544185161590576171875

#include <Windows.h>
#include <Shlwapi.h>
#include <Xinput.h>
#include <math.h>
#include <malloc.h>

const wchar_t *default_ini = L"[main]\n; corner_sensitivity: 0 through 90 degrees. 0 = no corners, 90 = everything is a corner\ncorner_sensitivity = 50\n";

DWORD(__stdcall *__XInputGetState)(DWORD, XINPUT_STATE*) = nullptr;

float deadzone = 0;
float corner_sensitivity = 0;
double min_sens = 0;
double max_sens = 0;

DWORD __stdcall _XInputGetState(DWORD dwUserIndex, XINPUT_STATE *pState) {
	bool corner_allowed = true;
	DWORD ret = __XInputGetState(dwUserIndex, pState);


	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	double push = sqrt(pState->Gamepad.sThumbLX*pState->Gamepad.sThumbLX + pState->Gamepad.sThumbLY*pState->Gamepad.sThumbLY);

	if (push < 10000) { // Same as ZUN's hardcoded deadzone of 10000
		pState->Gamepad.sThumbLX = 0;
		pState->Gamepad.sThumbLY = 0;
		return ERROR_SUCCESS;
	}

	SHORT posX = pState->Gamepad.sThumbLX;
	SHORT posY = pState->Gamepad.sThumbLY;

	if (0 > posX) {
		if (posX == -32768) {
			posX = 32767;
		}
		else {
			posX = -posX;
		}
	}
	if (0 > posY) {
		if (posY == -32768) {
			posY = 32767;
		}
		else {
			posY = -posY;
		}
	}

	double angle = atan2(posY, posX) * (180/PI);

	if (!((angle - max_sens)*(angle - min_sens) <= 0)) {
		posX > posY ? pState->Gamepad.sThumbLY = 0 : pState->Gamepad.sThumbLX = 0;
	}

	return ERROR_SUCCESS;
}

int __stdcall thcrap_plugin_init() {
	DWORD tmp_get_state = *(DWORD*)0x0049A278;
	__XInputGetState = (DWORD(__stdcall*)(DWORD, XINPUT_STATE*))tmp_get_state;

	size_t path_len = (GetCurrentDirectoryW(0, NULL) + 24) * sizeof(wchar_t); // 22 = wcslen(L"corner_sensitivity.ini") + 2 (NULL terminator and \)
	wchar_t *path = (wchar_t*)_alloca(path_len);
	GetCurrentDirectoryW(path_len, path);
	wcscpy(PathAddBackslashW(path), L"corner_sensitivity.ini");
	if (!PathFileExistsW(path)) {
		HANDLE hIni = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		DWORD nByteW;
		WriteFile(hIni, (LPCVOID)default_ini, wcslen(default_ini) * sizeof(wchar_t), &nByteW, NULL);
		CloseHandle(hIni);
	}
	wchar_t corner_sensitivity_str[3];
	GetPrivateProfileStringW(L"main", L"corner_sensitivity", L"50", corner_sensitivity_str, 3, path);
	wchar_t *end;
	corner_sensitivity = wcstof(corner_sensitivity_str, &end);
	if (corner_sensitivity == 0.0) {
		MessageBoxW(*((HWND*)0x5226C0), L"The corner_sensitivity value in corner_sensitivity.ini is invalid, using default value!", L"Error", MB_ICONERROR | MB_OK);
		corner_sensitivity = 30.0;
	}
	min_sens = (90 - corner_sensitivity) / 2;
	max_sens = min_sens + corner_sensitivity;
	_freea(path);

	DWORD OldProtect;
	VirtualProtect((LPVOID)0x49A000L, 0x13000L, PAGE_READWRITE, &OldProtect);
	*(DWORD*)0x0049A278 = (DWORD)&_XInputGetState;
	VirtualProtect((LPVOID)0x49A000L, 0x13000L, OldProtect, &OldProtect);

	return 0;
}