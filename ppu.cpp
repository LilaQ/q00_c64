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
#include "bus.h"
#include "SDL2/include/SDL.h"

uint16_t cycles_on_current_scanline = 0;
uint16_t current_scanline = 0;
uint16_t raster_irq_row = 0;
bool DRAW_TOP_BOTTOM_BORDER = true;
int32_t ADR = 0;
vector<SPRITE> SPRITES_VEC(8);
array<uint8_t, 0x31> VIC_REGISTERS;
uint16_t VIC_scanline = 0;
SCREEN_POS VIC_scr_pos = SCREEN_POS::NO_RENDER;

IRQ_STATUS irq_status;
IRQ_MASK irq_mask;
SDL_Renderer* renderer;
SDL_Window* window;
SDL_Texture* texture;

vector<uint8_t> VRAM(504 * 284 * 3);	//	RGB - 320 * 200 pixel inside, with border 402 * 284
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
	SDL_UpdateTexture(texture, NULL, (uint8_t*)VRAM.data(), 504 * sizeof(unsigned char) * 3);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void initPPU(string filename) {

	//	init and create window and renderer
	SDL_Init(SDL_INIT_VIDEO);
	//SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_CreateWindowAndRenderer(524, 284, 0, &window, &renderer);
	SDL_SetWindowSize(window, 1028, 588);
	//SDL_RenderSetLogicalSize(renderer, 512, 480);
	SDL_SetWindowResizable(window, SDL_TRUE);

	//	for fast rendering, create a texture
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 504, 284);
	initWindow(window, filename);

	VIC_REGISTERS.fill(0x00);
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

inline void renderSprites(uint16_t pixel_on_scanline, uint16_t scanline, uint32_t ADR, uint8_t i) {

	//	adjust x and y to accomodate to the sprite-raster that sprites can be drawn to
	const uint8_t sprite_fix = 0;
	const int16_t x = pixel_on_scanline - 112;
	const int16_t y = scanline + 16;
	const uint16_t screen_start = 0x40 * (VIC_REGISTERS[0x18] & 0b11110000);

	const SPRITE S = SPRITES_VEC[i];

	//	3 byte per line, 21 lines
	uint8_t spr_x = x - S.pos_x;
	uint8_t width_factor = 1 + S.width_doubled;

	uint8_t color_byte	= S.data[(spr_x / width_factor) / 8];
	uint8_t color_bit_h = color_byte & (1 << (7 - ((spr_x / width_factor) % 8)));
	uint8_t color_bit_l = 0x00;
	uint8_t color_bits = 0x00;
	if (S.multicolor) {
		//	looking at high bit
		if ((spr_x / width_factor) % 2 == 0) {
			color_bit_h = color_byte & (1 << (7 - ((spr_x / width_factor) % 8)));
			color_bit_l = color_byte & (1 << (7 - (((spr_x / width_factor) + 1) % 8)));
		}
		else if ((spr_x / width_factor) % 2 == 1) {
			color_bit_l = color_byte & (1 << (7 - ((spr_x / width_factor) % 8))) ;
			color_bit_h = color_byte & (1 << (7 - (((spr_x / width_factor) - 1) % 8)));
		}
	}
	color_bit_h = color_bit_h > 0;
	color_bit_l = color_bit_l > 0;
	color_bits = (color_bit_h << 1) | color_bit_l;
	uint8_t color = 0x00;
	//	single color (hires)
	if (!S.multicolor) {
		if (color_bit_h) {
			color = VIC_REGISTERS[0x27 + i];
			VRAM[ADR]		= COLORS[color][0];
			VRAM[ADR + 1]	= COLORS[color][1];
			VRAM[ADR + 2]	= COLORS[color][2];
		}
	}
	//	multicolor
	else {
		switch (color_bits)
		{
		case 0b00:
			return;
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
		default:
			break;
		}
		VRAM[ADR]		= COLORS[color][0];
		VRAM[ADR + 1]	= COLORS[color][1];
		VRAM[ADR + 2]	= COLORS[color][2];
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
	if (SCREENPOS != SCREEN_POS::NO_RENDER) {
		uint8_t color_choice = 0x00;

		//	Basic VRAM start address and OFFSET of the pixel we want to write to in this iteration
		ADR = (scanline * 504 * 3) + (x * 3);
		uint16_t OFFSET = (uint16_t)(((scanline - 35) / 8) * 40 + (x - offset_x - 24 - 112) / 8);

		//	TEXTMODE
		if (((SCREENPOS == SCREEN_POS::SCREEN) ||
			(SCREENPOS == SCREEN_POS::ROW_25_AREA) ||
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
				color_choice = ((chr & (1 << (7 - (x - offset_x - 112) % 8))) > 0) ? 1 : 0;
				VRAM[ADR]		= (color_choice) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[ADR + 1]	= (color_choice) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[ADR + 2]	= (color_choice) ? COLORS[color][2] : COLORS[bg_color][2];
			}

			//	MULTICOLOR MODE
			else {
				/*
					%00 = $d021 (Hintergrundfarbe)
					%01 = $d022
					%10 = $d023
					%11 = Farbe laut Farb-RAM (ab $d800)
				*/
				uint8_t index = (7 - ((x - offset_x) % 8));
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
				color_choice = (high_bit << 1) | low_bit;
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
					VRAM[ADR]		= ((chr & (1 << (7 - (x - offset_x - 112) % 8))) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
					VRAM[ADR + 1]	= ((chr & (1 << (7 - (x - offset_x - 112) % 8))) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
					VRAM[ADR + 2]	= ((chr & (1 << (7 - (x - offset_x - 112) % 8))) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
				}
			}
		}
		//	BITMAP MODE
		else if ((SCREENPOS == SCREEN_POS::SCREEN) || (SCREENPOS == SCREEN_POS::COL_38_AREA) ||	(SCREENPOS == SCREEN_POS::ROW_25_AREA)) {
			uint16_t BMP_OFFSET = (OFFSET / 40) * 320 + (OFFSET % 40) * 8 + ((scanline - 35) % 8);
			color_choice = readFromMemByVIC(bmp_start_address + BMP_OFFSET) & (0b10000000 >> ((x - 112) % 8));
			uint8_t color_index = (color_choice) ? (readFromMemByVIC(bmp_color_address + OFFSET) & 0b11110000) >> 4 : readFromMemByVIC(bmp_color_address + OFFSET) & 0b1111;

			//	MULTICOLOR MODE
			if ((VIC_REGISTERS[0x16] & 0b10000)) {
				uint8_t index = (7 - ((x - 112) % 8));
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
				color_choice = (high_bit << 1) | low_bit;
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

		//	border
		uint8_t border_color = VIC_REGISTERS[0x20] & 0xf;
		if ((SCREENPOS == SCREEN_POS::BORDER_LR ) || ((SCREENPOS == SCREEN_POS::SCREEN) && ((VIC_REGISTERS[0x11] & 0b10000) == 0))) {
			VRAM[(scanline * 504 * 3) + (x * 3)] = COLORS[border_color][0];
			VRAM[(scanline * 504 * 3) + (x * 3) + 1] = COLORS[border_color][1];
			VRAM[(scanline * 504 * 3) + (x * 3) + 2] = COLORS[border_color][2];
		}
		//	Top / Bottom border - account border opening
		if (SCREENPOS == SCREEN_POS::BORDER_TB) {
			uint8_t border_color	= VIC_REGISTERS[0x20] & 0xf;
			uint8_t bg_color		= VIC_REGISTERS[0x21] & 0xf;
			uint8_t col				= border_color;
			if (!DRAW_TOP_BOTTOM_BORDER) {
				col	= (readFromMem(0x3fff)& (1 << (x % 8))) ? border_color : bg_color;
			}
			VRAM[(scanline * 504 * 3) + (x * 3)]		= COLORS[col][0];
			VRAM[(scanline * 504 * 3) + (x * 3) + 1]	= COLORS[col][1];
			VRAM[(scanline * 504 * 3) + (x * 3) + 2]	= COLORS[col][2];
		}

		//	TODO : We need a proper timing in which Sprites / BG are being drawn (especially when borders are disabled)
		//	sprites
		for (int8_t i = 7; i >= 0; i--) {
			//	draw Sprite behind Foreground? No? Then continue drawing the sprite, else skip
			SPRITE S = SPRITES_VEC[i];
			if (VIC_isSpriteEnabled(i) && S.isDrawing() && (S.pos_x <= (x - 112) && (S.pos_x + S.width) > (x - 112))) {
				if (S.prio_background) {
					//	is this pixel Foreground? Yes? Then draw the Sprite over it, if not, skip
					if (
						(((VIC_REGISTERS[0x16] & 0b10000) > 0) && ((color_choice == 0b11) || (color_choice == 0b10))) ||						//	Multicolor mode - 0b10 and 0b11 are foreground
						(((VIC_REGISTERS[0x16] & 0b10000) == 0) && (color_choice != 0b1))														//	Normal color mode - 0b1 is foreground
						)
					{
						renderSprites(x, scanline, ADR, i);
					}
				}
				else {
					renderSprites(x, scanline, ADR, i);
				}
				//	check if this is the last data that had to be drawn, to disable drawing of the sprite and make it availabel to be reset on Y-axis again
				if (((scanline + 16 + 1) >= (S.pos_y + S.height)) && ((x - 112 + 1) >= (S.pos_x + S.width))) {
					SPRITES_VEC[i].setDrawing(false);
				}
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

/*	REWRITE FROM HERE ON		*/

//	DEBUG
uint16_t currentScanline() {
	return VIC_scanline;
}
uint16_t currentPixel() {
	return (BUS_currentCycle()) * 8;
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
	if (VIC_scanline == raster_irq_row && irq_mask.irq_can_be_cause_by_rasterline == true) {
		irq_status.setFlags(0b10000001);		//	set "IRQ FROM VIC", and as reason set "IRQ FROM RASTERLINE"
		return true;
	}
	return false;
}

void VIC_fetchGraphicsData(uint8_t amount) {

	while (amount--)
	{
		for (uint8_t i = 0; i < 8; i++) {
			uint16_t x_pos = (currentPixel() + i);
			if (VIC_scanline >= 0 && VIC_scanline < 16) {			//	VBlank Top
				VIC_scr_pos = SCREEN_POS::NO_RENDER;
			}
			else if (VIC_scanline >= 16 && VIC_scanline < 51) {		//	Top Border
				if ((x_pos >= 88 && x_pos < 136) ||
					(x_pos >= 456 && x_pos < 491)) {
					VIC_scr_pos = SCREEN_POS::BORDER_LR;
				}
				else if (x_pos >= 136 && x_pos < 456) {
					VIC_scr_pos = SCREEN_POS::BORDER_TB;
				}
				else {
					//VIC_scr_pos = SCREEN_POS::NO_RENDER;
					VIC_scr_pos = SCREEN_POS::BORDER_LR;
				}
			}
			else if (VIC_scanline >= 51 && VIC_scanline < 55) {		//	Top, 38-row-area
				if ((x_pos >= 88 && x_pos < 136) ||
					(x_pos >= 456 && x_pos < 491)) {
					VIC_scr_pos = SCREEN_POS::BORDER_LR;
				}
				else if (x_pos >= 136 && x_pos < 456) {
					VIC_scr_pos = SCREEN_POS::ROW_25_AREA;
				}
				else {
					//VIC_scr_pos = SCREEN_POS::NO_RENDER;
					VIC_scr_pos = SCREEN_POS::BORDER_LR;
				}
			}
			else if (VIC_scanline >= 55 && VIC_scanline < 246) {	//	Screen
				if ((x_pos >= 88	&& x_pos < 136) ||
					(x_pos >= 456	&& x_pos < 491)) {
					VIC_scr_pos = SCREEN_POS::BORDER_LR;
				}
				else if (x_pos >= 136 && x_pos < 143) {
					VIC_scr_pos = SCREEN_POS::COL_38_AREA;
				}
				else if (x_pos >= 143 && x_pos < 447) {
					VIC_scr_pos = SCREEN_POS::SCREEN;
				}
				else if (x_pos >= 447 && x_pos < 456) {
					VIC_scr_pos = SCREEN_POS::COL_38_AREA;
				}
				else {
					//VIC_scr_pos = SCREEN_POS::NO_RENDER;
					VIC_scr_pos = SCREEN_POS::BORDER_LR;
				}

			}
			else if (VIC_scanline >= 246 && VIC_scanline < 251) {	//	Bottom, 38-row-are
				if ((x_pos >= 88 && x_pos < 136) ||
					(x_pos >= 456 && x_pos < 491)) {
					VIC_scr_pos = SCREEN_POS::BORDER_LR;
				}
				else if (x_pos >= 136 && x_pos < 456) {
					VIC_scr_pos = SCREEN_POS::ROW_25_AREA;
				}
				else {
					//VIC_scr_pos = SCREEN_POS::NO_RENDER;
					VIC_scr_pos = SCREEN_POS::BORDER_LR;
				}
			}
			else if (VIC_scanline >= 251 && VIC_scanline < 299) {	//	Bottom Border
				if ((x_pos >= 88 && x_pos < 136) ||
					(x_pos >= 456 && x_pos < 491)) {
					VIC_scr_pos = SCREEN_POS::BORDER_LR;
				}
				else if (x_pos >= 136 && x_pos < 456) {
					VIC_scr_pos = SCREEN_POS::BORDER_TB;
				}
				else {
					//VIC_scr_pos = SCREEN_POS::NO_RENDER;
					VIC_scr_pos = SCREEN_POS::BORDER_LR;
				}
			}
			else if (VIC_scanline >= 299 && VIC_scanline < 312) {	//	VBlank, bottom
				VIC_scr_pos = SCREEN_POS::NO_RENDER;
			}

			//	open border Top/Bottom hack
			/*
				If 25 lines open, in line 25 back to 24, end of 25 check is set to false
			*/
			//	reset border-drawing
			if (VIC_scanline == 50 && x_pos == 503) {
				DRAW_TOP_BOTTOM_BORDER = false;
			}
			//	24 Lines, check border-drawing
			if (VIC_scanline == 241 && x_pos == 503) {
				if ((VIC_REGISTERS[0x11] & 0b1000) == 0)
					DRAW_TOP_BOTTOM_BORDER = true;
			}
			//	25 Lines, check border-drawing
			if (VIC_scanline == 249 && x_pos == 503) {
				if (VIC_REGISTERS[0x11] & 0b1000)
					DRAW_TOP_BOTTOM_BORDER = true;
			}

			//	render pixel
			renderByPixels(VIC_scanline - 16, x_pos, VIC_scr_pos);
			if (x_pos == 503 && VIC_scanline == 0) {
				drawFrame();
				//drawDebug();
			}
		}
	}
}

void VIC_fetchSpriteAttributes(uint8_t sprite_no) {
	SPRITES_VEC.at(sprite_no).reinit(sprite_no, VIC_REGISTERS, VIC_scanline);
}

void VIC_fetchSpritePointer(uint8_t sprite_no) {
	SPRITES_VEC.at(sprite_no).fetchSpritePointer(sprite_no, VIC_REGISTERS);
}

bool VIC_isSpriteInLine(uint8_t sprite_no, uint16_t y) {
	//	check if this sprite is part of the current line
	return (y >= (SPRITES_VEC[sprite_no].pos_y) && y < (SPRITES_VEC[sprite_no].pos_y + SPRITES_VEC[sprite_no].height));
}

bool VIC_isSpriteInCurrentLine(uint8_t sprite_no) {
	return VIC_isSpriteInLine(sprite_no, VIC_scanline);
}

bool VIC_isSpriteInNextLine(uint8_t sprite_no) {
	return VIC_isSpriteInLine(sprite_no, VIC_scanline + 1);
}

bool VIC_isSpriteInPrevLine(uint8_t sprite_no) {
	return VIC_isSpriteInLine(sprite_no, VIC_scanline - 1);
}

void VIC_fetchSpriteDataBytes(uint8_t sprite_no) {
	uint8_t sprite_fix = (sprite_no >= 0 && sprite_no <= 2) ? 1 : 0;
	if (VIC_isSpriteInLine(sprite_no, VIC_scanline + sprite_fix)) {
		SPRITES_VEC[sprite_no].fetchSpriteDataBytes(sprite_no, VIC_scanline + sprite_fix);
	}
}

void VIC_enableDrawingOfSprite(uint8_t id) {
	SPRITES_VEC[id].setDrawing(true);
}

bool VIC_isSpriteEnabled(uint8_t sprite_no) {
	//	check if sprite is enabled first
	return (VIC_REGISTERS[0x15] & (1 << sprite_no));
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