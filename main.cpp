#include <string>
#include <iostream>
#include <cstdint>
#include "cpu.h"
#include "ppu.h"
#include "mmu.h"
#include "wmu.h"
#include "cia.h"
#include "SDL2/include/SDL_syswm.h"
#include "SDL2/include/SDL.h"
#undef main
using namespace::std;
//	[q00.c64]

SDL_Event event;					//	Eventhandler for all SDL events
bool unpaused = true;
uint8_t lastcyc = 0;

//	DEBUG
bool lcy = false;

int main()
{
	resetMMU();

	resetCPU();

	initPPU("n/a");

	while (1) {
		if (unpaused) {
			lastcyc = stepCPU();
			if (lcy) {
				lcy = false;
				printf("[ Cycles: %d ]\n", lastcyc);
				logDraw();
			}
			stepPPU(lastcyc);
			tickAllTimers(lastcyc);
		}
		handleWindowEvents(event);
	}

	return 1;
}

void logCycles() {
	lcy = true;
}

int getLastCyc() {
	return lastcyc;
}

void togglePause() {
	unpaused ^= 1;
}

void setPause() {
	unpaused = 0;
}

void resetPause() {
	unpaused = 1;
}