#define _CRT_SECURE_NO_DEPRECATE
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include "D64Parser.h"
#include "cpu.h"
#include "mmu.h"
#include "main.h"
#include "cia.h"

unsigned char memory[0x10000] = { 0xff };
unsigned char kernal[0x02000];		//	TODO 
unsigned char basic[0x02000];		//	TODO --- Only chr1 and chr2 are switched to external arrays yet
unsigned char chr1[0x800];
unsigned char chr2[0x800];
bool pbc = false;

void powerUp() {
	//	TODO
}

void reset() {

}

void loadD64(string f) {
	D64Parser parser;
	parser.init(f);
	parser.printAll();
	std::vector<uint8_t> r = parser.getData(0);
	for (int i = 2; i < r.size(); i++) {
		memory[0x7ff + i] = r.at(i);
	}
	printf("done\n");
}

void loadPRG(string f) {
	std::vector<uint8_t> d(0xc200);
	FILE* file = fopen(f.c_str(), "rb");
	int pos = 0;
	while (fread(&d[pos], 1, 1, file)) {
		pos++;
	}
	fclose(file);
	for (int i = 2; i < d.size(); i++) {
		memory[0x7ff + i] = d.at(i);
	}
}

void loadFirmware(string filename) {

	//	load fw
	unsigned char cartridge[0x4000];
	FILE* file = fopen(filename.c_str(), "rb");
	int pos = 0;
	while (fread(&cartridge[pos], 1, 1, file)) {
		pos++;
	}
	fclose(file);

	//	map BASIC interpreter (first 8kb) to 0xa000 - 0xbfff
	for (int i = 0; i < 0x2000; i++) {
		basic[i] = cartridge[i];
	}

	//	map KERNAL
	for (int i = 0; i < 0x2000; i++) {
		kernal[i] = cartridge[0x2000 + i];
	}
}

void loadCHRROM(string filename) {
	
	//	load file
	unsigned char cartridge[0x4000];
	FILE* file = fopen(filename.c_str(), "rb");
	int pos = 0;
	while (fread(&cartridge[pos], 1, 1, file)) {
		pos++;
	}
	fclose(file);

	//	map first character ROM
	for (int i = 0; i < 0x800; i++) {
		chr1[i] = cartridge[i];
	}
	//	map second character ROM
	for (int i = 0; i < 0x800; i++) {
		chr2[i] = cartridge[i + 0x800];
	}
}

uint8_t readFromMem(uint16_t adr) {
	switch (adr)
	{
		case 0xdc00:			//	read Keyboard / Joystick
			return readDataPortA();
			break;
		case 0xdc01:			//	read Keyboard / Joystick
			return readDataPortB();
			break;


		case 0xdc04:			//	read CIA1 TimerA Low
			return readCIA1timerALo();
			break;
		case 0xdc05:			//	read CIA1 TimerA High
			return readCIA1timerAHi();
			break;
		case 0xdc06:			//	read CIA1 TimerB Low
			return readCIA1timerBLo();
			break;
		case 0xdc07:			//	read CIA1 TimerB High
			return readCIA1timerBHi();
			break;

		case 0xdc0d:			//	read CIA1 IRQ Control and Status
			return readCIA1IRQStatus();
			break;
		default:
			if (adr >= 0x1000 && adr < 0x1800) {		//	CHR1 
				return chr1[adr % 0x1000];
			}
			if (adr >= 0x1800 && adr < 0x2000) {		//	CHR2 
				return chr2[adr % 0x1800];
			}
			if (adr >= 0xa000 && adr < 0xc000) {		//	BASIC
				return basic[adr % 0xa000];
			}
			if (adr >= 0xe000 && adr < 0x10000) {		//	KERNAL
				return kernal[adr % 0xe000];
			}
			return memory[adr];
			break;
	}
}

void writeToMem(uint16_t adr, uint8_t val) {
	//	write-protect ROM adresses
	switch (adr)
	{
		case 0xdc00:			//	write Keyboard / Joystick
			writeDataPortA(val);
			break;
		case 0xdc01:			//	write Keyboard / Joystick
			writeDataPortB(val);
			break;
		case 0xdc02:			//	Port A RW
			setPortARW(val);
			break;
		case 0xdc03:			//	Port B RW
			setPortBRW(val);
			break;

		case 0xdc04:
			setCIA1timerAlatchLo(val);
			break;
		case 0xdc05:
			setCIA1timerAlatchHi(val);
			break;
		case 0xdc06:
			setCIA1timerBlatchLo(val);
			break;
		case 0xdc07:
			setCIA1timerBlatchHi(val);
			break;

		case 0xdc0d:
			setCIA1IRQcontrol(val);
			break;
		case 0xdc0e:
			setCIA1TimerAControl(val);
			break;

		default:
			if (adr >= 0x1000 && adr < 0x1800) {		//	CHR1 
				chr1[adr % 0x1000] = val;
				break;
			}
			if (adr >= 0x1800 && adr < 0x2000) {		//	CHR2 
				chr2[adr % 0x1800] = val;
				break;
			}
			memory[adr] = val;
			break;
	}
	if (adr == 0xa565) {
		printf("Someone's writing! %x\n", val);
	}
}

bool pageBoundaryCrossed() {
	return pbc;
}

//	addressing modes
uint16_t a;
uint16_t getImmediate(uint16_t adr) {
	return adr;
}
uint16_t getAbsolute(uint16_t adr) {
	return (readFromMem(adr + 1) << 8) | readFromMem(adr);
}
uint16_t getAbsoluteXIndex(uint16_t adr, uint8_t X) {
	a = ((readFromMem(adr + 1) << 8) | readFromMem(adr));
	pbc = (a & 0xff00) != ((a + X) & 0xff00);
	return (a + X) % 0x10000;
}
uint16_t getAbsoluteYIndex(uint16_t adr, uint8_t Y) {
	a = ((readFromMem(adr + 1) << 8) | readFromMem(adr));
	pbc = (a & 0xff00) != ((a + Y) & 0xff00);
	return (a + Y) % 0x10000;
}
uint16_t getZeropage(uint16_t adr) {
	return readFromMem(adr);
}
uint16_t getZeropageXIndex(uint16_t adr, uint8_t X) {
	return (readFromMem(adr) + X) % 0x100;
}
uint16_t getZeropageYIndex(uint16_t adr, uint8_t Y) {
	return (readFromMem(adr) + Y) % 0x100;
}
uint16_t getIndirect(uint16_t adr) {
	return (readFromMem(readFromMem(adr + 1) << 8 | ((readFromMem(adr) + 1) % 0x100))) << 8 | readFromMem(readFromMem(adr + 1) << 8 | readFromMem(adr));
}
uint16_t getIndirectXIndex(uint16_t adr, uint8_t X) {
	a = (readFromMem((readFromMem(adr) + X + 1) % 0x100) << 8) | readFromMem((readFromMem(adr) + X) % 0x100);
	return a % 0x10000;
}
uint16_t getIndirectYIndex(uint16_t adr, uint8_t Y) {
	a = ((readFromMem((readFromMem(adr) + 1) % 0x100) << 8) | (readFromMem(readFromMem(adr))));
	pbc = (a & 0xff00) != ((a + Y) & 0xff00);
	return (a + Y) % 0x10000;
}