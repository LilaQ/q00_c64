#pragma once
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
//	[q00.c64]

SDL_Event event;					
bool unpaused = true;
bool isBadline = false;
bool rasterIRQ = false;
bool flaggedAfterNextInstr = false;
bool flaggedForIRQ = false;
bool cpuHalted = false;
bool SHOW_BUS = false;
uint8_t c = 0;
uint8_t _c[63];								//	2 = bus-takeover; 1 = cycle blocked by VIC; 0 = cycle free for 6510
uint8_t _s[8] = { 0,0,0,0,0,0,0,0 };		//	1 = Sprite enabled; 0 = Sprite disabled;
uint8_t _t[8] = { 57,59,61,0,2,4,6,8 };		//	cycle no.s for each Sprite

void BUS_showBus() {
	SHOW_BUS = true;
}

bool checkBusTransfer(uint8_t i) {
	if ((_s[(i + 7) % 8] == 0) && (_s[(i + 6) % 8] == 0)) {
		return true;
	}
	return false;
}

bool checkSpriteGap(uint8_t i) {
	if ((_s[(i + 7) % 8] == 0) && (_s[(i + 6) % 8] == 1)) {
		return true;
	}
	return false;
}

int main()
{
	resetMMU();
	CPU_reset();
	initPPU("n/a");

	while (1) {
		if (unpaused) {

			/*
				Cycle management for a single rasterline
			*/
			/*

				normale Zeile (aktive Sprites: keine)
						 111111111122222222223333333333444444444455555555556666 
				123456789012345678901234567890123456789012345678901234567890123 Zyklen
				p p p p p rrrrrgggggggggggggggggggggggggggggggggggggggg  p p p  P-1 VIC
																				P-2 VIC
				xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx P-2 CPU

				normale Zeile (aktive Sprites: Sprite-0)
						 111111111122222222223333333333444444444455555555556666
				123456789012345678901234567890123456789012345678901234567890123
				p p p p p rrrrrgggggggggggggggggggggggggggggggggggggggg  psp p  P-1 VIC
																		 ss     P-2 VIC
				xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxXXX  xxxx P-2 CPU

				Bad Line (aktive Sprites: keine)
						 111111111122222222223333333333444444444455555555556666 
				123456789012345678901234567890123456789012345678901234567890123 Zyklen
				p p p p p rrrrrgggggggggggggggggggggggggggggggggggggggg  p p p  P-1 VIC
							  cccccccccccccccccccccccccccccccccccccccc          P-2 VIC
				xxxxxxxxxxxXXX                                        xxxxxxxxx P-2 CPU
				23 cycles available

				Bad Line (aktive Sprites: ALLE)
						 111111111122222222223333333333444444444455555555556666 
				123456789012345678901234567890123456789012345678901234567890123 Zyklen
				pspspspspsrrrrrgggggggggggggggggggggggggggggggggggggggg  pspsps P-1 VIC
				ssssssssss    cccccccccccccccccccccccccccccccccccccccc   ssssss P-2 VIC
						  xXXX                                        XXX       P-2 CPU
				4-7 cycles available

				g= grafix data fetch (character images or graphics data)
				r= refresh
				p= sprite image pointer fetch
				c= character and color CODE fetch during a bad scan line
				s= sprite data fetch
				x= processor executing instructions
				X= processor executing an instruction, bus request pending

				  A Bad Line Condition is given at any arbitrary clock cycle, if at the
				 negative edge of ø0 at the beginning of the cycle RASTER >= $30 and RASTER
				 <= $f7 and the lower three bits of RASTER are equal to YSCROLL and if the
				 DEN bit was set during an arbitrary cycle of raster line $30.

			*/
			/*
				VIC
				------------------------------------------------
				Cycle 01			: VIC Sprite 3 Pointer fetch
				Cycle 03			: VIC Sprite 4 Pointer fetch
				Cycle 05			: VIC Sprite 5 Pointer fetch
				Cycle 07			: VIC Sprite 6 Pointer fetch
				Cycle 09			: VIC Sprite 7 Pointer fetch
				Cycle 11 - Cycle 15 : VIC DRAM refresh
				Cycle 16 - Cycle 55 : VIC graphics data fetch
				Cycle 58			: VIC Sprite 0 Pointer fetch
				Cycle 60			: VIC Sprite 1 Pointer fetch
				Cycle 62			: VIC Sprite 2 Pointer fetch

				6510
				------------------------------------------------
				Cycle 11 - Cycle 15 : 6510 compute instruction
				Cycle 16 - Cycle 55 : 6510 compute instruction (only if not a badline) 				
				Cycle 47 - Cycle 62	: 6510 compute instruction (only if there are at least 3 free cycles between sprite data fetches)
			*/


			/*
				High-Phi Phase
			*/
			//	TODO SWITCH CASE ALL OF THIS

			if (c == 1) {
				isBadline = VIC_isBadline();
				//if (isBadline)
					//printf("BADLINE at %x\n", currentScanline());
				VIC_fetchSpritePointer(3);
				if (_s[3]) {
					VIC_fetchSpriteDataBytes(3);
				}
			}
			else if (c == 3) {
				VIC_fetchSpritePointer(4);	
				if (_s[4]) {
					VIC_fetchSpriteDataBytes(4);
				}
			}
			else if (c == 5) {
				VIC_fetchSpritePointer(5);
				if (_s[5]) {
					VIC_fetchSpriteDataBytes(5);
				}
			}
			else if (c == 7) {
				VIC_fetchSpritePointer(6);
				if (_s[6]) {
					VIC_fetchSpriteDataBytes(6);
				}
			}
			else if (c == 9) {
				VIC_fetchSpritePointer(7);
				if (_s[7]) {
					VIC_fetchSpriteDataBytes(7);
				}
			}
			else if ((c == 10 || c == 54) && SHOW_BUS) {
				printf("Scanline %d (BL? %d) \t", currentScanline(), VIC_isBadline());
				for (int i = 0; i <= 62; i++) {
					printf("%d",_c[i]);
				}
				printf("\n");
			}
			else if (c >= 11 && c <= 15) {
				//	TODO: Das hier wird 5 mal ausgeführt, in jedem Cycle -> unnötig und kostet performance
				//	Fetch attributes for Sprites (height, width, position etc.)
				_s[0] = VIC_isSpriteEnabled(0);
				_s[1] = VIC_isSpriteEnabled(1);
				_s[2] = VIC_isSpriteEnabled(2);
				_s[3] = VIC_isSpriteEnabled(3);
				_s[4] = VIC_isSpriteEnabled(4);
				_s[5] = VIC_isSpriteEnabled(5);
				_s[6] = VIC_isSpriteEnabled(6);
				_s[7] = VIC_isSpriteEnabled(7);
				VIC_fetchSpriteAttributes(0);
				VIC_fetchSpriteAttributes(1);
				VIC_fetchSpriteAttributes(2);
				VIC_fetchSpriteAttributes(3);
				VIC_fetchSpriteAttributes(4);
				VIC_fetchSpriteAttributes(5);
				VIC_fetchSpriteAttributes(6);
				VIC_fetchSpriteAttributes(7);
				//	Reset Cycle-Array, that stores VIC-busy cycles
				memset(_c, 0x00, sizeof(_c));
				if (isBadline) {
					_c[11] = 2;			//	Badline, bus-takeover cycles
					_c[12] = 2;			//	Badline, bus-takeover cycles
					_c[13] = 2;			//	Badline, bus-takeover cycles
					for (int i = 14; i <= 53; i++) {
						_c[i] = 1;		//	Badline, cycles stolen from CPU for VIC
					}
				}
			}
			else if (c == 58) {
				VIC_fetchSpritePointer(0);
				if (_s[0]) {
					VIC_fetchSpriteDataBytes(0);
				}
			}
			else if (c == 60) {
				VIC_fetchSpritePointer(1);
				if (_s[1]) {
					VIC_fetchSpriteDataBytes(1);
				}
			}
			else if (c == 62) {
				VIC_fetchSpritePointer(2);
				if (_s[2]) {
					VIC_fetchSpriteDataBytes(2);
				}
			}
			/*
				Sprite loading, cycle stealing from 6510
			*/

			//	Sprite #0
			if (c >= 54 && c <= 56 && _s[0] && VIC_isSpriteInNextLine(0)) {		//	Bus-Takeover cycles Sprite #0 (1,2,3)
				if (!isBadline) {
					_c[c] = 2;
				}
				else {
					_c[c] = 1;
				}
			}
			if (c >= 57 && c <= 58 && _s[0] && VIC_isSpriteInNextLine(0)) {		//	Sprite pointer / data Sprite #0
				_c[c] = 1;
			}

			//	Sprite #1
			if (c == 56 && _s[1] && !_s[0] && VIC_isSpriteInNextLine(1)) {		//	Bus-Takeover cycles Sprite #1 (1)
				_c[c] = 2;
			}
			if (c >= 57 && c <= 58 && _s[1] && VIC_isSpriteInNextLine(1)) {
				if (!_s[0]) {													//	Bus-Takeover cycles Sprite #1 (2,3)
					_c[c] = 2;
				}
			}
			if (c >= 59 && c <= 60 && _s[1] && VIC_isSpriteInNextLine(1)) {		//	Sprite pointer / data Sprite #1
				_c[c] = 1;
			}

			//	Sprite #2
			if (c == 58 && _s[2] && !_s[1] && VIC_isSpriteInNextLine(2)) {
				if (!_s[0]) {
					_c[c] = 2;													//	Bus-Takeover cycles Sprite #2 (1)
				}
			}
			if (c >= 59 && c <= 60 && _s[2] && VIC_isSpriteInNextLine(2)) {
				if (!_s[1]) {
					if (!_s[0]) {
						_c[c] = 2;												//	Bus-Takeover cycles Sprite #2 (2,3)
					}
					else {
						_c[c] = 1;												//	Fill-Cycles betweend Sprite #0 and #2
					}
				}
			}
			if (c >= 61 && c <= 62 && _s[2] && VIC_isSpriteInNextLine(2)) {		//	Sprite pointer / data Sprite #2
				_c[c] = 1;
			}

			//	Sprite #3
			if (c == 60 && _s[3] && !_s[2] && VIC_isSpriteInNextLine(3)) {
				if (!_s[1]) {
					_c[c] = 2;													//	Bus-Takeover cycles Sprite #3 (1)
				}
			}
			if (c >= 61 && c <= 62 && _s[3] && VIC_isSpriteInNextLine(3)) {
				if (!_s[2]) {
					if (!_s[1]) {
						_c[c] = 2;												//	Bus-Takeover cycles Sprite #3 (2,3)
					}
					else {
						_c[c] = 1;												//	Fill-Cycles between Sprite #1 and #3
					}
				}
			}
			if (c >= 0 && c <= 1 && _s[3] && VIC_isSpriteInCurrentLine(3)) {
				_c[c] = 1;														//	Sprite pointer / data Sprite #3
			}

			//	Sprite #4
			if (c == 62 && _s[4] && !_s[3] && VIC_isSpriteInNextLine(4)) {
				if (!_s[2]) {
					_c[c] = 2;													//	Bus-Takeover cycles Sprite #4 (1)
				}
			}
			if (c >= 0 && c <= 1 && _s[4] && VIC_isSpriteInCurrentLine(4)) {
				if (!_s[3]) {
					if (!_s[2]) {
						_c[c] = 2;												//	Bus-Takeover cycles Sprite #4 (2,3)
					}
					else {
						_c[c] = 1;												//	Fill-Cycles between Sprite #2 and #4
					}
				}
			}
			if (c >= 2 && c <= 3 && _s[4] && VIC_isSpriteInCurrentLine(4)) {
				_c[c] = 1;														//	Sprite pointer / data Sprite #4
			}

			//	Sprite #5
			if (c == 1 && _s[5] && !_s[4] && VIC_isSpriteInCurrentLine(5)) {
				if (!_s[3]) {
					_c[c] = 2;													//	Bus-Takeover cycles Sprite #5 (1)
				}
			}
			if (c >= 2 && c <= 3 && _s[5] && VIC_isSpriteInCurrentLine(5)) {
				if (!_s[4]) {
					if (!_s[3]) {
						_c[c] = 2;												//	Bus-Takeover cycles Sprite #5 (2,3)
					}
					else {
						_c[c] = 1;												//	Fill-Cycles between Sprite #3 and #5
					}
				}
			}
			if (c >= 4 && c <= 5 && _s[5] && VIC_isSpriteInCurrentLine(5)) {
				_c[c] = 1;														//	Sprite pointer / data Sprite #5
			}

			//	Sprite #6
			if (c == 3 && _s[6] && !_s[5] && VIC_isSpriteInCurrentLine(6)) {
				if (!_s[4]) {
					_c[c] = 2;													//	Bus-Takeover cycles Sprite #6 (1)
				}
			}
			if (c >= 4 && c <= 5 && _s[6] && VIC_isSpriteInCurrentLine(6)) {
				if (!_s[5]) {
					if (!_s[4]) {
						_c[c] = 2;												//	Bus-Takeover cycles Sprite #6 (2,3)
					}
					else {
						_c[c] = 1;												//	Fill-Cycles between Sprite #4 and #6
					}
				}
			}
			if (c >= 6 && c <= 7 && _s[6] && VIC_isSpriteInCurrentLine(6)) {
				_c[c] = 1;														//	Sprite pointer / data Sprite #6
			}

			//	Sprite #7 - cont here
			if (c == 5 && _s[7] && !_s[6] && VIC_isSpriteInCurrentLine(7)) {
				if (!_s[5]) {
					_c[c] = 2;													//	Bus-Takeover cycles Sprite #7 (1)
				}
			}
			if (c >= 6 && c <= 7 && _s[7] && VIC_isSpriteInCurrentLine(7)) {
				if (!_s[6]) {
					if (!_s[5]) {
						_c[c] = 2;												//	Bus-Takeover cycles Sprite #7 (2,3)
					}
					else {
						_c[c] = 1;												//	Fill-Cycles between Sprite #5 and #7
					}
				}
			}
			if (c >= 8 && c <= 9 && _s[7] && VIC_isSpriteInCurrentLine(7)) {
				_c[c] = 1;														//	Sprite pointer / data Sprite #7
			}

			VIC_fetchGraphicsData(1);
 
			/*
				Low-Phi Phase / 6510 [VIC on steals]
				- only execute CPU instruction, if the VIC didn't block the cycle in _c-array
			*/

			if (flaggedForIRQ) {
				setIRQ(true);
				flaggedForIRQ = false;
			}
			if (flaggedAfterNextInstr) {
				flaggedForIRQ = true;
				flaggedAfterNextInstr = false;
			}
			else if (c >= 0 && c <= 62) {
				//	Regular CPU cycle
				if (_c[c] == 0) {
					if (cpuHalted) {
						//	Resume CPU
						cpuHalted = false;
					}
					//else {
						CPU_executeInstruction();
					//}
				}
				else if (_c[c] == 2 && !cpuHalted) {
					//	Execution on Takeover cycle
					CPU_executeInstruction();
				}
				else {
					//	Skipping cycle
					char output[200];
					snprintf(output, sizeof(output), "Cycle: %d(%02x) - CPU Stalled!", c, c);
					//printMsg("CPU", "BUS-TAKEOVER", string(output));
				}
				if (c == 0) {
					if (VIC_checkRasterIRQ()) {
						//flaggedAfterNextInstr = true;
						setIRQ(true);
					}
				}
			}

			c++;
			if (c == 63) {
				c = 0;
				VIC_nextScanline();
				//	TODO: Can window handling events being polled only once per frame? 
				//	(right now it's once per line)
				handleWindowEvents(event);
			}

			tickAllTimers(1);

		}
		else {
			handleWindowEvents(event);
		}
	}

	return 1;
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

//	DEBUG
uint8_t BUS_currentCycle() {
	return c;
}

void BUS_haltCPU() {
	/*char output[32];
	snprintf(output, sizeof(output), "Halt on cycle %d", c);
	printMsg("BUS", "LOG", string(output));*/
	cpuHalted = true;
}

bool BUS_takeoverActive() {
	return c[_c] == 2;
}