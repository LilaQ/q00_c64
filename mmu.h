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
void loadFirmware(string filename);
void loadCHRROM(string filename);
void dumpMem();
uint8_t getByte(uint16_t adr);
void writeByte(uint16_t adr, uint8_t val);
