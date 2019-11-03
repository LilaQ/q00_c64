#pragma once
#include <stdint.h>
#include <iostream>
#include <string>
#include <array>
#include "ppu.h"
#include "mmu.h"
#include "cpu.h"
#include "wmu.h"
#include "SDL2/include/SDL.h"
using namespace std;

array<uint8_t, 0x31> VIC_REGISTERS;
int cycles_on_current_scanline = 0;
int last_cycles_on_current_scanline = 0;
uint16_t current_scanline = 0;
uint16_t raster_irq_row = 0;
bool DRAW_BORDER = true;

IRQ_STATUS irq_status;
IRQ_MASK irq_mask;
SDL_Renderer* renderer;
SDL_Window* window;
SDL_Texture* texture;

unsigned char VRAM[400 * 284 * 3];	//	RGB - 320 * 200 pixel inside, with border 402 * 284

const unsigned char COLORS[16][3] = {
	{0x00, 0x00, 0x00},
	{0xff, 0xff, 0xff},
	{0x81, 0x33, 0x38},
	{0x75, 0xce, 0xc8},
	{0x8e, 0x3c, 0x97},
	{0x56, 0xac, 0x4d},
	{0x2e, 0x2c, 0x9b},
	{0xed, 0xf1, 0x71},
	{0x8e, 0x50, 0x29},
	{0x55, 0x38, 0x00},
	{0xc4, 0x6c, 0x71},
	{0x4a, 0x4a, 0x4a},
	{0x7b, 0x7b, 0x7b},
	{0xa9, 0xff, 0x9f},
	{0x70, 0x6d, 0xeb},
	{0xb2, 0xb2, 0xb2}
};

void drawFrame() {
	SDL_UpdateTexture(texture, NULL, VRAM, 400 * sizeof(unsigned char) * 3);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void initPPU(string filename) {

	//	init and create window and renderer
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_CreateWindowAndRenderer(400, 284, 0, &window, &renderer);
	SDL_SetWindowSize(window, 800, 588);
	//SDL_RenderSetLogicalSize(renderer, 512, 480);
	SDL_SetWindowResizable(window, SDL_TRUE);

	//	for fast rendering, create a texture
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 400, 284);
	initWindow(window, filename);

	drawFrame();
}

void renderByCycles(int16_t current_scanline, uint16_t cycles_on_current_scanline, SCREEN_POS SCREENPOS) {

	//	remove the offset of VBLANK (for drawing/rendering)
	current_scanline -= 14;

	//	calculate the pixels we need to draw at this cycle position
	uint16_t pixel_from = (cycles_on_current_scanline - 6 - 1) * 8;
	uint16_t pixel_to = (cycles_on_current_scanline - 6) * 8;

	//	Textmode Variables
	uint16_t chrrom = 1024 * (VIC_REGISTERS[0x18] & 0b1110);
	uint16_t screen = 0x400 * ((VIC_REGISTERS[0x18] & 0b11110000) / 0x10);
	uint8_t offset_x = VIC_REGISTERS[0x16] & 0b111;
	uint8_t border_mode_offset = (VIC_REGISTERS[0x16] & 0b1000) ? 0 : 16;	//	38 cols if off, 40 cols if on
	uint16_t colorram = 0xd800;

	//	Bitmap Variables
	uint16_t bmp_start_address = (VIC_REGISTERS[0x18] & 0b1000) ? 0x2000 : 0x0000;
	uint16_t bmp_color_address = ((VIC_REGISTERS[0x18] & 0b11110000) >> 4) * 0x400;

	//	Render Line (if visible scanline)
	if (current_scanline >= 0) {
		for (int i = pixel_from; i < pixel_to; i++) {

			//	Basic VRAM start address and OFFSET of the pixel we want to write to in this iteration
			uint32_t ADR = (current_scanline * 400 * 3) + ((i + offset_x) * 3);
			uint16_t OFFSET = ((current_scanline - 42) / 8) * 40 + (i - 40) / 8;

			//	TEXTMODE
			if ((VIC_REGISTERS[0x11] & 0b100000) == 0 || VIC_REGISTERS[0x11] & 0b1000000) {

				//	inside

				uint8_t char_id = readFromMemByVIC(screen + OFFSET);
				uint8_t bg_color = VIC_REGISTERS[0x21];

				//	EXTENDED BG COLOR MODE
				if (VIC_REGISTERS[0x11] & 0b1000000) {
					bg_color = VIC_REGISTERS[0x21 + ((char_id >> 6) & 0b11)];
					char_id &= 0b111111;
				}
				uint16_t char_address = chrrom + (char_id * 8) + ((current_scanline - 42) % 8);
				uint8_t color = readFromMemByVIC(colorram + OFFSET);
				uint8_t chr = readFromMemByVIC(char_address);

				//	NORMAL MODE
				if ((VIC_REGISTERS[0x16] & 0b10000) == 0) {
					VRAM[ADR] = ((chr & (1 << (7 - (i - 40) % 8))) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
					VRAM[ADR + 1] = ((chr & (1 << (7 - (i - 40) % 8))) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
					VRAM[ADR + 2] = ((chr & (1 << (7 - (i - 40) % 8))) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
				}

				//	MULTICOLOR MODE
				else {
					/*
						%00 = $d021 (Hintergrundfarbe)
						%01 = $d022
						%10 = $d023
						%11 = Farbe laut Farb-RAM (ab $d800)
					*/
					uint8_t index = (7 - (i % 8));
					uint8_t current_byte = chr;
					//	If we are at bit 0 of the 2 bits...
					uint8_t high_bit = ((current_byte & (1 << (index + 1))) > 0) ? 1 : 0;
					uint8_t low_bit = ((current_byte & (1 << index)) > 0) ? 1 : 0;
					//	Else, we are at bit 1 of the 2 bits
					if (index % 2) {
						high_bit = ((current_byte & (1 << index)) > 0) ? 1 : 0;
						low_bit = ((current_byte & (1 << (index - 1))) > 0) ? 1 : 0;
						//	edge case, index is at the end, we jump to the next byte
						if (index == 0) {
							low_bit = (((readFromMemByVIC(chrrom + (char_id * 8) + ((current_scanline - 40) % 8)) + 1) & (1 << (index + 1))) > 0) ? 1 : 0;
						}
					}
					uint8_t color_choice = (high_bit << 1) | low_bit;
					//	4 bits for color; highest bit enables low-res multicolor mode...
					if (color & 0b1000) {
						switch (color_choice)
						{
						case 0x00:	//	BG Color
							color = VIC_REGISTERS[0x21];
							break;
						case 0x01:
							color = VIC_REGISTERS[0x22];
							break;
						case 0x02:
							color = VIC_REGISTERS[0x23];
							break;
						}
						VRAM[ADR] = COLORS[color][0];
						VRAM[ADR + 1] = COLORS[color][1];
						VRAM[ADR + 2] = COLORS[color][2];
					}
					//	...else it's still going to be hires single color mode
					else {
						VRAM[ADR] = ((chr & (1 << (7 - (i - 40) % 8))) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
						VRAM[ADR + 1] = ((chr & (1 << (7 - (i - 40) % 8))) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
						VRAM[ADR + 2] = ((chr & (1 << (7 - (i - 40) % 8))) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
					}
				}
			}
			//	BITMAP MODE
			else {
				uint16_t BMP_OFFSET = (OFFSET / 40) * 320 + (OFFSET % 40) * 8 + ((current_scanline - 42) % 8);
				uint8_t bit = readFromMemByVIC(bmp_start_address + BMP_OFFSET) & (0b10000000 >> (i % 8));
				uint8_t color_index = (bit) ? (readFromMemByVIC(bmp_color_address + OFFSET) & 0b11110000) >> 4 : readFromMemByVIC(bmp_color_address + OFFSET) & 0b1111;

				//	MULTICOLOR MODE
				if ((VIC_REGISTERS[0x16] & 0b10000)) {
					uint8_t index = (7 - (i % 8));
					uint8_t current_byte = readFromMemByVIC(bmp_start_address + BMP_OFFSET);
					//	If we are at bit 0 of the 2 bits...
					uint8_t high_bit = ((current_byte & (1 << (index + 1))) > 0) ? 1 : 0;
					uint8_t low_bit = ((current_byte & (1 << index)) > 0) ? 1 : 0;
					//	Else, we are at bit 1 of the 2 bits
					if (index % 2) {
						high_bit = ((current_byte & (1 << index)) > 0) ? 1 : 0;
						low_bit = ((current_byte & (1 << (index - 1))) > 0) ? 1 : 0;
						//	edge case, index is at the end, we jump to the next byte
						if (index == 0) {
							low_bit = (((readFromMemByVIC(bmp_start_address + BMP_OFFSET + 1)) & (1 << (index + 1))) > 0) ? 1 : 0;
						}
					}
					uint8_t color_choice = (high_bit << 1) | low_bit;
					switch (color_choice)
					{
					case 0x00:	//	BG Color
						color_index = VIC_REGISTERS[0x21];
						break;
					case 0x01:
						color_index = (readFromMemByVIC(bmp_color_address + OFFSET) & 0b11110000) >> 4;
						break;
					case 0x02:
						color_index = readFromMemByVIC(bmp_color_address + OFFSET) & 0b1111;
						break;
					case 0x03:
						color_index = readFromMemByVIC(0xd800 + OFFSET) & 0b1111;
						break;
					}
					uint8_t bg_color = VIC_REGISTERS[0x21];
				}

				VRAM[ADR] = COLORS[color_index][0];
				VRAM[ADR + 1] = COLORS[color_index][1];
				VRAM[ADR + 2] = COLORS[color_index][2];
			}

			//	border
			uint8_t border_color = VIC_REGISTERS[0x20];
			if (SCREENPOS == SCREEN_POS::BORDER_LR || SCREENPOS == SCREEN_POS::BORDER_TB) {
				if (DRAW_BORDER) {
					VRAM[(current_scanline * 400 * 3) + (i * 3)] = COLORS[border_color][0];
					VRAM[(current_scanline * 400 * 3) + (i * 3) + 1] = COLORS[border_color][1];
					VRAM[(current_scanline * 400 * 3) + (i * 3) + 2] = COLORS[border_color][2];
				}
			}
		}
	}
}

void stepPPU(uint8_t cpu_cyc) {
	while (--cpu_cyc) {
		//	Raster Ray
		cycles_on_current_scanline++;
		if (cycles_on_current_scanline >= 63) {
			cycles_on_current_scanline = 0;
			current_scanline++;
			if (current_scanline >= 312) {
				current_scanline = 0;
			}
		}

		//	VBLANK (top of the screen)
		if (current_scanline >= 0 && current_scanline <= 13) {
			//	actually draw the screen
			if (cycles_on_current_scanline == 0 && current_scanline == 0) {
				drawFrame();
			}
		}

		//	Border and Screen
		else if (current_scanline >= 14 && current_scanline <= 297) {

			//	Left HBLANK
			if (cycles_on_current_scanline >= 0 && cycles_on_current_scanline <= 5) {
				//	do nothing
			}
			//	Left Border
			else if (cycles_on_current_scanline >= 6 && cycles_on_current_scanline <= 11) {
				renderByCycles(current_scanline, cycles_on_current_scanline, SCREEN_POS::BORDER_LR);
			}
			//	Screen
			else if (cycles_on_current_scanline >= 12 && cycles_on_current_scanline <= 51) {
				if (current_scanline >= 14 && current_scanline <= 55) {
					renderByCycles(current_scanline, cycles_on_current_scanline, SCREEN_POS::BORDER_TB);
				}
				else if	(current_scanline >= 56 && current_scanline <= 255) {
					renderByCycles(current_scanline, cycles_on_current_scanline, SCREEN_POS::SCREEN);
				}
				else if (current_scanline >= 256 && current_scanline <= 297) {
					renderByCycles(current_scanline, cycles_on_current_scanline, SCREEN_POS::BORDER_TB);
				}
			}
			//	Right Border
			else if (cycles_on_current_scanline >= 52 && cycles_on_current_scanline <= 56) {
				renderByCycles(current_scanline, cycles_on_current_scanline, SCREEN_POS::BORDER_LR);
			}
			//	Right HBLANK
			else if (cycles_on_current_scanline >= 57 && cycles_on_current_scanline <= 62) {
				//	do nothing
			}
			
		}

		//	VBLANK (bottom of the screen)
		else if (current_scanline >= 298 && current_scanline <= 311) {
			//	Set VBlank IRQ
			if (cycles_on_current_scanline == 0 && current_scanline == 298) {
				setIRQ(true);
				printf("irq vblank -->");
			}
		}

		//	Rasterzeileninterrupt
		if (irq_mask.irq_can_be_cause_by_rasterline && cycles_on_current_scanline == 0) {	//	enabled?
			if (current_scanline == raster_irq_row) {
				irq_status.setFlags(0b10000001);		//	set "IRQ FROM VIC", and as reason set "IRQ FROM RASTERLINE"
				setIRQ(true);
				printf("raster vblank -->");
				return;
			}
		}
	}
}

uint8_t getCurrentScanline() {
	return current_scanline & 0xff;
}

void writeVICregister(uint16_t adr, uint8_t val) {
	switch (adr)
	{
		case 0xd011:
			raster_irq_row &= 0b11111111;
			raster_irq_row |= ((val & 0b10000000) << 1);
			break;
		case 0xd012:			//	Set Rasterzeilen IRQ
			raster_irq_row &= 0b100000000;
			raster_irq_row |= val;
			break;
		case 0xd019:			//	Clear IRQ flags, that are no longer needed
			irq_status.clearFlags(val);
			break;
		case 0xd01a:			//	IRQ Mask (what is allowed to cause IRQs)
			irq_mask.set(val);
			break;
		default:
			break;
	}
	VIC_REGISTERS[adr % 0xd000] = val;
}

uint8_t readVICregister(uint16_t adr) {
	switch (adr) {
		case 0xd011:	
			return raster_irq_row;
			break;
		case 0xd012:			//	Current Scanline
			return getCurrentScanline();
			break;
		case 0xd019:			//	IRQ flags, (active IRQs)
			return irq_status.get();
			break;
		case 0xd01a:			//	IRQ Mask (what is allowed to cause IRQs)
			return irq_mask.get();
			break;
		default:
			return VIC_REGISTERS[adr % 0xd000];
			break;
	}
}