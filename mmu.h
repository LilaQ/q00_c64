#pragma once
#include <stdint.h>
#include <string>
using namespace::std;
void powerUp();
void reset();
unsigned char readFromMem(uint16_t adr);
uint16_t getImmediate(uint16_t adr);
uint16_t getZeropage(uint16_t adr);
uint16_t getZeropageXIndex(uint16_t adr, uint8_t X);
uint16_t getZeropageYIndex(uint16_t adr, uint8_t Y);
uint16_t getIndirect(uint16_t adr);
uint16_t getIndirectXIndex(uint16_t adr, uint8_t X);
uint16_t getIndirectYIndex(uint16_t adr, uint8_t Y);
uint16_t getAbsolute(uint16_t adr);
uint16_t getAbsoluteXIndex(uint16_t adr, uint8_t X);
uint16_t getAbsoluteYIndex(uint16_t adr, uint8_t Y);
void writeToMem(uint16_t adr, uint8_t val);
bool pageBoundaryCrossed();
void loadFirmware(string filename);
void loadCHRROM(string filename);
