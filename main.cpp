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
uint8_t c = 0;
uint8_t _c[63];								//	1 = cycle blocked by VIC; 0 = cycle free for 6510
uint8_t _s[8] = { 1,0,1,0,0,1,0,0 };		//	1 = Sprite enabled; 0 = Sprite disabled;
uint8_t _t[8] = { 57,59,61,0,2,4,6,8 };		//	cycle no.s for each Sprite

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
	resetCPU();
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

				Observe! The left edge of the chart is not the left edge of the screen nor
					 the left edge of the beam, but the sprite x-coordinate 0. If you
					 have opened the borders, you know what I mean. A sprite can be
					 moved left from the coordinate 0 by using x-values greater than 500.
				 ___________
				|  _______  |<-- Maximum sized video screen
				|||       | |
				|||       |<-- Normal C64 screen
				|||       | |
				|||_______| |
				||          |
				||__________|
				 ^ Sprite coordinate 0

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

				/*
					VIC Stub
				*/
				if (c == 1) {
					isBadline = VIC_isBadline();
					VIC_fetchSpritePointer(3);
					_s[3] = VIC_isSpriteEnabled(3);
					if (_s[3]) {
						VIC_fetchSpriteDataBytes(3);
					}
				}
				else if (c == 3) {
					VIC_fetchSpritePointer(4);
					_s[4] = VIC_isSpriteEnabled(4);
					if (_s[4]) {
						VIC_fetchSpriteDataBytes(4);
					}
				}
				else if (c == 5) {
					VIC_fetchSpritePointer(5);
					_s[5] = VIC_isSpriteEnabled(5);
					if (_s[5]) {
						VIC_fetchSpriteDataBytes(5);
					}
				}
				else if (c == 7) {
					VIC_fetchSpritePointer(6);
					_s[6] = VIC_isSpriteEnabled(6);
					if (_s[6]) {
						VIC_fetchSpriteDataBytes(6);
					}
				}
				else if (c == 9) {
					VIC_fetchSpritePointer(7);
					_s[7] = VIC_isSpriteEnabled(7);
					if (_s[7]) {
						VIC_fetchSpriteDataBytes(7);
					}
				}
				else if (c >= 11 && c <= 15) {
					//	Fetch attributes for Sprites (height, width, position etc.)
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
					//	Check for each sprite, that is visible for this line, if it needs a sprite-skip (2cy) or bus-transfer (3cy)
					for (uint8_t i = 0; i < 8; i++) {
						if (_s[i] && VIC_isSpriteInLine(i)) {
							_c[_t[i]] = 1;
							_c[_t[i] + 1] = 1;
							if (checkBusTransfer(i)) {
								_c[_t[i] - 1] = 1;
								_c[_t[i] - 2] = 1;
								_c[_t[i] - 3] = 1;
							}
							if (checkSpriteGap(i)) {
								_c[_t[i] - 1] = 1;
								_c[_t[i] - 2] = 1;
							}
						}
					}
					/*if (_s[0]) {
						for (int i = 0; i < 63; i++) {
							printf("%02d ", i);
						}
						printf("\n");
						for (int i = 0; i < 63; i++) {
							printf("%02d ", _c[i]);
						}
					}*/
				}
				else if (c == 58) {
					VIC_fetchSpritePointer(0);
					_s[0] = VIC_isSpriteEnabled(0);
					if (_s[0]) {
						VIC_fetchSpriteDataBytes(0);
					}
				}
				else if (c == 60) {
					VIC_fetchSpritePointer(1);
					_s[1] = VIC_isSpriteEnabled(1);
					if (_s[1]) {
						VIC_fetchSpriteDataBytes(1);
					}
				}
				else if (c == 62) {
					VIC_fetchSpritePointer(2);
					_s[2] = VIC_isSpriteEnabled(2);
					if (_s[2]) {
						VIC_fetchSpriteDataBytes(2);
					}
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
				else if (c >= 0 && c <= 10) {
					if (_c[c] == 0)
						CPU_executeInstruction();
					if (c == 0) {
						if (VIC_checkRasterIRQ()) {
							flaggedAfterNextInstr = true;
						}
					}
				}
				//	Graphics Fetch
				else if (c >= 11 && c <= 53) {
					if (!isBadline) {
						CPU_executeInstruction();
					}
				}
				else if (c >= 54 && c <= 62) {
					if (_c[c] == 0)
						CPU_executeInstruction();
				}
				

				c++;
				if (c == 63) {
					c = 0;
					VIC_nextScanline();
					handleWindowEvents(event);
				}

				tickAllTimers(1);

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
uint8_t currentCycle() {
	return c;
}