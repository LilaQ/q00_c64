#pragma once
#include <stdint.h>
uint8_t readDataPortA();
uint8_t readDataPortB();
void writeDataPortA(uint8_t val);
void writeDataPortB(uint8_t val);
void setPortARW(uint8_t val);
void setPortBRW(uint8_t val);
void setKeyboardInput(uint8_t *KEYS);

uint8_t readCIA1timerALo();
uint8_t readCIA1timerAHi();
uint8_t readCIA1timerBLo();
uint8_t readCIA1timerBHi();
void setCIA1timerAlatchHi(uint8_t val);
void setCIA1timerAlatchLo(uint8_t val);
void setCIA1timerBlatchHi(uint8_t val);
void setCIA1timerBlatchLo(uint8_t val);
uint8_t readCIA1IRQStatus();
void setCIA1TimerAControl(uint8_t val);
void setCIA1TimerBControl(uint8_t val);
void setCIA1IRQcontrol(uint8_t val);

void tickAllTimers(uint8_t cycles);
