#pragma once
void togglePause();
void setPause();
void resetPause();

void BUS_haltCPU();
bool BUS_takeoverActive();

//	debug
uint8_t currentCycle();