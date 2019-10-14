#define _CRT_SECURE_NO_DEPRECATE
#include <stdint.h>
#include <stdio.h>
#include "cpu.h"
#include "mmu.h"
#include "main.h"
#include "cia.h"

unsigned char memory[0x10000] = { 0xff };
bool pbc = false;

void powerUp() {
	//	TODO
}

void reset() {

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
		memory[0xa000 + i] = cartridge[i];
	}

	//	map KERNAL
	for (int i = 0; i < 0x2000; i++) {
		memory[0xe000 + i] = cartridge[0x2000 + i];
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
		memory[0x1000 + i] = cartridge[i];
	}
	//	map second character ROM
	for (int i = 0; i < 0x800; i++) {
		memory[0x1800 + i] = cartridge[i];
	}
}

uint8_t readFromMem(uint16_t adr) {
	switch (adr)
	{
		case 0xdc00:			//	read Keyboard / Joystick
			readDataPortA();
			break;
		case 0xdc01:			//	read Keyboard / Joystick
			readDataPortB();
			break;


		case 0xdc04:			//	read CIA1 TimerA Low
			readCIA1timerALo();
			break;
		case 0xdc05:			//	read CIA1 TimerA High
			readCIA1timerAHi();
			break;
		case 0xdc06:			//	read CIA1 TimerB Low
			readCIA1timerBLo();
			break;
		case 0xdc07:			//	read CIA1 TimerB High
			readCIA1timerBHi();
			break;

		case 0xdc0d:			//	read CIA1 IRQ Control and Status
			readCIA1IRQStatus();
			break;
		default:
			return memory[adr];
			break;
	}
}

void writeToMem(uint16_t adr, uint8_t val) {
	//	write-protect ROM adresses
	switch (adr)
	{
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
			if (adr < 0xa000 || (adr >= 0xc000 && adr < 0xe000))
				memory[adr] = val;
			break;
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
	return (memory[adr + 1] << 8) | memory[adr];
}
uint16_t getAbsoluteXIndex(uint16_t adr, uint8_t X) {
	a = ((memory[adr + 1] << 8) | memory[adr]);
	pbc = (a & 0xff00) != ((a + X) & 0xff00);
	return (a + X) % 0x10000;
}
uint16_t getAbsoluteYIndex(uint16_t adr, uint8_t Y) {
	a = ((memory[adr + 1] << 8) | memory[adr]);
	pbc = (a & 0xff00) != ((a + Y) & 0xff00);
	return (a + Y) % 0x10000;
}
uint16_t getZeropage(uint16_t adr) {
	return memory[adr];
}
uint16_t getZeropageXIndex(uint16_t adr, uint8_t X) {
	return (memory[adr] + X) % 0x100;
}
uint16_t getZeropageYIndex(uint16_t adr, uint8_t Y) {
	return (memory[adr] + Y) % 0x100;
}
uint16_t getIndirect(uint16_t adr) {
	return (memory[memory[adr + 1] << 8 | ((memory[adr] + 1) % 0x100)]) << 8 | memory[memory[adr + 1] << 8 | memory[adr]];
}
uint16_t getIndirectXIndex(uint16_t adr, uint8_t X) {
	a = (memory[(memory[adr] + X + 1) % 0x100] << 8) | memory[(memory[adr] + X) % 0x100];
	return a % 0x10000;
}
uint16_t getIndirectYIndex(uint16_t adr, uint8_t Y) {
	a = ((memory[(memory[adr] + 1) % 0x100] << 8) | (memory[memory[adr]]));
	pbc = (a & 0xff00) != ((a + Y) & 0xff00);
	return (a + Y) % 0x10000;
}