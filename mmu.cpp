#pragma once
#define _CRT_SECURE_NO_DEPRECATE
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include "D64Parser.h"
#include "wmu.h"
#include "cpu.h"
#include "ppu.h"
#include "mmu.h"
#include "main.h"
#include "cia.h"
#include <fstream>


unsigned char memory[0x10000] = { 0x00 };
unsigned char kernal[0x02000] = { 0x00 };
unsigned char basic[0x02000] = { 0x00 };
unsigned char chr[0x1000] = { 0x00 };
unsigned char colorram[0x400] = { 0x00 };
MEMTYPE memtype_a000_bfff = MEMTYPE::BASIC;
MEMTYPE memtype_d000_dfff = MEMTYPE::IO;
MEMTYPE memtype_e000_ffff = MEMTYPE::KERNAL;
bool pbc = false;

// disk hotloading
string LOAD_FILE = "";
D64Parser parser;

void powerUpMMU() {
	resetMMU();
}

void resetMMU() {
	unsigned char r = 0x00;
	for (int i = 0; i < sizeof(memory); i++)
		memory[i] = r;
	for (int i = 0; i < sizeof(kernal); i++)
		kernal[i] = r;
	for (int i = 0; i < sizeof(basic); i++)
		basic[i] = r;
	for (int i = 0; i < sizeof(chr); i++)
		chr[i] = r;
	for (int i = 0; i < sizeof(colorram); i++)
		colorram[i] = r;

	//	load firmware (BASIC and KERNAL)
	loadFirmware("./fw.bin.ptc");

	//	load char ROM
	loadCHRROM("./char.bin");

	memory[0x0001] = 0x37;		//	Zeropage for PLA
}

//	TODO CLEAR MEMORY, FROM PRIOR SOFTWARE, BEFORE LOADING ANY PRG

void loadD64(string f) {
	parser.init(f);
	parser.printAll();
	std::vector<uint8_t> r = parser.dirList();
	for (uint32_t i = 0; i < r.size(); i++) {
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
		memory[((d.at(1) << 8) | d.at(0)) - 2 + i] = d.at(i);
	}
	setTitle(f);
}

void loadPRGFromDisk(string f) {
	if (parser.filenameExists(f)) {
		std::vector<uint8_t> res = parser.getDataByFilename(f);
		for (uint32_t i = 2; i < res.size(); i++) {
			memory[((res.at(1) << 8) | res.at(0)) - 2 + i] = res.at(i);
		}
	}
	setTitle(f);
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

	//	map character ROM
	for (int i = 0; i < 0x1000; i++) {
		chr[i] = cartridge[i];
	}
}

void refreshMemoryMapping() {
	//	Zeropage; this can configure the PLA, and thus the memory mapping, and the ROMs that are enabled
	uint8_t LORAM = (memory[0x0001] & 0b1);
	uint8_t HIRAM = ((memory[0x0001] & 0b10) >> 1);
	uint8_t CHAREN = ((memory[0x0001] & 0b100) >> 2);
	/*if (CHAREN)
		memtype_d000_dfff = MEMTYPE::IO;
	else {
		if (HIRAM)
			memtype_d000_dfff = MEMTYPE::CHARROM;
		else
			memtype_d000_dfff = MEMTYPE::RAM;
	}
	if (HIRAM)
		memtype_e000_ffff = MEMTYPE::KERNAL;
	else
		memtype_e000_ffff = MEMTYPE::RAM;
	if (HIRAM && LORAM)
		memtype_a000_bfff = MEMTYPE::BASIC;
	else
		memtype_a000_bfff = MEMTYPE::RAM;*/
	if (!LORAM && !HIRAM) {
		memtype_a000_bfff = MEMTYPE::RAM;
		memtype_d000_dfff = MEMTYPE::RAM;
		memtype_e000_ffff = MEMTYPE::RAM;
	}
	else if (LORAM && !HIRAM) {
		memtype_a000_bfff = MEMTYPE::RAM;
		memtype_e000_ffff = MEMTYPE::RAM;
	}
	else if (!LORAM && HIRAM) {
		memtype_a000_bfff = MEMTYPE::RAM;
		memtype_e000_ffff = MEMTYPE::KERNAL;
	}
	else if (LORAM && HIRAM) {
		memtype_a000_bfff = MEMTYPE::BASIC;
		memtype_e000_ffff = MEMTYPE::KERNAL;
	}
	if (!CHAREN) {
		if (LORAM || HIRAM) {
			memtype_d000_dfff = MEMTYPE::CHARROM;
		}
	}
	else if (CHAREN) {
		if (LORAM || HIRAM) {
			memtype_d000_dfff = MEMTYPE::IO;
		}
	}
}

uint8_t readFromMem(uint16_t adr) {

	//	refresh Mapping
	refreshMemoryMapping();

	//	I/O, CHRROM, and RAM area
	if (adr >= 0xd000 && adr < 0xe000) {
		//	0xd000-0xdfff set to I/Os
		if (memtype_d000_dfff == MEMTYPE::IO) {
			//	VIC REGISTERS
			if (adr >= 0xd000 && adr <= 0xd030) {
				return readVICregister(adr);
			}
			//	COLOR RAM
			else if (adr >= 0xd800 && adr < 0xdc00) {
				return colorram[(adr % 0xd800) % 0x0fff];
			}
			//	CIA REGISTERS & DEFAULT
			else {
				switch (adr) {
					case 0xdc00:			//	read CIA1 Keyboard / Joystick
						return readCIA1DataPortA();
						break;
					case 0xdc01:			//	read CIA1 sKeyboard / Joystick
						return readCIA1DataPortB();
						break;

						//	CIA 1
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

						//	CIA 2
					case 0xdd00:			//	read VIC Bank selection
						return readCIA2DataPortA();
						break;	
					case 0xdd01:			//	read Serial Port (Unused for now)
						return readCIA2DataPortB();
						break;

					case 0xdd04:			//	read CIA1 TimerA Low
						return readCIA2timerALo();
						break;
					case 0xdd05:			//	read CIA1 TimerA High
						return readCIA2timerAHi();
						break;
					case 0xdd06:			//	read CIA1 TimerB Low
						return readCIA2timerBLo();
						break;
					case 0xdd07:			//	read CIA1 TimerB High
						return readCIA2timerBHi();
						break;

					case 0xdc0d:			//	read CIA1 IRQ Control and Status
						return readCIA1IRQStatus();
						break;
					case 0xdd0d:			//	read CIA2 NMI Control and Status
						return readCIA2NMIStatus();
						break;
					default:
						return memory[adr];
						break;
				}
			}
		}
		//	0xd000-0xdfff set to RAM
		else if (memtype_d000_dfff == MEMTYPE::RAM) {
			return memory[adr];
		}
		//	0xd000-0xdfff set to CHRROM
		else if (memtype_d000_dfff == MEMTYPE::CHARROM) {
			return chr[(adr % 0xd000) & 0x0fff];
		}
	}
	else if (adr >= 0xa000 && adr < 0xc000) {		//	BASIC
		if (memtype_a000_bfff == MEMTYPE::BASIC) {
			return basic[(adr % 0xa000) & 0x1fff];
		}
		else if (memtype_a000_bfff == MEMTYPE::RAM) {
			return memory[adr];
		}
	}
	else if (adr >= 0xe000 && adr <= 0xffff) {		//	KERNAL
		if (memtype_e000_ffff == MEMTYPE::KERNAL) {
			/*
					KERNAL HOOKS
			*/

			//	SETNAM hook
			if (adr == 0xffbd) {
				LOAD_FILE = "";
				printf("SETTING NAME y: %x x: %x\n", getCPURegs().Y, getCPURegs().X);
				for (int i = 0; i < 16; i++) {
					LOAD_FILE += memory[((getCPURegs().Y << 8) | getCPURegs().X) + i];
					cout << LOAD_FILE << "<-- FILENAME";
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
			return kernal[(adr % 0xe000) & 0x1fff];
		}
		else if (memtype_e000_ffff == MEMTYPE::RAM) {
			return memory[adr];
		}
	}
	return memory[adr & 0xffff];
}

//	reads from the VIC; the VIC can only read from a 16k window at once (dictated by Port B of CIA2)
uint8_t readFromMemByVIC(uint16_t adr) {
	uint8_t bank_no = 0b11 - (readCIA2DataPortA() & 0b11);
	//	CHRROM
	if (adr >= 0x1000 && adr <= 0x1fff && (bank_no == 0 || bank_no == 2)) {
		//return readChar(adr);
		return chr[adr % 0x1000];
	}
	//	COLORRAM
	else if (adr >= 0xd800 && adr < 0xdc00) {
		return colorram[(adr % 0xd800) & 0x0fff];
	}
	if ((adr + bank_no * 0x4000) > 0xffff)
		printf("WTF! Accessing outside of memory! adr:%x bank_no:%d fulladdr:%d\n", adr, bank_no, (adr + bank_no * 0x4000));
	return memory[adr + bank_no * 0x4000];
}

void writeToMemByVIC(uint16_t adr, uint8_t val) {
	memory[adr] = val;
}

void writeToMem(uint16_t adr, uint8_t val) {

	//	refresh Mapping
	refreshMemoryMapping();

	//	I/O, can be disabled by Zeropage $01
	if (adr >= 0xd000 && adr < 0xe000) {
		if (memtype_d000_dfff == MEMTYPE::IO) {
			//	VIC REGISTERS
			if (adr >= 0xd000 && adr <= 0xd030) {
				writeVICregister(adr, val);
			}
			//	COLOR RAM
			else if (adr >= 0xd800 && adr < 0xdc00) {
				colorram[(adr % 0xd800) % 0x0fff] = val;
			}
			//	CIA REGISTERS & DEFAULT
			else {
				switch (adr)
				{

					/*case 0xd000 ... 0xd030:			//	Set Rasterzeilen IRQ
						writeVICregister(adr, val);
						break;*/
						/*case 0xd011:
							setRasterIRQhi(val);

							break;
						case 0xd012:			//	Set Rasterzeilen IRQ
							setRasterIRQlow(val);
							break;

						case 0xd019:			//	Clear IRQ flags, that are no longer needed
							clearIRQStatus(val);
							break;
						case 0xd01a:			//	IRQ Mask (what is allowed to cause IRQs)
							setIRQMask(val);
							break;*/

							/*
								CIAs
							*/
							//	CIA 1
				case 0xdc00:			//	write CIA1 Keyboard / Joystick
					writeCIA1DataPortA(val);
					break;
				case 0xdc01:			//	write CIA1 Keyboard / Joystick
					writeCIA1DataPortB(val);
					break;
				case 0xdc02:			//	CIA1 Port A RW
					setCIA1PortARW(val);
					break;
				case 0xdc03:			//	CIA1 Port B RW
					setCIA1PortBRW(val);
					break;

					//	CIA 1
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

					//	CIA 2
				case 0xdd00:			//	write VIC Bank selection
					writeCIA2DataPortA(val);
					break;
				case 0xdd01:			//	write Serial Port (Unused for now)
					writeCIA1DataPortB(val);
					break;

				case 0xdd04:
					setCIA2timerAlatchLo(val);
					break;
				case 0xdd05:
					setCIA2timerAlatchHi(val);
					break;
				case 0xdd06:
					setCIA2timerBlatchLo(val);
					break;
				case 0xdd07:
					setCIA2timerBlatchHi(val);
					break;

					//	CIA 1
				case 0xdc0d:
					setCIA1IRQcontrol(val);
					break;
				case 0xdc0e:
					setCIA1TimerAControl(val);
					break;
				case 0xdc0f:
					setCIA1TimerBControl(val);
					break;

					//	CIA 2
				case 0xdd0d:
					setCIA2NMIcontrol(val);
					break;
				case 0xdd0e:
					setCIA2TimerAControl(val);
					break;
				case 0xdd0f:
					setCIA2TimerBControl(val);
					break;

				default:
					memory[adr] = val;
					break;
				}
			}
		}
		else if (memtype_d000_dfff == MEMTYPE::RAM) {
			memory[adr] = val;
		}
	}
	else if (adr >= 0xa000 && adr < 0xc000) {
		if (memtype_a000_bfff == MEMTYPE::RAM)
			memory[adr] = val;
	}
	else if (adr == 0xe000 && adr <= 0xffff) {
		if (memtype_e000_ffff == MEMTYPE::RAM) {
			memory[adr] = val;
		}
	}
	else {
		memory[adr] = val;
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

void dumpMem() {
	ofstream myfile("memdump_myemu.bin");
	if (myfile.is_open())
	{
		for (int i = 0; i < sizeof(memory); i++) {
			myfile << memory[i];
		}
		myfile.close();
	}
}