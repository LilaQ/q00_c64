#pragma once
#include <stdint.h>
#include <iostream>
#include <string>
#include <array>
#include "ppu.h"
#include "mmu.h"
#include "cpu.h"
#include "wmu.h"
#include "cia.h"
#include "SDL2/include/SDL.h"
#include <array>
#include <vector>
using namespace std;

array<uint8_t, 0x31> VIC_REGISTERS;
uint16_t cycles_on_current_scanline = 0;
uint16_t current_scanline = 0;
uint16_t raster_irq_row = 0;
bool DRAW_BORDER = true;
int32_t ADR = 0;

IRQ_STATUS irq_status;
IRQ_MASK irq_mask;
SDL_Renderer* renderer, *debug_renderer;
SDL_Window* window, *debug_window;
SDL_Texture* texture, *debug_texture;

vector<uint8_t> VRAM(400 * 284 * 3);	//	RGB - 320 * 200 pixel inside, with border 402 * 284
vector<uint8_t> SPRITES(24 * 21 * 3);	//	RGB - 320 * 200 pixel inside, with border 402 * 284

const unsigned char COLORS[16][3] = {
	{0x00, 0x00, 0x00},
	{0xff, 0xff, 0xff},
	{0x81, 0x33, 0x38},
	{0x75, 0xce, 0xc8},
	{0x8e, 0x3c, 0x97},
	{0x56, 0xac, 0x4d},
	{0x48, 0x3a, 0xaa},
	{0xed, 0xf1, 0x71},
	{0x8e, 0x50, 0x29},
	{0x55, 0x38, 0x00},
	{0xc4, 0x6c, 0x71},
	{0x4a, 0x4a, 0x4a},
	{0x7b, 0x7b, 0x7b},
	{0xa9, 0xff, 0x9f},
	{0x86, 0x7a, 0xde},
	{0xb2, 0xb2, 0xb2}
};

void drawFrame() {
	SDL_UpdateTexture(texture, NULL, (uint8_t*)VRAM.data(), 400 * sizeof(unsigned char) * 3);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}
void debugSprites(uint16_t pixel_on_scanline, uint16_t scanline, uint32_t ADR);
void drawDebug() {

	for (int i = 0; i < 21; i++) {
		for (int j = 0; j < 24; j++) {
			SPRITES[(i * 24 * 3) + (j * 3)] = 0x00;
			SPRITES[(i * 24 * 3) + (j * 3) + 1] = 0x00;
			SPRITES[(i * 24 * 3) + (j * 3) + 2] = 0x00;
			debugSprites(j, i, (i * 24 * 3) + (j * 3));
		}
	}

	SDL_UpdateTexture(debug_texture, NULL, (uint8_t*)SPRITES.data(), 24 * sizeof(unsigned char) * 3);
	SDL_RenderCopy(debug_renderer, debug_texture, NULL, NULL);
	SDL_RenderPresent(debug_renderer);
}

void initPPU(string filename) {

	//	init and create window and renderer
	SDL_Init(SDL_INIT_VIDEO);
	//SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_CreateWindowAndRenderer(400, 284, 0, &window, &renderer);
	SDL_SetWindowSize(window, 800, 588);
	//SDL_RenderSetLogicalSize(renderer, 512, 480);
	SDL_SetWindowResizable(window, SDL_TRUE);

	//	for fast rendering, create a texture
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 400, 284);
	initWindow(window, filename);

	VIC_REGISTERS.fill(0x00);


	SDL_CreateWindowAndRenderer(24, 21, 0, &debug_window, &debug_renderer);
	debug_texture = SDL_CreateTexture(debug_renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 24, 21);
	SDL_SetWindowSize(debug_window, 240, 210);


	drawFrame();
}

void setScreenSize(UI_SCREEN_SIZE scr_s) {
	switch (scr_s)
	{
		case UI_SCREEN_SIZE::ORIG:
			SDL_SetWindowSize(window, 400, 304);
			break;
		case UI_SCREEN_SIZE::X2:
			SDL_SetWindowSize(window, 800, 588);
			break;
		default:
			break;
	}
}

void debugSprites(uint16_t pixel_on_scanline, uint16_t scanline, uint32_t ADR) {

	//	adjust x and y to accomodate to the sprite-raster that sprites can be drawn to
	int16_t x = pixel_on_scanline;
	int16_t y = scanline;
	uint16_t screen_start = 0x40 * (VIC_REGISTERS[0x18] & 0b11110000);


	//	check if sprite is enabled first
	if (VIC_REGISTERS[0x15] & (1 << 1)) {

		//	actual sprite data
		uint16_t sprite_data = (readFromMemByVIC(screen_start + 0x03f8 + 1) * 64);

		//	24x21 is the normal size of a sprite
		bool width_doubled = (VIC_REGISTERS[0x1d] & (1 << 1)) > 0;
		bool height_doubled = (VIC_REGISTERS[0x17] & (1 << 1)) > 0;
		bool prio_background = (VIC_REGISTERS[0x1b] & (1 << 1)) > 0;
		bool multicolor = (VIC_REGISTERS[0x1c] & (1 << 1)) > 0;
		uint8_t width = 24 * (1 + width_doubled);
		uint8_t height = 21 * (1 + height_doubled);

		//	check if sprite potentially covers the current pixel
		if ((x >= 0 && x < (width)) &&
			(y >= 0 && y < (height))) {

			//	3 byte per line, 21 lines
			uint8_t spr_x = x;
			uint8_t spr_y = y;
			uint8_t color_bit_h = readFromMemByVIC(sprite_data + (spr_y * 3) + (spr_x / 8)) & (1 << (7 - (spr_x % 8)));
			uint8_t color_bit_l = 0x00;
			uint8_t color_bits = 0x00;
			if (multicolor) {
				//	looking at high bit
				if (spr_x % 2 == 0) {
					color_bit_h = readFromMemByVIC(sprite_data + (spr_y * 3) + (spr_x / 8)) & (1 << (7 - (spr_x % 8)));
					color_bit_l = readFromMemByVIC(sprite_data + (spr_y * 3) + (spr_x / 8)) & (1 << (7 - ((spr_x + 1) % 8)));
				}
				else if (spr_x % 2 == 1) {
					color_bit_l = readFromMemByVIC(sprite_data + (spr_y * 3) + (spr_x / 8)) & (1 << (7 - (spr_x % 8)));
					color_bit_h = readFromMemByVIC(sprite_data + (spr_y * 3) + ((spr_x - 1) / 8)) & (1 << (7 - ((spr_x - 1) % 8)));
				}
			}
			color_bit_h = color_bit_h > 0;
			color_bit_l = color_bit_l > 0;
			color_bits = (color_bit_h << 1) | color_bit_l;
			uint8_t color = 0x00;
			//	single color (hires)
			if (!multicolor) {
				if (color_bit_h) {
					color = VIC_REGISTERS[0x27 + 1];
					SPRITES[ADR] = COLORS[color][0];
					SPRITES[ADR + 1] = COLORS[color][1];
					SPRITES[ADR + 2] = COLORS[color][2];
				}
			}
			//	multicolor
			else {
				if (color_bits) {
					switch (color_bits)
					{
					case 0b00:
						printf("This shouldn't be happening\n");
						break;
					case 0b01:
						color = VIC_REGISTERS[0x25];
						break;
					case 0b10:
						color = VIC_REGISTERS[0x27 + 1];						
						break;
					case 0b11:
						color = VIC_REGISTERS[0x26];
						break;
					}
					SPRITES[ADR] = COLORS[color][0];
					SPRITES[ADR + 1] = COLORS[color][1];
					SPRITES[ADR + 2] = COLORS[color][2];
				}
			}

		}

	}

}

struct SPRITE {
	//	actual sprite data
	uint16_t screen_start;
	uint16_t sprite_data;

	//	24x21 is the normal size of a sprite
	bool width_doubled;
	bool height_doubled;
	bool prio_background;
	bool multicolor;
	uint8_t width;
	uint8_t height;
	uint16_t pos_x;
	uint16_t pos_y;

	void reinit(uint8_t i, uint8_t x) {
		//	actual sprite data
		screen_start = 0x40 * (VIC_REGISTERS[0x18] & 0b11110000);

		//	24x21 is the normal size of a sprite
		width_doubled = (VIC_REGISTERS[0x1d] & (1 << i)) > 0;
		height_doubled = (VIC_REGISTERS[0x17] & (1 << i)) > 0;
		prio_background = (VIC_REGISTERS[0x1b] & (1 << i)) > 0;
		multicolor = (VIC_REGISTERS[0x1c] & (1 << i)) > 0;
		width = 24 * (1 + width_doubled);
		height = 21 * (1 + height_doubled);
		pos_x = (VIC_REGISTERS[0x00 + (i * 2)]) | (((VIC_REGISTERS[0x10] & (1 << i)) > 0) << 8);
		pos_y = VIC_REGISTERS[0x01 + (i * 2)];

		if(x == pos_x)
			sprite_data = (readFromMemByVIC(screen_start + 0x03f8 + i) * 64);
	}
};

vector<SPRITE> SPRITES_VEC(8);

void renderSprites(uint16_t pixel_on_scanline, uint16_t scanline, uint32_t ADR) {

	//	adjust x and y to accomodate to the sprite-raster that sprites can be drawn to
	int16_t x = pixel_on_scanline - 24;
	int16_t y = scanline + 15;
	uint16_t screen_start = 0x40 * (VIC_REGISTERS[0x18] & 0b11110000);

	//	iterate through all 8 available sprites
	for (int8_t i = 7; i >= 0; i--) {

		//	check if sprite is enabled first
		if (VIC_REGISTERS[0x15] & (1 << i)) {

			//	check if sprite potentially covers the current pixel
			if ((x >= SPRITES_VEC[i].pos_x && x < (SPRITES_VEC[i].pos_x + SPRITES_VEC[i].width)) &&
				(y >= SPRITES_VEC[i].pos_y && y < (SPRITES_VEC[i].pos_y + SPRITES_VEC[i].height))) {

				//	3 byte per line, 21 lines
				uint8_t spr_x = x - SPRITES_VEC[i].pos_x;
				uint8_t spr_y = y - SPRITES_VEC[i].pos_y;

				uint8_t color_bit_h = readFromMemByVIC(SPRITES_VEC[i].sprite_data + (spr_y * 3) + (spr_x / 8)) & (1 << (7 - (spr_x % 8)));
				uint8_t color_bit_l = 0x00;
				uint8_t color_bits = 0x00;
				if (SPRITES_VEC[i].multicolor) {
					//	looking at high bit
					if (spr_x % 2 == 0) {
						color_bit_h = readFromMemByVIC(SPRITES_VEC[i].sprite_data + (spr_y * 3) + (spr_x / 8)) & (1 << (7 - (spr_x % 8)));
						color_bit_l = readFromMemByVIC(SPRITES_VEC[i].sprite_data + (spr_y * 3) + (spr_x / 8)) & (1 << (7 - ((spr_x + 1) % 8)));
					}
					else if (spr_x % 2 == 1) {
						color_bit_l = readFromMemByVIC(SPRITES_VEC[i].sprite_data + (spr_y * 3) + (spr_x / 8)) & (1 << (7 - (spr_x % 8)));
						color_bit_h = readFromMemByVIC(SPRITES_VEC[i].sprite_data + (spr_y * 3) + ((spr_x - 1) / 8)) & (1 << (7 - ((spr_x - 1) % 8)));
					}
				}
				color_bit_h = color_bit_h > 0;
				color_bit_l = color_bit_l > 0;
				color_bits = (color_bit_h << 1) | color_bit_l;
				uint8_t color = 0x00;
				//	single color (hires)
				if (!SPRITES_VEC[i].multicolor) {
					if (color_bit_h) {
						color = VIC_REGISTERS[0x27 + i];
						VRAM[ADR] = COLORS[color][0];
						VRAM[ADR + 1] = COLORS[color][1];
						VRAM[ADR + 2] = COLORS[color][2];
					}
				}
				//	multicolor
				else {
					if (color_bits) {
						switch (color_bits)
						{
							case 0b00:
								printf("This shouldn't be happening\n");
								break;
							case 0b01:
								color = VIC_REGISTERS[0x25];
								break;
							case 0b10:
								color = VIC_REGISTERS[0x27 + i];
								break;
							case 0b11:
								color = VIC_REGISTERS[0x26];
								break;	
						}
						VRAM[ADR] = COLORS[color][0];
						VRAM[ADR + 1] = COLORS[color][1];
						VRAM[ADR + 2] = COLORS[color][2];
					}
				}
				//	DEBUG green dot upper left corner
				if (spr_x == 0 && spr_y == 0) {
					VRAM[ADR] = COLORS[color][0];
					VRAM[ADR + 1] = COLORS[color][1];
					VRAM[ADR + 2] = COLORS[color][2];
				}
			}

		}

	}
}

void renderByPixels(uint16_t scanline, int16_t x, SCREEN_POS SCREENPOS) {
	
	uint8_t bank_no = 0b11 - (readCIA2DataPortA() & 0b11);

	//	Textmode Variables
	uint16_t chrrom = 1024 * (VIC_REGISTERS[0x18] & 0b1110);
	uint16_t screen = 0x40 * (VIC_REGISTERS[0x18] & 0b11110000);
	uint8_t offset_x = VIC_REGISTERS[0x16] & 0b111;
	uint16_t colorram = 0xd800;

	//	Bitmap Variables
	uint16_t bmp_start_address = ((VIC_REGISTERS[0x18] & 0b1000) ? 0x2000 : 0x0000);
	uint16_t bmp_color_address = ((VIC_REGISTERS[0x18] & 0b11110000) >> 4) * 0x400;


	//	DEBUG WARP
	//if ((VIC_REGISTERS[0x11] & 0b10000) == 1) {
	if(1) {

		if (SCREENPOS != SCREEN_POS::NO_RENDER) {
			//	Basic VRAM start address and OFFSET of the pixel we want to write to in this iteration
			ADR = (scanline * 400 * 3) + ((x + offset_x) * 3);
			uint16_t OFFSET = (uint16_t)(((scanline - 35) / 8) * 40 + (x - 48) / 8);

			//	TEXTMODE
			if (((SCREENPOS == SCREEN_POS::SCREEN) ||
				(SCREENPOS == SCREEN_POS::ROW_38_AREA) ||
				(SCREENPOS == SCREEN_POS::COL_38_AREA)) &&
				((VIC_REGISTERS[0x11] & 0b100000) == 0)
				) {

				//	inside

				uint8_t char_id = readFromMemByVIC(screen + OFFSET);
				uint8_t bg_color = VIC_REGISTERS[0x21] & 0xf;

				//	EXTENDED BG COLOR MODE
				if (VIC_REGISTERS[0x11] & 0b1000000) {
					bg_color = VIC_REGISTERS[0x21 + ((char_id >> 6) & 0b11)] & 0xf;
					char_id &= 0b111111;
				}
				uint16_t char_address = chrrom + (char_id * 8) + ((scanline - 35) % 8);
				uint8_t color = readFromMemByVIC(colorram + OFFSET) & 0xf;
				uint8_t chr = readFromMemByVIC(char_address);

				//	NORMAL MODE
				if ((VIC_REGISTERS[0x16] & 0b10000) == 0) {
					VRAM[ADR] = ((chr & (1 << (7 - (x - 48) % 8))) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
					VRAM[ADR + 1] = ((chr & (1 << (7 - (x - 48) % 8))) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
					VRAM[ADR + 2] = ((chr & (1 << (7 - (x - 48) % 8))) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
				}

				//	MULTICOLOR MODE
				else {
					/*
						%00 = $d021 (Hintergrundfarbe)
						%01 = $d022
						%10 = $d023
						%11 = Farbe laut Farb-RAM (ab $d800)
					*/
					uint8_t index = (7 - (x % 8));
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
							low_bit = (((readFromMemByVIC(chrrom + (char_id * 8) + ((scanline - 35) % 8)) + 1) & (1 << (index + 1))) > 0) ? 1 : 0;
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
						case 0x03:
							color = readFromMemByVIC(OFFSET + colorram) % 8;
							break;
						}
						color &= 0xf;
						VRAM[ADR] = COLORS[color][0];
						VRAM[ADR + 1] = COLORS[color][1];
						VRAM[ADR + 2] = COLORS[color][2];
					}
					//	...else it's still going to be hires single color mode
					else {
						color &= 0xf;
						VRAM[ADR] = ((chr & (1 << (7 - (x - 48) % 8))) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
						VRAM[ADR + 1] = ((chr & (1 << (7 - (x - 48) % 8))) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
						VRAM[ADR + 2] = ((chr & (1 << (7 - (x - 48) % 8))) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
					}
				}
			}
			//	BITMAP MODE
			else if ((SCREENPOS == SCREEN_POS::SCREEN) ||
				(SCREENPOS == SCREEN_POS::COL_38_AREA) ||
				(SCREENPOS == SCREEN_POS::ROW_38_AREA)
				) {
				uint16_t BMP_OFFSET = (OFFSET / 40) * 320 + (OFFSET % 40) * 8 + ((scanline - 35) % 8);
				uint8_t bit = readFromMemByVIC(bmp_start_address + BMP_OFFSET) & (0b10000000 >> (x % 8));
				uint8_t color_index = (bit) ? (readFromMemByVIC(bmp_color_address + OFFSET) & 0b11110000) >> 4 : readFromMemByVIC(bmp_color_address + OFFSET) & 0b1111;

				//	MULTICOLOR MODE
				if ((VIC_REGISTERS[0x16] & 0b10000)) {
					uint8_t index = (7 - (x % 8));
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
				}
				color_index &= 0xf;

				VRAM[ADR] = COLORS[color_index][0];
				VRAM[ADR + 1] = COLORS[color_index][1];
				VRAM[ADR + 2] = COLORS[color_index][2];
			}

			//	sprites
			renderSprites(x, scanline, ADR);

			//	border
			uint8_t border_color = VIC_REGISTERS[0x20] & 0xf;
			if (((SCREENPOS == SCREEN_POS::BORDER_LR || SCREENPOS == SCREEN_POS::BORDER_TB) && DRAW_BORDER) ||
				((SCREENPOS == SCREEN_POS::SCREEN) && ((VIC_REGISTERS[0x11] & 0b10000) == 0))) {
				if (ADR > 340797 || ADR < 0)
					printf("ASSERTION! Mem addressed out of range! %d\n", ADR);
				VRAM[(scanline * 400 * 3) + (x * 3)] = COLORS[border_color][0];
				VRAM[(scanline * 400 * 3) + (x * 3) + 1] = COLORS[border_color][1];
				VRAM[(scanline * 400 * 3) + (x * 3) + 2] = COLORS[border_color][2];

			}
		}
	}
}

/*
					  Raster-  Takt-   sichtb.  sichtbare
	VIC-II    System  zeilen   zyklen  Zeilen   Pixel/Zeile
	-------------------------------------------------------
	6569      PAL      312       63      284       403
*/
bool ldr = false;
void logDraw() {
	ldr = true;
}

/*	REWRITE FROM HERE ON		*/
uint16_t VIC_scanline = 0;
uint16_t VIC_scr_x = 404;
SCREEN_POS VIC_scr_pos = SCREEN_POS::NO_RENDER;

//	DEBUG
uint8_t currentScanline() {
	return VIC_scanline;
}

bool VIC_isBadline() {
	//	scanline 16 is the first scanline of the top border
	//	the top border is 35 scanlines high, therefore scanline 51 is the first scanline of 'SCREEN'
	//if (VIC_scanline >= 51 && VIC_scanline <= 250) {
	if (VIC_scanline >= 0x30 && VIC_scanline <= 0xf7) {
		//	compare to YSCROLL
		if ((VIC_REGISTERS[0x11] & 0b111) == (VIC_scanline & 0b111)) {
			//printf("%d is a badline [ Y-SCROLL %d ] \n", (VIC_scanline - 51), (VIC_REGISTERS[0x11] & 0b111));
			if((readVICregister(0x11) & 0b10000) > 0)
				return true;
		}
	}
	//printf("%d is a NOT badline [ Y-SCROLL %d ] \n", (VIC_scanline - 51), (VIC_REGISTERS[0x11] & 0b111));
	return false;
}

bool VIC_checkRasterIRQ() {
	if (VIC_scanline == raster_irq_row) {
		irq_status.setFlags(0b10000001);		//	set "IRQ FROM VIC", and as reason set "IRQ FROM RASTERLINE"
		return true;
	}
	return false;
}

void VIC_fetchGraphicsData(uint8_t cycle) {

	for(uint8_t i = 0; i < 8; i++) {

		if (VIC_scanline >= 0 && VIC_scanline < 16) {			//	VBlank Top
			VIC_scr_pos = SCREEN_POS::NO_RENDER;
		}
		else if (VIC_scanline >= 16 && VIC_scanline < 51) {		//	Top Border
			if ((VIC_scr_x >= 0 && VIC_scr_x < 24) ||
				(VIC_scr_x >= 480 && VIC_scr_x < 504) ||
				(VIC_scr_x >= 343 && VIC_scr_x < 380)) {
				VIC_scr_pos = SCREEN_POS::BORDER_LR;
			}
			else if (VIC_scr_x >= 24 && VIC_scr_x < 343) {
				VIC_scr_pos = SCREEN_POS::BORDER_TB;
			}
			else {
				VIC_scr_pos = SCREEN_POS::NO_RENDER;
			}
		}
		else if (VIC_scanline >= 51 && VIC_scanline < 55) {		//	Top, 38-row-area
			if ((VIC_scr_x >= 0 && VIC_scr_x < 24) ||
				(VIC_scr_x >= 480 && VIC_scr_x < 504) ||
				(VIC_scr_x >= 343 && VIC_scr_x < 380)) {
				VIC_scr_pos = SCREEN_POS::BORDER_LR;
			}
			else if (VIC_scr_x >= 24 && VIC_scr_x <= 343) {
				VIC_scr_pos = SCREEN_POS::ROW_38_AREA;
			}
			else {
				VIC_scr_pos = SCREEN_POS::NO_RENDER;
			}
		}
		else if (VIC_scanline >= 55 && VIC_scanline < 246) {	//	Screen
			if ((VIC_scr_x >= 0 && VIC_scr_x < 24) ||
				(VIC_scr_x >= 480 && VIC_scr_x < 504)) {
				VIC_scr_pos = SCREEN_POS::BORDER_LR;
			}
			else if (VIC_scr_x >= 24 && VIC_scr_x < 31) {
				VIC_scr_pos = SCREEN_POS::COL_38_AREA;
			}
			else if (VIC_scr_x >= 31 && VIC_scr_x <= 334) {
				VIC_scr_pos = SCREEN_POS::SCREEN;
			}
			else if (VIC_scr_x >= 334 && VIC_scr_x < 343) {
				VIC_scr_pos = SCREEN_POS::COL_38_AREA;
			}
			else if (VIC_scr_x >= 343 && VIC_scr_x < 380) {
				VIC_scr_pos = SCREEN_POS::BORDER_LR;
			}
			else {
				VIC_scr_pos = SCREEN_POS::NO_RENDER;
			}

		}
		else if (VIC_scanline >= 246 && VIC_scanline < 251) {	//	Bottom, 38-row-are
			if ((VIC_scr_x >= 0 && VIC_scr_x < 24) ||
				(VIC_scr_x >= 480 && VIC_scr_x < 504) ||
				(VIC_scr_x >= 343 && VIC_scr_x < 380)) {
				VIC_scr_pos = SCREEN_POS::BORDER_LR;
			}
			else if (VIC_scr_x >= 24 && VIC_scr_x <= 343) {
				VIC_scr_pos = SCREEN_POS::ROW_38_AREA;
			}
			else {
				VIC_scr_pos = SCREEN_POS::NO_RENDER;
			}
		}
		else if (VIC_scanline >= 251 && VIC_scanline < 299) {	//	Bottom Border
			if ((VIC_scr_x >= 0 && VIC_scr_x < 24) ||
				(VIC_scr_x >= 480 && VIC_scr_x < 504) ||
				(VIC_scr_x >= 343 && VIC_scr_x < 380)) {
				VIC_scr_pos = SCREEN_POS::BORDER_LR;
			}
			else if (VIC_scr_x >= 24 && VIC_scr_x < 343) {
				VIC_scr_pos = SCREEN_POS::BORDER_TB;
			}
			else {
				VIC_scr_pos = SCREEN_POS::NO_RENDER;
			}
		}
		else if (VIC_scanline >= 299 && VIC_scanline < 312) {	//	VBlank, bottom
			VIC_scr_pos = SCREEN_POS::NO_RENDER;
		}

		//	render pixel
		//	offset, according to which pixels need to be our 0/0 point in our actual window
		renderByPixels(VIC_scanline - 16, (VIC_scr_x + 24) % 504, VIC_scr_pos);

		//	increase X-Position & draw actual frame after it's beend rendered completely
		VIC_scr_x++;
		if (VIC_scr_x == 504) {
			VIC_scr_x = 0;
		}
		if (VIC_scr_x == 404 && VIC_scanline == 0) {
			drawFrame();
			//drawDebug();
		}
	}	
}

void VIC_fetchSpritePointer(uint8_t sprite_no) {

}

void VIC_dataRefresh() {
	//	NOP
}

void VIC_nextScanline() {
	VIC_scanline++;
	if (VIC_scanline == 312) {
		VIC_scanline = 0;
	}
}
















void writeVICregister(uint16_t adr, uint8_t val) {
	switch (adr)
	{
		case 0xd011:
			raster_irq_row &= 0b01111111;
			raster_irq_row |= (val & 0b10000000) << 1;
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
	if (adr == 0xd020) {
		//printf("[RasterIRQ set to %d] setting Bordercolor %x at scanline: %d at cycle %d\n", raster_irq_row, val, current_scanline, cy_0);
		//if (val == 0)
			//printf("\n");
	}
	VIC_REGISTERS[adr % 0xd000] = val;
}

uint8_t readVICregister(uint16_t adr) {
	switch (adr) {
		case 0xd011: {
			uint8_t ret = VIC_REGISTERS[adr % 0xd000];
			ret &= 0b01111111;
			ret |= (VIC_scanline >> 1) & 0b10000000;
			return ret;
			break;
		}
		case 0xd012:			//	Current Scanline
			//printf(" [Scanline: %d] ", current_scanline);
			return (uint8_t)(VIC_scanline & 0xff);
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