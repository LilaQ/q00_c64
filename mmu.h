#pragma once
#include <stdint.h>
#include <string>

enum class MEMTYPE {
	IO,
	CHARROM,
	RAM,
	KERNAL,
	BASIC
};

using namespace::std;
void powerUpMMU();
void resetMMU();
void loadD64(string f);
void loadPRG(string f);
uint8_t readFromMem(uint16_t adr);
uint8_t readFromMemByVIC(uint16_t adr);
void writeToMemByVIC(uint16_t adr, uint8_t val);
void writeToMem(uint16_t adr, uint8_t val);
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
bool pageBoundaryCrossed();
void loadFirmware(string filename);
void loadCHRROM(string filename);
void dumpMem();
