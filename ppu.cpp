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

void initPPU(string filename) {

	//	init and create window and renderer
	SDL_Init(SDL_INIT_VIDEO);
	//SDL_SetHint(SDL_HINT_RENDER_VSYNC, 0);
	SDL_CreateWindowAndRenderer(320, 200, 0, &window, &renderer);
	SDL_SetWindowSize(window, 640, 400);
	//SDL_RenderSetLogicalSize(renderer, 512, 480);
	SDL_SetWindowResizable(window, SDL_TRUE);

	//	for fast rendering, create a texture
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 320, 200);
	initWindow(window, filename);
}

void drawFrame() {
	SDL_UpdateTexture(texture, NULL, VRAM, 320 * sizeof(unsigned char) * 3);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void stepPPU() {
	drawFrame();
}
