#include "vk.h"

#define ID_BUTTON_CHANGE 10
#define ID_BUTTON_RESET 11

#define REG_SUBKEY_PATH "SOFTWARE\\Duality\\KeyboardBlocker"

HWND gv_main;
HWND gv_label;
HWND gv_count;
HWND gv_change;
HWND gv_reset;

HBRUSH gv_hbrush;
HFONT gv_font;

unsigned char gv_block_key[10] = {0};
char gv_output[400];

bool gv_changing = false;

int gv_num = 0;

HKEY gv_regkey;

void ChangeWindowPosition(HWND hwnd, int x, int y, int bx, int by) {
	if (x < bx) { x = bx; }
	if (y < by) { y = by; }
	SetWindowPos(hwnd, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void ChangeWindowSize(HWND hwnd, int x, int y, int bx, int by) {
	if (x < bx) { x = bx; }
	if (y < by) { y = by; }
	SetWindowPos(hwnd, 0, 0, 0, x, y, SWP_NOMOVE | SWP_NOZORDER);
}

//Change/Print Key Variable
void ChangeKeyNPrint (unsigned char lp_key) {
	int i;
	bool add = true;
	char lv_output[6];
	
	if (lp_key == 0x00) { goto EXECUTE; }
	
	for (i = 0; i < gv_num; i++) {
		if (gv_block_key[i] == lp_key) {
			for (int j = i; j < gv_num - 1; j++) {
				gv_block_key[j] = gv_block_key[j + 1];
			}
			gv_num--;
			add = false;
			break;
		}
	}
	
	if (i == 10) { return; }
	if (add) {
		gv_block_key[gv_num++] = lp_key;
	}
	
	EXECUTE:
	
	ChangeWindowSize(gv_main, 200, 80 + (16 * gv_num), 200, 100);
	ChangeWindowSize(gv_label, 140, 16 * gv_num, 140, 20);
	ChangeWindowPosition(gv_change, 30, 20 + 16 * gv_num, 30, 40);
	ChangeWindowPosition(gv_reset, 100, 20 + 16 * gv_num, 100, 40);
	
	gv_output[0] = '\0';
	for (i = 0; i < gv_num; i++) {
		sprintf(lv_output, "Key%d", i);
		RegSetValueEx(gv_regkey, lv_output, 0, REG_BINARY, &gv_block_key[i], 1);
		if (i) {
			strcat(gv_output, "\r\n");
		}
		VirtualKeyCodeText(gv_block_key[i], gv_output);
	}
	RegSetValueEx(gv_regkey, "KeyCount", 0, REG_DWORD, (BYTE*)&gv_num, 4);
	sprintf(lv_output, "%d/10", gv_num);
	SetWindowText(gv_label, gv_output);
	SetWindowText(gv_count, lv_output);
}

bool gv_insert = false;

//Hookup Proc
LRESULT CALLBACK KeyProc(int lp_code, WPARAM lp_wParam, LPARAM lp_lParam) {
	unsigned char lv_key = ((KBDLLHOOKSTRUCT *)lp_lParam)->vkCode;
	
	if (gv_changing) {
		switch (lp_wParam) {
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
				if (!gv_insert) {
					ChangeKeyNPrint(lv_key);
					gv_insert = true;
				}
				break;
			case WM_KEYUP:
			case WM_SYSKEYUP:
				gv_insert = false;
				break;
		}
		return 1;
	} else {
		for (int i = 0; i < gv_num; i++) {
			if (gv_block_key[i] == lv_key) {
				return 1;
			}
		}
	}

	return CallNextHookEx(NULL, lp_code, lp_wParam, lp_lParam);
}

//Main Window Proc
LRESULT CALLBACK WindowProc (HWND lp_hwnd, UINT lp_uMsg, WPARAM lp_wParam, LPARAM lp_lParam) {

	switch (lp_uMsg) {
		case WM_DESTROY: {
			DeleteObject(gv_hbrush);
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
				gv_num = 0;
				for (int i = 0; i < 10; i++) {
					gv_block_key[i] = 0x00;
				}
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
	
	DWORD lv_size;
	unsigned char lv_key;
	char lv_valname[6];
	int lv_max;
	
	lv_return = RegCreateKeyEx(HKEY_CURRENT_USER, REG_SUBKEY_PATH, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &gv_regkey, &lv_result);
	if (lv_return != ERROR_SUCCESS) {
		MessageBox(lp_main, "Registry Open Failed!", "Error", MB_OK | MB_ICONERROR);
		PostQuitMessage(0);
		return;
	}
	
	if (lv_result == REG_OPENED_EXISTING_KEY) {
		lv_size = 4;
		RegGetValue(gv_regkey, NULL, "KeyCount", RRF_RT_REG_DWORD, NULL, &lv_max, &lv_size);
		for (int i = 0; i < lv_max; i++) {
			lv_size = 1;
			sprintf(lv_valname, "Key%d", i);
			lv_return = RegGetValue(gv_regkey, NULL, lv_valname, RRF_RT_REG_BINARY, NULL, &lv_key, &lv_size);
			if (lv_return == ERROR_SUCCESS) {
				ChangeKeyNPrint(lv_key);
			}
		}
	}
}

int WINAPI WinMain (HINSTANCE lp_hInstance, HINSTANCE lp_hPrevInstance, LPSTR lp_pCmdLine, int lp_nCmdShow) {
	
	gv_hbrush = CreateSolidBrush(RGB(240,240,240));
	
	gv_font = CreateFont(16,0,0,0,0,0,0,0,HANGEUL_CHARSET,3,2,1,
						VARIABLE_PITCH | FF_ROMAN, "Ebrima");
	
	//Create Window Class (Main)
	WNDCLASSEX lv_wcMain = {};

	lv_wcMain.cbSize = sizeof(lv_wcMain);
	lv_wcMain.lpfnWndProc = WindowProc;
	lv_wcMain.hInstance = lp_hInstance;
	lv_wcMain.hbrBackground = gv_hbrush;
	lv_wcMain.lpszClassName = "Duality_Frame_1";
	
	RegisterClassEx(&lv_wcMain);
	
	//Create Main Window
	gv_main = CreateWindow("Duality_Frame_1", "Keyboard-Blocker", WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
							CW_USEDEFAULT, CW_USEDEFAULT, 200, 100,
							NULL, NULL, lp_hInstance, NULL);
	
	//Virtual Key Code Text Window
	gv_label = CreateWindow("STATIC", "",
							WS_BORDER | WS_CHILD | WS_VISIBLE | SS_CENTER,
							40, 10, 140, 20,
							gv_main, NULL, lp_hInstance, NULL);
	
	gv_count = CreateWindow("STATIC", "0/10",
							WS_CHILD | WS_VISIBLE | SS_CENTER,
							5, 10, 30, 20,
							gv_main, NULL, lp_hInstance, NULL);
	SetWindowFont(gv_count, gv_font, FALSE);
	
	gv_change = CreateWindow("BUTTON", "Change",
							WS_BORDER | WS_CHILD | WS_VISIBLE | BS_CENTER | BS_VCENTER,
							30, 40, 60, 20,
							gv_main, (HMENU)ID_BUTTON_CHANGE, lp_hInstance, NULL);
	
	gv_reset = CreateWindow("BUTTON", "Reset",
							WS_BORDER | WS_CHILD | WS_VISIBLE | BS_CENTER | BS_VCENTER,
							100, 40, 60, 20,
							gv_main, (HMENU)ID_BUTTON_RESET, lp_hInstance, NULL);
	
	RegistryInit(gv_main);
	
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