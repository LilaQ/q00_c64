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
#include "main.h"
#undef main

using namespace::std;

SDL_Window* mainWindow;				//	Main Window

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
	HMENU hEdit = CreateMenu();
	HMENU hMem = CreateMenu();
	HMENU hConfig = CreateMenu();
	HMENU hSound = CreateMenu();
	HMENU hPalettes = CreateMenu();
	HMENU hSavestates = CreateMenu();
	HMENU hVol = CreateMenu();
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFile, "[ main ]");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hConfig, "[ config ]");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hVol, "[ vol ]");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hSavestates, "[ savestates ]");
	AppendMenu(hMenuBar, MF_STRING, 11, "[ ||> un/pause ]");
	AppendMenu(hMenuBar, MF_STRING, 2, "[ memory ]");
	AppendMenu(hFile, MF_STRING, 9, "» load rom");
	AppendMenu(hFile, MF_STRING, 7, "» reset");
	AppendMenu(hFile, MF_STRING, 1, "» exit");
	AppendMenu(hSavestates, MF_STRING, 12, "» save state");
	AppendMenu(hSavestates, MF_STRING, 13, "» load state");
	AppendMenu(hConfig, MF_POPUP, (UINT_PTR)hSound, "[ sound ]");
	AppendMenu(hSound, MF_STRING, 14, "» disable/enable");
	AppendMenu(hVol, MF_STRING, 24, "» 10%");
	AppendMenu(hVol, MF_STRING, 25, "» 20%");
	AppendMenu(hVol, MF_STRING, 26, "» 30%");
	AppendMenu(hVol, MF_STRING, 27, "» 40%");
	AppendMenu(hVol, MF_STRING, 28, "» 50%");
	AppendMenu(hVol, MF_STRING, 29, "» 60%");
	AppendMenu(hVol, MF_STRING, 30, "» 70%");
	AppendMenu(hVol, MF_STRING, 31, "» 80%");
	AppendMenu(hVol, MF_STRING, 32, "» 90%");
	AppendMenu(hVol, MF_STRING, 33, "» 100%");
	SetMenu(hwnd, hMenuBar);

	//	Enable WM events for SDL Window
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
}

void showMemoryMap() {

	SDL_Renderer* renderer;
	SDL_Window* window;

	//	init and create window and renderer
	SDL_Init(SDL_INIT_VIDEO);
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
			readFromMem(i),
			readFromMem(i + 1),
			readFromMem(i + 2),
			readFromMem(i + 3),
			readFromMem(i + 4),
			readFromMem(i + 5),
			readFromMem(i + 6),
			readFromMem(i + 7),
			readFromMem(i + 8),
			readFromMem(i + 9),
			readFromMem(i + 10),
			readFromMem(i + 11),
			readFromMem(i + 12),
			readFromMem(i + 13),
			readFromMem(i + 14),
			readFromMem(i + 15)
		);
		s.append((string)title);
	}

	const TCHAR* text = s.c_str();
	HDC wdc = GetWindowDC(hScroll);
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
			//	Reset
			else if (LOWORD(event.syswm.msg->msg.win.wParam) == 7) {
				resetCPU();
			}
			//	Load ROM
			else if (LOWORD(event.syswm.msg->msg.win.wParam) == 9) {
				char f[100];
				OPENFILENAME ofn;

				ZeroMemory(&f, sizeof(f));
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
				ofn.lpstrFilter = "C64 Disks\0*.d64;*.prg\0C64 Programs\0*.prg\0";
				ofn.lpstrFile = f;
				ofn.nMaxFile = MAX_PATH;
				ofn.lpstrTitle = "[ rom selection ]";
				ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

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
			//	save state
			else if (LOWORD(event.syswm.msg->msg.win.wParam) == 12) {
				//	TODO STUB
				/*printf("saving state\n");
				char f[100];
				OPENFILENAME ofn;

				ZeroMemory(&f, sizeof(f));
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
				ofn.lpstrFilter = "GameBoy Savestates\0*.gbss\0";
				ofn.lpstrFile = f;
				ofn.lpstrDefExt = "gbss";
				ofn.nMaxFile = MAX_PATH;
				ofn.lpstrTitle = "[ rom selection ]";
				ofn.Flags = OFN_DONTADDTORECENT;

				if (GetSaveFileNameA(&ofn)) {
					filename = f;
					saveState(f, registers, flags, pc, sp);
				}*/
			}
			//	load state
			else if (LOWORD(event.syswm.msg->msg.win.wParam) == 13) {

				//	halt emulation to avoid damaging the data (keep it that way, user must unpause)
				/*unpaused = false;

				printf("loading savestate\n");
				char f[100];
				OPENFILENAME ofn;

				ZeroMemory(&f, sizeof(f));
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = NULL;
				ofn.lpstrFilter = "GameBoy Savestates\0*.gbss\0";
				ofn.lpstrFile = f;
				ofn.nMaxFile = MAX_PATH;
				ofn.lpstrTitle = "[ savestate selection ]";
				ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

				if (GetOpenFileNameA(&ofn)) {
					filename = f;
					loadState(f, registers, flags, pc, sp);
				}*/
			}
			//	disable / enable sound
			else if (LOWORD(event.syswm.msg->msg.win.wParam) == 14) {
				//toggleAudio();
			}
			//	set volume
			else if (LOWORD(event.syswm.msg->msg.win.wParam) == 24 ||
				LOWORD(event.syswm.msg->msg.win.wParam) == 25 ||
				LOWORD(event.syswm.msg->msg.win.wParam) == 26 ||
				LOWORD(event.syswm.msg->msg.win.wParam) == 27 ||
				LOWORD(event.syswm.msg->msg.win.wParam) == 28 ||
				LOWORD(event.syswm.msg->msg.win.wParam) == 29 ||
				LOWORD(event.syswm.msg->msg.win.wParam) == 30 ||
				LOWORD(event.syswm.msg->msg.win.wParam) == 31 ||
				LOWORD(event.syswm.msg->msg.win.wParam) == 32 ||
				LOWORD(event.syswm.msg->msg.win.wParam) == 33
				) {
				//setVolume((float)(LOWORD(event.syswm.msg->msg.win.wParam) - 23) * 0.1);
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
}


