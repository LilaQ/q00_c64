#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "mmu.h"
#include "cpu.h"
#include "wmu.h"
#include "SDL2/include/SDL_syswm.h"
#include "SDL2/include/SDL.h"

SDL_Renderer* renderer;
SDL_Window* window;
SDL_Texture* texture;
unsigned char VRAM[402 * 284 * 3];	//	RGB - 320 * 200 pixel inside, with border 402 * 284
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

void initPPU(string filename) {

	//	init and create window and renderer
	SDL_Init(SDL_INIT_VIDEO);
	//SDL_SetHint(SDL_HINT_RENDER_VSYNC, 0);
	SDL_CreateWindowAndRenderer(402, 284, 0, &window, &renderer);
	SDL_SetWindowSize(window, 804, 568);
	//SDL_RenderSetLogicalSize(renderer, 512, 480);
	SDL_SetWindowResizable(window, SDL_TRUE);

	//	for fast rendering, create a texture
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 402, 284);
	initWindow(window, filename);
}

void drawFrame() {
	SDL_UpdateTexture(texture, NULL, VRAM, 402 * sizeof(unsigned char) * 3);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void renderFrame() {
	for (int i = 0; i < 25; i++) {
		for (int j = 0; j < 40; j++) {
			uint16_t offset = (i * 40) + j;
			uint8_t char_id = readFromMem(0x400 + offset);
			uint16_t chrrom = 0x1000;
			uint16_t colorram = 0xd800;

			//	draw character, from top to bottom
			for (int k = 0; k < 8; k++) {
				uint8_t row = readFromMem(chrrom + (char_id * 8) + k);
				uint8_t color = readFromMem(colorram + offset);
				uint8_t bg_color = readFromMem(0xd021);
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 123] = ((row & 0x80) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 1 + 123] = ((row & 0x80) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 2 + 123] = ((row & 0x80) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 3 + 123] = ((row & 0x40) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 4 + 123] = ((row & 0x40) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 5 + 123] = ((row & 0x40) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 6 + 123] = ((row & 0x20) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 7 + 123] = ((row & 0x20) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 8 + 123] = ((row & 0x20) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 9 + 123] = ((row & 0x10) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 10 + 123] = ((row & 0x10) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 11 + 123] = ((row & 0x10) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 12 + 123] = ((row & 0x08) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 13 + 123] = ((row & 0x08) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 14 + 123] = ((row & 0x08) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 15 + 123] = ((row & 0x04) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 16 + 123] = ((row & 0x04) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 17 + 123] = ((row & 0x04) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 18 + 123] = ((row & 0x02) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 19 + 123] = ((row & 0x02) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 20 + 123] = ((row & 0x02) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 21 + 123] = ((row & 0x01) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 22 + 123] = ((row & 0x01) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[(i * 9648) + ((k + 42) * 1206) + (j * 24) + 23 + 123] = ((row & 0x01) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
			}
		}
	}
}

void renderBorder() {
	uint16_t chrrom = 0x1000;
	uint16_t colorram = 0xd800;
	for (int i = 0; i < 402; i++) {
		for (int j = 0; j < 284; j++) {
			//	border
			uint16_t border_color = readFromMem(0xd020);
			if (i <= 41 || i >= 360 || j <= 41 || j >= 241) {
				VRAM[(j * 402 * 3) + (i * 3)] = COLORS[border_color][0];
				VRAM[(j * 402 * 3) + (i * 3) + 1] = COLORS[border_color][1];
				VRAM[(j * 402 * 3) + (i * 3) + 2] = COLORS[border_color][2];
			}
			//	frame
			else {
				uint16_t offset = ((j - 41) / 8) * 40 + (i - 41) / 8;
				uint8_t char_id = readFromMem(0x400 + offset);

				uint8_t row = readFromMem(chrrom + (char_id * 8) + ((j-41) % 8));
				uint8_t color = readFromMem(colorram + offset);

				uint8_t bg_color = readFromMem(0xd021);
				VRAM[(j * 402 * 3) + (i * 3)] = ((row & (1 << (7 - (i - 41) % 8))) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[(j * 402 * 3) + (i * 3) + 1] = ((row & (1 << (7 - (i - 41) % 8))) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[(j * 402 * 3) + (i * 3) + 2] = ((row & (1 << (7 - (i - 41) % 8))) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
			}
		}
	}
}

int cpu_cycles = 0;

void stepPPU(uint8_t cpu_cyc) {
	cpu_cycles += cpu_cyc;
	if (cpu_cycles >= 63) {
		cpu_cycles %= 63;
		uint8_t d012 = readFromMem(0xd012);
		d012++;
		writeToMem(0xd012, d012);
		if (!d012) {
			//renderFrame();
			renderBorder();
			drawFrame();
		}
	}
}
