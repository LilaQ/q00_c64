#define _CRT_SECURE_NO_DEPRECATE
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include "D64Parser.h"
#include "cpu.h"
#include "ppu.h"
#include "mmu.h"
#include "main.h"
#include "cia.h"

unsigned char memory[0x10000] = { 0xff };
unsigned char kernal[0x02000];		//	TODO 
unsigned char basic[0x02000];		//	TODO --- Only chr1 and chr2 are switched to external arrays yet
unsigned char chr1[0x800];
unsigned char chr2[0x800];
bool pbc = false;

// disk hotloading
string LOAD_FILE;
D64Parser parser;

void powerUp() {
	//	TODO
}

void reset() {

}

void loadD64(string f) {
	parser.init(f);
	parser.printAll();
	std::vector<uint8_t> r = parser.dirList();
	for (int i = 0; i < r.size(); i++) {
		memory[0x801 + i] = r.at(i);
	}
}

void loadPRG(string f) {
	uint8_t buf[1];
	std::vector<uint8_t> d;
	FILE* file = fopen(f.c_str(), "rb");
	while (fread(&buf[0], sizeof(uint8_t), 1, file)) {
		d.push_back(buf[0]);
	}
	fclose(file);
	printf("d size %x - %d\n", d.size(), d.size());
	for (uint16_t i = 2; i < d.size(); i++) {
		memory[0x7ff + i] = d.at(i);
	}
}

void loadPRGFromDisk(string f) {
	if (parser.filenameExists(f)) {
		std::vector<uint8_t> res = parser.getDataByFilename(f);
		for (int i = 2; i < res.size(); i++) {
			memory[0x7ff + i] = res.at(i);
		}
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
	//	SETNAM hook
	if (adr == 0xffbd) {
		LOAD_FILE = "";
		printf("SETTING NAME y: %x x: %x\n", getCPURegs().Y, getCPURegs().X);
		for (int i = 0; i < 16; i++) {
			LOAD_FILE += memory[((getCPURegs().Y << 8) | getCPURegs().X) + i];
		}
	}
	//	LOAD ,8,1 HOOK
	if (adr == 0xf52e || adr == 0xf4de) {
		if (parser.filenameExists(LOAD_FILE)) {
			clearCarry();		//	File exists
			cout << "File exists " << LOAD_FILE << "\n";
			loadPRGFromDisk(LOAD_FILE);
		}
		else {
			setCarry();			//	File does not exist
			cout << "File does not exist " << LOAD_FILE << "\n";
		}
		
	}
	//	Evaluate PLA (0x0001, decides which memory areas are visible at which adresses)
	uint8_t b0 = memory[0x0001] & 0b001;
	uint8_t b1 = memory[0x0001] & 0b010;
	uint8_t b2 = memory[0x0001] & 0b100;
	//	TODO Continue this


	switch (adr)
	{
		case 0xd012:			//	Current Scanline
			return getCurrentScanline();
			break;
		case 0xd019:			//	IRQ flags, (active IRQs)
			return getIRQStatus();
			break;
		case 0xd01a:			//	IRQ Mask (what is allowed to cause IRQs)
			return getIRQMask();
			break;
		
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

uint8_t readChar(uint16_t adr) {
	if (adr >= 0x1000 && adr < 0x1800)
		return chr1[adr % 0x1000];
	else if (adr >= 0x1800 && adr < 0x2000)
		return chr2[adr % 0x1800];
	return 0x00;
}

void writeToMem(uint16_t adr, uint8_t val) {
	//	write-protect ROM adresses
	switch (adr)
	{
		case 0xd011:			//	Set Rasterzeilen IRQ
			setRasterIRQhi(val);
			memory[adr] = val;
			break;
		case 0xd012:			//	Set Rasterzeilen IRQ
			setRasterIRQlow(val);
			break;
		case 0xd019:			//	Clear IRQ flags, that are no longer needed
			clearIRQStatus(val);
			break;
		case 0xd01a:			//	IRQ Mask (what is allowed to cause IRQs)
			setIRQMask(val);
			break;

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