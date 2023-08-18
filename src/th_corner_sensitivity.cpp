#define PI 3.1415926535897931159979634685441

#include <thcrap.h>
#include <Xinput.h>

const wchar_t *default_ini =
L"[main]\n"
L"; corner_sensitivity: 0 through 90 degrees. 0 = no corners, 90 = everything is a corner\n"
L"corner_sensitivity = 50\n"
L"; deadzone: 0 = seemingly still joystick inputs still recognized will be considered inputs, 32767 = A full push wouldn't be enough for an input to get through\n"
L"deadzone = 10000\n"
;

typedef DWORD(__stdcall XInputGetState_t)(DWORD, XINPUT_STATE*);
XInputGetState_t *orig_XInputGetState = nullptr;

int deadzone = 0;
float min_sens = 0;
float max_sens = 0;

#define FLOAT_AS_INT(a) (*(int*)&a)

DWORD __stdcall detoured_XInputGetState(DWORD dwUserIndex, XINPUT_STATE *pState) {
	bool corner_allowed = true;
	DWORD ret = orig_XInputGetState(dwUserIndex, pState);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	SHORT posX = pState->Gamepad.sThumbLX;
	SHORT posY = pState->Gamepad.sThumbLY;

	// Comparing squared values is more accurate and faster
	// especially because all values stay ints rather than
	// being converted to floats or doubles
	int push = posX * posX + posY * posY;
	if (push < deadzone) {
		pState->Gamepad.sThumbLX = 0;
		pState->Gamepad.sThumbLY = 0;
		return ERROR_SUCCESS;
	}

	if (posX == -32768)	posX = 32767;
	if (posX < 0) posX = -posX;

	if (posY == -32768) posY = 32767;
	if (posY < 0) posY = -posY;

	float angle = atan2(posY, posX);

	if (!(((angle - max_sens) < 0) && ((angle - min_sens) > 0))) {
		if (posX > posY)
			pState->Gamepad.sThumbLY = 0;
		else if (posX < posY)
			pState->Gamepad.sThumbLX = 0;
	} else {
		if (posX > posY)
			pState->Gamepad.sThumbLY > 0 ? pState->Gamepad.sThumbLY = posX : pState->Gamepad.sThumbLY = -posX;
		else if (posX < posY)
			pState->Gamepad.sThumbLX > 0 ? pState->Gamepad.sThumbLX = posY : pState->Gamepad.sThumbLX = -posY;
	}
	return ERROR_SUCCESS;
}

int __stdcall thcrap_plugin_init() {
	HMODULE hXInput;
	const char* xinput_name;

	if (hXInput = GetModuleHandleW(L"xinput1_4.dll")) {
		xinput_name = "xinput1_4.dll";
	}
	else if (hXInput = GetModuleHandleW(L"xinput1_3.dll")) {;
		xinput_name = "xinput1_3.dll";
	}
	else {
		return 1;
	}

	orig_XInputGetState = (XInputGetState_t*)GetProcAddress(hXInput, "XInputGetState");
	int ret = detour_chain(xinput_name, 1,
		"XInputGetState", detoured_XInputGetState, &orig_XInputGetState,
		NULL
	);

	size_t path_len = (GetCurrentDirectoryW(0, NULL)
		+ 1  // Backslash
		+ 6  // wcslen(L"config") = 6
		+ 25 // wcslen(L"th_corner_sensitivity.ini") = 25
		+ 1  // NULL terminator
	);

	VLA(wchar_t, path, path_len);
	GetCurrentDirectoryW(path_len * sizeof(wchar_t), path);

	wcscpy(PathAddBackslashW(path), L"config");
	wcscpy(PathAddBackslashW(path), L"th_corner_sensitivity.ini");

	if (!PathFileExistsW(path)) {
		HANDLE hIni = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		DWORD nByteW;
		WriteFile(hIni, (LPCVOID)default_ini, wcslen(default_ini) * sizeof(wchar_t), &nByteW, NULL);
		CloseHandle(hIni);
	}

	wchar_t corner_sensitivity_str[8];
	GetPrivateProfileStringW(L"main", L"corner_sensitivity", L"50", corner_sensitivity_str, 8, path);
	wchar_t *end;
	float corner_sensitivity = wcstof(corner_sensitivity_str, &end);

	if (corner_sensitivity == 0.0) {
		log_mbox("th_corner_sensitivity error", MB_ICONERROR | MB_OK, "The corner_sensitivity value in corner_sensitivity.ini is invalid, using default value!");
		corner_sensitivity = 30.0;
	}

	min_sens = (90 - corner_sensitivity) / 2;
	max_sens = min_sens + corner_sensitivity;

	// detoured_XInputGetState won't have to convert
	// anything to degrees if those are already radian
	min_sens *= PI / 180;
	max_sens *= PI / 180;

	deadzone = GetPrivateProfileIntW(L"main", L"deadzone", 10000, path);

	// detoured_XInputGetState needs to square this number anyways
	deadzone *= deadzone;

	VLA_FREE(path);

	return 0;
}
