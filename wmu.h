#pragma once
#include <string>
#include "SDL2/include/SDL.h"
using namespace::std;
void initWindow(SDL_Window* win, string filename);
void handleWindowEvents(SDL_Event event);
void setTitle(string filename);
void updateTitle(string filename, uint16_t fps = 0);
void printMsg(string mType, string mLevel, string msg, bool lineBreak);
void printMsg(string mType, string mLevel, string msg);
void WMU_setFPS(uint16_t fps);
