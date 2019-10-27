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

void drawFrame() {
	SDL_UpdateTexture(texture, NULL, VRAM, 402 * sizeof(unsigned char) * 3);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void initPPU(string filename) {

	//	init and create window and renderer
	SDL_Init(SDL_INIT_VIDEO);
	//SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_CreateWindowAndRenderer(402, 284, 0, &window, &renderer);
	SDL_SetWindowSize(window, 804, 588);
	//SDL_RenderSetLogicalSize(renderer, 512, 480);
	SDL_SetWindowResizable(window, SDL_TRUE);

	//	for fast rendering, create a texture
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 402, 284);
	initWindow(window, filename);

	drawFrame();
}

void renderLine(uint16_t j) {
	uint16_t chrrom = 1024 * (readFromMem(0xd018) & 0b1110);
	uint8_t offset_x = readFromMem(0xd016) & 0b111;
	uint16_t colorram = 0xd800;
	for (int i = 0; i < 402; i++) {
		uint16_t border_color = readFromMem(0xd020);
		if (i <= 39 || i >= 360 || j <= 39 || j >= 240) {
			VRAM[(j * 402 * 3) + (i * 3)] = COLORS[border_color][0];
			VRAM[(j * 402 * 3) + (i * 3) + 1] = COLORS[border_color][1];
			VRAM[(j * 402 * 3) + (i * 3) + 2] = COLORS[border_color][2];
		}
		//	frame
		else {
			uint16_t offset = ((j - 40) / 8) * 40 + (i - 40) / 8;
			uint8_t char_id = readFromMem(0x400 + offset);

			uint8_t row = readChar(chrrom + (char_id * 8) + ((j-40) % 8));
			uint8_t color = readFromMem(colorram + offset);

			uint8_t bg_color = readFromMem(0xd021);
			VRAM[(j * 402 * 3) + ((i + offset_x) * 3)] = ((row & (1 << (7 - (i - 40) % 8))) > 0) ? COLORS[color][0] : COLORS[bg_color][0];
			VRAM[(j * 402 * 3) + ((i + offset_x) * 3) + 1] = ((row & (1 << (7 - (i - 40) % 8))) > 0) ? COLORS[color][1] : COLORS[bg_color][1];
			VRAM[(j * 402 * 3) + ((i + offset_x) * 3) + 2] = ((row & (1 << (7 - (i - 40) % 8))) > 0) ? COLORS[color][2] : COLORS[bg_color][2];
		}
	}
}

int cpu_cycles = 0;
int render_row = 0;
void stepPPU(uint8_t cpu_cyc) {
	cpu_cycles += cpu_cyc;
	if (cpu_cycles >= 63) {
		cpu_cycles %= 63;
		renderLine(render_row);
		render_row++;
		writeToMem(0xd012, render_row & 0xff);
		if (render_row >= 284) {
			drawFrame();
			render_row = 0;
		}
	}
}
