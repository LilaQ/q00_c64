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

SDL_Event event;					
bool unpaused = true;
bool isBadline = false;
bool rasterIRQ = false;
bool flaggedAfterNextInstr = false;
bool flaggedForIRQ = false;
uint8_t c = 13;
uint8_t cycles_left = 0;

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
				VIC_fetchGraphicsData(1);
				if (c == 1) {
					isBadline = VIC_isBadline();
					VIC_fetchSpritePointer(3);
				}
				else if (c == 3) {
					VIC_fetchSpritePointer(4);
				}
				else if (c == 5) {
					VIC_fetchSpritePointer(5);
				}
				else if (c == 7) {
					VIC_fetchSpritePointer(6);
				}
				else if (c == 9) {
					VIC_fetchSpritePointer(7);
				}
				else if (c >= 11 && c <= 15) {
					VIC_dataRefresh();
				}
				else if (c == 58) {
					VIC_fetchSpritePointer(0);
				}
				else if (c == 60) {
					VIC_fetchSpritePointer(1);
				}
				else if (c == 62) {
					VIC_fetchSpritePointer(2);
				}
 
			/*
				Low-Phi Phase
			*/

				/*
					6510
				*/
				if (flaggedForIRQ) {
					setIRQ(true);
					flaggedForIRQ = false;
				}
				if (flaggedAfterNextInstr) {
					flaggedForIRQ = true;
					flaggedAfterNextInstr = false;
				}
				if (c >= 1 && c <= 11) {
					if (cycles_left == 0) {
						//if (pendingIRQ())
							//printf("Running IRQ Routine on cycle %d\n", c);
						cycles_left = CPU_executeInstruction();
					}
					if (c == 1) {
						if (VIC_checkRasterIRQ()) {
							printf("Found RasterIRQ\n");
							if (cycles_left == 1) {
								flaggedAfterNextInstr = true;
							}
							else {
								flaggedForIRQ = true;
							}
						}
					}
					cycles_left--;
				}
				else if (c >= 12 && c <= 54) {
					if (!isBadline) {
						if (cycles_left == 0) {
							cycles_left = CPU_executeInstruction();
						}
						cycles_left--;
					}
				}
				else if (c >= 55 && c <= 63) {
					if (cycles_left == 0) {
						cycles_left = CPU_executeInstruction();
					}
					cycles_left--;
				}
				

				c++;
				if (c == 64) {
					c = 1;
					VIC_nextScanline();
				}

				tickAllTimers(1);




		}
		handleWindowEvents(event);
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