#define _CRT_SECURE_NO_DEPRECATE
#include <stdint.h>
#include <stdio.h>
#include "cpu.h"
#include "mmu.h"
#include "main.h"

unsigned char memory[0x10000];
bool pbc = false;

void powerUp() {
	//	TODO
}

void reset() {

}

uint8_t readFromMem(uint16_t adr) {
	return memory[adr];
}

void writeToMem(uint16_t adr, uint8_t val) {
	memory[adr] = val;
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