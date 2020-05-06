#define _CRT_SECURE_NO_DEPRECATE
#include "SDL2/include/SDL.h"
#include <string>
#include <iostream>
#include <string.h>
#include <thread>
#ifdef _WIN32
#include <Windows.h>
#include <WinUser.h>
#include "SDL2/include/SDL_syswm.h"
#endif // _WIN32
#include "commctrl.h"
#include "ppu.h"
#include "cpu.h"
#include "mmu.h"
#include "cia.h"
#include "bus.h"
#include "wmu.h"
#include "sid.h"
#undef main

using namespace::std;

SDL_Window* mainWindow;				//	Main Window
SDL_GameController* controller;
uint8_t joypad = 0x00;
string current_title;

void initWindow(SDL_Window* win, string filename) {
	mainWindow = win;
	char title[50];
	string rom = filename;
	if (filename.find_last_of("\\") != string::npos)
		rom = filename.substr(filename.find_last_of("\\") + 1);
	snprintf(title, sizeof title, "[ q00.c64 ][ rom: %s ]", rom.c_str());
	SDL_SetWindowTitle(mainWindow, title);
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(mainWindow, &wmInfo);
	HWND hwnd = wmInfo.info.win.window;
	HMENU hMenuBar = CreateMenu();
	HMENU hFile = CreateMenu();
	HMENU hScreen = CreateMenu();
	HMENU hSid = CreateMenu();
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFile, "[ main ]");
	AppendMenu(hMenuBar, MF_STRING, 11, "[ ||> un/pause ]");
	AppendMenu(hMenuBar, MF_STRING, 2, "[ memory ]");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hSid, "[ sid ]");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hScreen, "[ screen ]");
	AppendMenu(hMenuBar, MF_STRING, 3, "[ log now ]");
	AppendMenu(hMenuBar, MF_STRING, 4, "[ print bus ]");
	AppendMenu(hFile, MF_STRING, 9, "� load rom");
	AppendMenu(hFile, MF_STRING, 7, "� reset");
	AppendMenu(hFile, MF_STRING, 1, "� exit");
	AppendMenu(hScreen, MF_STRING, 12, "� orig size");
	AppendMenu(hScreen, MF_STRING, 13, "� 2x size");
	AppendMenu(hSid, MF_STRING, 15, "� toggle voice 1 on/off");
	AppendMenu(hSid, MF_STRING, 16, "� toggle voice 2 on/off");
	AppendMenu(hSid, MF_STRING, 17, "� toggle voice 3 on/off");
	AppendMenu(hSid, MF_STRING, 18, "� disable");
	SetMenu(hwnd, hMenuBar);

	//	Enable WM events for SDL Window
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

	//	init SDL Gamecontroller
	controller = SDL_NumJoysticks() ? SDL_GameControllerOpen(0) : NULL;
}

void showMemoryMap() {

	SDL_Renderer* renderer;
	SDL_Window* window;

	//	init and create window and renderer
	SDL_CreateWindowAndRenderer(550, 470, 0, &window, &renderer);
	SDL_SetWindowSize(window, 550, 470);
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	SDL_SetWindowTitle(window, "[ q00.c64 ][ mem map ]");

	HWND hwnd = wmInfo.info.win.window;
	HINSTANCE hInst = wmInfo.info.win.hinstance;
	HWND hScroll = CreateWindow("EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_AUTOVSCROLL | ES_LEFT | WS_BORDER | ES_MULTILINE | ES_READONLY | ES_MULTILINE | ES_READONLY, 10, 80, 530, 380, hwnd, NULL, hInst, NULL);

	//	MEMDUMP Control
	string s = "Offset      00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\r\n\r\n";
	for (int i = 0; i < 0x10000; i += 0x10) {
		char title[70];
		snprintf(title, sizeof title, "0x%04x      %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x \r\n", i,
			readFromMem((uint16_t)i),
			readFromMem((uint16_t)(i + 1)),
			readFromMem((uint16_t)(i + 2)),
			readFromMem((uint16_t)(i + 3)),
			readFromMem((uint16_t)(i + 4)),
			readFromMem((uint16_t)(i + 5)),
			readFromMem((uint16_t)(i + 6)),
			readFromMem((uint16_t)(i + 7)),
			readFromMem((uint16_t)(i + 8)),
			readFromMem((uint16_t)(i + 9)),
			readFromMem((uint16_t)(i + 10)),
			readFromMem((uint16_t)(i + 11)),
			readFromMem((uint16_t)(i + 12)),
			readFromMem((uint16_t)(i + 13)),
			readFromMem((uint16_t)(i + 14)),
			readFromMem((uint16_t)(i + 15))
		);
		s.append((string)title);
	}

	const TCHAR* text = s.c_str();
	HFONT font = (HFONT)GetStockObject(ANSI_FIXED_FONT);
	LOGFONT lf;
	GetObject(font, sizeof(LOGFONT), &lf);
	lf.lfWeight = FW_LIGHT;
	HFONT boldFont = CreateFontIndirect(&lf);
	SendMessage(hScroll, WM_SETFONT, (WPARAM)boldFont, 60);
	SendMessage(hScroll, WM_SETTEXT, 60, reinterpret_cast<LPARAM>(text));
}

void handleWindowEvents(SDL_Event event) {
	//	poll events from menu
	SDL_PollEvent(&event);
	switch (event.type)
	{
	case SDL_SYSWMEVENT:
		if (event.syswm.msg->msg.win.msg == WM_COMMAND) {
			//	Exit
			if (LOWORD(event.syswm.msg->msg.win.wParam) == 1) {
				exit(0);
			}
			//	Mem
			else if (LOWORD(event.syswm.msg->msg.win.wParam) == 2) {
				showMemoryMap();
			}
			//	Enable logging
			else if (LOWORD(event.syswm.msg->msg.win.wParam) == 3) {
				setLog(true);
				printf("LOGGING START\n");
			}
			//	Print BUS
			else if (LOWORD(event.syswm.msg->msg.win.wParam) == 4) {
				BUS_showBus();
			}
			//	Reset
			else if (LOWORD(event.syswm.msg->msg.win.wParam) == 7) {
				CPU_reset();
			}
			//	Load ROM
			else if (LOWORD(event.syswm.msg->msg.win.wParam) == 9) {
				char f[100];
				OPENFILENAME ofn;

				ZeroMemory(&f, sizeof(f));
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
				ofn.lpstrFilter = "C64 Disks\0*.d64;*.prg\0";
				ofn.lpstrFile = f;
				ofn.nMaxFile = MAX_PATH;
				ofn.lpstrTitle = "[ rom selection ]";
				ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

				if (GetOpenFileNameA(&ofn)) {
					string s(f);
					for (auto& c : s) c = toupper(c);
					std::cout << s;
					if (s.substr(s.size() - 3) == "D64")
						loadD64(f);
					if (s.substr(s.size() - 3) == "PRG")
						loadPRG(f);
				}
			}
			//	pause / unpause
			else if (LOWORD(event.syswm.msg->msg.win.wParam) == 11) {
				togglePause();
			}
			//	screen size : orig
			else if (LOWORD(event.syswm.msg->msg.win.wParam) == 12) {
				setScreenSize(UI_SCREEN_SIZE::ORIG);
			}
			//	screen size : 2x
			else if (LOWORD(event.syswm.msg->msg.win.wParam) == 13) {
				setScreenSize(UI_SCREEN_SIZE::X2);
			}
			//	toggle SID channels
			else if (LOWORD(event.syswm.msg->msg.win.wParam) == 15) {
				SID_toggleChannel1();
			}
			else if (LOWORD(event.syswm.msg->msg.win.wParam) == 16) {
				SID_toggleChannel2();
			}
			else if (LOWORD(event.syswm.msg->msg.win.wParam) == 17) {
				SID_toggleChannel3();
			}
		}
		//	close a window
		if (event.syswm.msg->msg.win.msg == WM_CLOSE) {
			DestroyWindow(event.syswm.msg->msg.win.hwnd);
			PostMessage(event.syswm.msg->msg.win.hwnd, WM_CLOSE, 0, 0);
		}
		break;
	default:
		break;
	};

	uint8_t* keys = (uint8_t*)SDL_GetKeyboardState(NULL);
	//	pause/unpause
	if (keys[SDL_SCANCODE_PAGEDOWN]) {
		togglePause();
		keys[SDL_SCANCODE_PAGEDOWN] = 0;
	}

	//	handle keyboard to KERNAL
	setKeyboardInput(keys);

	//	use gamepad as input
	joypad = 0x00;
	if (controller != NULL) {
		bool Up = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
		bool Down = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
		bool Left = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
		bool Right = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
		bool Back = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START);
		bool Start = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK);
		bool LeftShoulder = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
		bool RightShoulder = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
		bool AButton = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A);
		bool BButton = SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B);

		int16_t StickX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
		int16_t StickY = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);

		//	Joypad
		joypad |= ((Right || (StickX > 10000)) ? 1 : 0) << 3;
		joypad |= ((Left  || (StickX < -10000)) ? 1 : 0) << 2;
		joypad |= ((Up  || (StickY < -10000)) ? 1 : 0) << 0;
		joypad |= ((Down  || (StickY > 10000)) ? 1 : 0) << 1;
		joypad |= ((AButton || BButton ) ? 1 : 0) << 4;
	}
	//	use keyboard only
	else {
		joypad |= (keys[SDL_SCANCODE_A] ? 1 : 0) << 2;
		joypad |= (keys[SDL_SCANCODE_S] ? 1 : 0) << 1;
		joypad |= (keys[SDL_SCANCODE_W] ? 1 : 0) << 0;
		joypad |= (keys[SDL_SCANCODE_D] ? 1 : 0) << 3;
		joypad |= (keys[SDL_SCANCODE_SPACE] ? 1 : 0) << 4;
	}

	//setJoystick1Input(&joypad);
	setJoystick2Input(&joypad);
}

void setTitle(string title) {
	current_title = title;
	updateTitle(title, 0);
}

void updateTitle(string filename, uint16_t fps) {
	char title[250];
	uint8_t last_slash = (filename.find_last_of("\\") != string::npos) ? (uint8_t)filename.find_last_of("\\") + 1 : 0;
	uint8_t distance = (uint8_t)filename.size() - last_slash;
	filename = filename.substr(last_slash, distance);
	snprintf(title, sizeof title, "[ q00.c64 ][ rom: %s ][ fps: %d ]", filename.c_str(), fps);
	for (int i = 0; i < sizeof(title); i++) {
		if (title[i] < 0x00)
			title[i] = 0x20;
	}
	SDL_SetWindowTitle(mainWindow, title);
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(mainWindow, &wmInfo);
}

void WMU_setFPS(uint16_t fps) {
	updateTitle(current_title, fps);
}

void printMsg(string mType, string mLevel, string msg, bool lineBreak) {
	string lb = (lineBreak) ? "\n" : "";
	printf("\x1b[47m\x1b\[30m[%s]\x1b[0m [\x1b[33m%s\x1b[0m] - %s %s", mType.c_str(), mLevel.c_str(), msg.c_str(), lb.c_str());
}

void printMsg(string mType, string mLevel, string msg) {
	printMsg(mType, mLevel, msg, true);
}
