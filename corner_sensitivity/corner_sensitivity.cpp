#define PI 3.141592653589793115997963468544185161590576171875

#include <Windows.h>
#include <Shlwapi.h>
#include <Xinput.h>
#include <math.h>
#include <malloc.h>

DWORD(__stdcall *__XInputGetState)(DWORD, XINPUT_STATE*) = nullptr;

float deadzone = 0;
float corner_sensitivity = 0;
double min_sens = 0;
double max_sens = 0;


DWORD _XInputGetState(DWORD dwUserIndex, XINPUT_STATE *pState) {
	if (!corner_sensitivity | !deadzone) {
		size_t path_len = (GetCurrentDirectoryW(0, NULL) + 24) * sizeof(wchar_t); // 22 = wcslen(L"corner_sensitivity.ini") + 2 (NULL terminator and \)
		wchar_t *path = (wchar_t*)_alloca(path_len);
		GetCurrentDirectoryW(path_len, path);
		wcscpy(PathAddBackslashW(path), L"corner_sensitivity.ini");
		wchar_t deadzone_str[6]; 
		wchar_t corner_sensitivity_str[3];
		GetPrivateProfileStringW(L"main", L"deadzone", L"600", deadzone_str, 6, path);
		GetPrivateProfileStringW(L"main", L"corner_sensitivity", L"30", corner_sensitivity_str, 3, path);
		wchar_t *end;
		deadzone = wcstof(deadzone_str, &end);
		corner_sensitivity = wcstof(corner_sensitivity_str, &end);
		if (deadzone = 0.0) {
			MessageBoxW(*((HWND*)0x5226C0), L"The deadzone value in corner_sensitivity.ini is invalid, using default value!", L"Error", MB_ICONERROR | MB_OK);
			deadzone = 600.0;
		}
		if (corner_sensitivity = 0.0) {
			MessageBoxW(*((HWND*)0x5226C0), L"The corner_sensitivity value in corner_sensitivity.ini is invalid, using default value!", L"Error", MB_ICONERROR | MB_OK);
			corner_sensitivity = 30.0;
		}
		_freea(path);
	}

	if (!min_sens | !max_sens) {
		double min_sens = (90 - corner_sensitivity) / 2;
		double max_sens = min_sens + corner_sensitivity;
	}
	

	bool corner_allowed = true;

	if (!__XInputGetState) {
		DWORD(__stdcall *__XInputGetState)(DWORD, XINPUT_STATE*) = (DWORD(__stdcall*)(DWORD, XINPUT_STATE*))GetProcAddress(GetModuleHandle(L"xinput1_3.dll"), "XInputGetState");
	}

	DWORD ret = __XInputGetState(dwUserIndex, pState);

	
	if (ret != ERROR_SUCCESS) {
		return ret;
	}
		
	double push = sqrt(pState->Gamepad.sThumbLX*pState->Gamepad.sThumbLX + pState->Gamepad.sThumbLY*pState->Gamepad.sThumbLY);

	if (push < deadzone) {
		pState->Gamepad.sThumbLX = 0;
		pState->Gamepad.sThumbLY = 0;
		return ERROR_SUCCESS;
	}
	
	SHORT posX;
	SHORT posY;

	if (pState->Gamepad.sThumbLX >> 15) {
		posX = -pState->Gamepad.sThumbLX;
	}
	else {
		posX = pState->Gamepad.sThumbLX;
	}

	if (pState->Gamepad.sThumbLY >> 15) {
		posY = -pState->Gamepad.sThumbLY;
	}
	else {
		posY = pState->Gamepad.sThumbLY;
	}

	double angle = atan2(posY, posX) * PI / 180;

	if (!((angle - min_sens) <= (max_sens - min_sens))) {
		corner_allowed = false;
	}
	
	BYTE sign = (pState->Gamepad.sThumbLX >> 15);
	posX > posY ? pState->Gamepad.sThumbLX = 32767 : pState->Gamepad.sThumbLY = 32767;
	if (corner_allowed = true) {
		posX > posY ? pState->Gamepad.sThumbLY = 32767 : pState->Gamepad.sThumbLX = 32767;
	}
	else {
		posX > posY ? pState->Gamepad.sThumbLY = 0 : pState->Gamepad.sThumbLX = 0;
	}

	return ret;
}