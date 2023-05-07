#include "vk.h"

#define ID_BUTTON_CHANGE 10
#define ID_BUTTON_RESET 11

#define REG_SUBKEY_PATH "SOFTWARE\\Duality\\KeyboardBlocker"
HWND gv_label;
HWND gv_change;
HWND gv_reset;

unsigned char gv_block_key = 0;

bool gv_changing = false;
char gv_output[40];

HKEY gv_regkey;

//Change/Print Key Variable
void ChangeKeyNPrint (unsigned char lp_key) {
	gv_block_key = lp_key;
	RegSetValueEx(gv_regkey, "Key", 0, REG_BINARY, &lp_key, 1);
	VirtualKeyCodeText(gv_block_key, gv_output);
	SetWindowText(gv_label, gv_output);
}

//Hookup Proc
LRESULT CALLBACK KeyProc(int lp_code, WPARAM lp_wParam, LPARAM lp_lParam) {
	unsigned char lv_key = ((KBDLLHOOKSTRUCT *)lp_lParam)->vkCode;
	
	if (gv_changing) {
		ChangeKeyNPrint(lv_key);
		return 1;
	} else if (gv_block_key == lv_key) {
		return 1;
	}

	return CallNextHookEx(NULL, lp_code, lp_wParam, lp_lParam);
}

//Main Window Proc
LRESULT CALLBACK WindowProc (HWND lp_hwnd, UINT lp_uMsg, WPARAM lp_wParam, LPARAM lp_lParam) {

	switch (lp_uMsg) {
		case WM_DESTROY: {
			PostQuitMessage(0);
			RegCloseKey(gv_regkey);
			return 0;
		}
		case WM_COMMAND: {
			if (LOWORD(lp_wParam) == ID_BUTTON_CHANGE) {
			if (HIWORD(lp_wParam) == BN_CLICKED) {
				gv_changing = !gv_changing;
				SetWindowText(gv_change, gv_changing ? "Confirm" : "Change");
			}}
			if (LOWORD(lp_wParam) == ID_BUTTON_RESET) {
			if (HIWORD(lp_wParam) == BN_CLICKED) {
				ChangeKeyNPrint(0x00);
			}}
			break;
		}
	}
	return DefWindowProc(lp_hwnd, lp_uMsg, lp_wParam, lp_lParam);
}

void RegistryInit (HWND lp_main) {
	DWORD lv_result;
	LSTATUS lv_return;
	
	DWORD lv_size = 1;
	unsigned char lv_key = 0x00;
	
	lv_return = RegCreateKeyEx(HKEY_CURRENT_USER, REG_SUBKEY_PATH, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &gv_regkey, &lv_result);
	if (lv_return != ERROR_SUCCESS) {
		MessageBox(lp_main, "Registry Open Failed!", "Error", MB_OK | MB_ICONERROR);
	}
	
	switch (lv_result) {
		case REG_CREATED_NEW_KEY: {
			lv_return = RegSetValueEx(gv_regkey, "Key", 0, REG_BINARY, &lv_key, 1);
			break;
		}
		case REG_OPENED_EXISTING_KEY: {
			lv_return = RegGetValue(gv_regkey, NULL, "Key", RRF_RT_REG_BINARY, NULL, &lv_key, &lv_size);
			break;
		}
	}
	if (lv_return == ERROR_SUCCESS) {
		ChangeKeyNPrint(lv_key);
	}
}

int WINAPI WinMain (HINSTANCE lp_hInstance, HINSTANCE lp_hPrevInstance, LPSTR lp_pCmdLine, int lp_nCmdShow) {
	
	//Create Window Class (Main)
	WNDCLASSEX lv_wcMain = {};

	lv_wcMain.cbSize = sizeof(lv_wcMain);
	lv_wcMain.lpfnWndProc = WindowProc;
	lv_wcMain.hInstance = lp_hInstance;
	lv_wcMain.lpszClassName = "Duality_Frame_1";
	
	RegisterClassEx(&lv_wcMain);

	//Create Main Window
	HWND lv_main = CreateWindowEx(WS_EX_TOPMOST, "Duality_Frame_1", "Keyboard-Blocker", WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
									CW_USEDEFAULT, CW_USEDEFAULT, 200, 100,
									NULL, NULL, lp_hInstance, NULL
									);
	
	//Virtual Key Code Text Window
	gv_label = CreateWindow("STATIC", "None",
									WS_BORDER | WS_CHILD | WS_VISIBLE | SS_CENTER,
									30, 10, 140, 20,
									lv_main, NULL, lp_hInstance, NULL
									);
	gv_change = CreateWindow("BUTTON", "Change",
										WS_BORDER | WS_CHILD | WS_VISIBLE | BS_CENTER | BS_VCENTER,
										30, 40, 60, 20,
										lv_main, (HMENU)ID_BUTTON_CHANGE, lp_hInstance, NULL
										);
	gv_reset = CreateWindow("BUTTON", "Reset",
										WS_BORDER | WS_CHILD | WS_VISIBLE | BS_CENTER | BS_VCENTER,
										100, 40, 60, 20,
										lv_main, (HMENU)ID_BUTTON_RESET, lp_hInstance, NULL
										);
	RegistryInit(lv_main);
	
	//Hookup Global Keyboard Input
	SetWindowsHookEx(WH_KEYBOARD_LL, KeyProc, lp_hInstance, 0);

	//Message Loop
	MSG lv_msg = {};
	while (GetMessage(&lv_msg, NULL, 0, 0) > 0) {
		TranslateMessage(&lv_msg);
		DispatchMessage(&lv_msg);
	}
	return 0;
}