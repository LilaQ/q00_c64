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
unsigned char VRAM[320 * 200 * 3];	//	RGB - 320 * 200 pixel
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
	SDL_CreateWindowAndRenderer(320, 200, 0, &window, &renderer);
	SDL_SetWindowSize(window, 640, 400);
	//SDL_RenderSetLogicalSize(renderer, 512, 480);
	SDL_SetWindowResizable(window, SDL_TRUE);

	//	for fast rendering, create a texture
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 320, 200);
	initWindow(window, filename);
}

void drawFrame() {
	SDL_UpdateTexture(texture, NULL, VRAM, 320 * sizeof(unsigned char) * 3);
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
				VRAM[(i * 7680) + (k * 960) + (j * 24)] = ((row & 0x80) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 1] = ((row & 0x80) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 2] = ((row & 0x80) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 3] = ((row & 0x40) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 4] = ((row & 0x40) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 5] = ((row & 0x40) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 6] = ((row & 0x20) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 7] = ((row & 0x20) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 8] = ((row & 0x20) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 9] = ((row & 0x10) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 10] = ((row & 0x10) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 11] = ((row & 0x10) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 12] = ((row & 0x08) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 13] = ((row & 0x08) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 14] = ((row & 0x08) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 15] = ((row & 0x04) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 16] = ((row & 0x04) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 17] = ((row & 0x04) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 18] = ((row & 0x02) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 19] = ((row & 0x02) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 20] = ((row & 0x02) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 21] = ((row & 0x01) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 22] = ((row & 0x01) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
				VRAM[(i * 7680) + (k * 960) + (j * 24) + 23] = ((row & 0x01) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
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
		printf("d012: %x\n", readFromMem(0xd012));
		if (d012 == 0) {
			renderFrame();
			drawFrame();
		}
	}
}
