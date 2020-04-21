#pragma once
#include <stdint.h>
#include <stdio.h>
void printLog();
void setCarry();		//	Only for hooking C64 LOAD routines for filechecking
void clearCarry();		//	Only for hooking C64 LOAD routines for filechecking

struct Registers
{
	unsigned char A;
	unsigned char X;
	unsigned char Y;
};

struct Status
{
	uint8_t status = 0x00;	//	TODO usually 0x34 on power up 
	uint8_t carry = 0;
	uint8_t zero = 0;
	uint8_t interruptDisable = 1;
	uint8_t decimal = 0;
	uint8_t overflow = 0;
	uint8_t negative = 0;
	uint8_t brk = 0;

	void setCarry(uint8_t v) {
		carry = v;
		status = (status & ~1) | carry;
	}

	void setZero(uint8_t v) {
		zero = v;
		status = (status & ~2) | (zero << 1);
	}

	void setInterruptDisable(uint8_t v) {
		interruptDisable = v;
		status = (status & ~4) | (interruptDisable << 2);
	}

	void setDecimal(uint8_t v) {
		decimal = v;
		status = (status & ~8) | (decimal << 3);
	}

	void setOverflow(uint8_t v) {
		overflow = v;
		status = (status & ~64) | (overflow << 6);
	}

	void setNegative(uint8_t v) {
		negative = v;
		status = (status & ~128) | (negative << 7);
	}

	void setBrk(uint8_t v) {
		brk = v;
		status = (status & ~16) | (brk << 4);
	}

	void setStatus(uint8_t s) {
		status = s | 0x20;
		carry = s & 1;
		zero = (s >> 1) & 1;
		interruptDisable = (s >> 2) & 1;
		decimal = (s >> 3) & 1;
		brk = (s >> 4) & 1;
		overflow = (s >> 6) & 1;
		negative = (s >> 7) & 1;
	}

};

enum class ADDR_MODE {
	ACCUMULATOR,
	ABSOLUT,
	ABSOLUT_X,
	ABSOLUT_Y,
	INDIRECT,
	INDIRECT_Y,
	INDIRECT_X,
	ZEROPAGE,
	ZEROPAGE_X,
	ZEROPAGE_Y,
	IMMEDIATE
};

uint8_t CPU_executeInstruction();
void CPU_reset();
Registers CPU_getRegs();
void setLog(bool v);

enum class RW {
	READ, WRITE, ENDE
};

const RW RW_LOOKUP_TABLE[0x100][8]{
	{ RW::READ },																								//	0x00
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0x01
	{ RW::READ },																								//	0x02
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },						//	0x03
	{ RW::READ,  RW::READ, RW::READ },																			//	0x04
	{ RW::READ,  RW::READ, RW::READ },																			//	0x05
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE, RW::WRITE },													//	0x06
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE, RW::WRITE },													//	0x07
	{ RW::READ,  RW::READ, RW::WRITE },																			//	0x08
	{ RW::READ,  RW::READ },																					//	0x09
	{ RW::READ,  RW::READ },																					//	0x0a
	{ RW::READ },																								//	0x0b
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x0c
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x0d
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0x0e
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0x0f
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x10
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0x11
	{ RW::READ },																								//	0x12
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },						//	0x13
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x14
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x15
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0x16
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0x17
	{ RW::READ,  RW::READ },																					//	0x18
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0x19
	{ RW::READ,  RW::READ },																					//	0x1a
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },								//	0x1b
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0x1c
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0x1d
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },								//	0x1e
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },								//	0x1f
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE, RW::WRITE, RW::READ },											//	0x20
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0x21
	{ RW::READ },																								//	0x22
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },						//	0x23
	{ RW::READ,  RW::READ, RW::READ },																			//	0x24
	{ RW::READ,  RW::READ, RW::READ },																			//	0x25
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE, RW::WRITE },													//	0x26
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE, RW::WRITE },													//	0x27
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x28
	{ RW::READ,  RW::READ },																					//	0x29
	{ RW::READ,  RW::READ },																					//	0x2a
	{ RW::READ },																								//	0x2b
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x2c
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x2d
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0x2e
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0x2f
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x30
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0x31
	{ RW::READ },																								//	0x32
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },						//	0x33
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x34
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x35
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0x36
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0x37
	{ RW::READ,  RW::READ },																					//	0x38
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0x39
	{ RW::READ,  RW::READ },																					//	0x3a
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },								//	0x3b
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0x3c
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0x3d
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },								//	0x3e
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },								//	0x3f
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0x40
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0x41
	{ RW::READ },																								//	0x42
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },						//	0x43
	{ RW::READ,  RW::READ, RW::READ },																			//	0x44
	{ RW::READ,  RW::READ, RW::READ },																			//	0x45
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE, RW::WRITE },													//	0x46
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE, RW::WRITE },													//	0x47
	{ RW::READ,  RW::READ, RW::WRITE },																			//	0x48
	{ RW::READ,  RW::READ },																					//	0x49
	{ RW::READ,  RW::READ },																					//	0x4a
	{ RW::READ },																								//	0x4b
	{ RW::READ,  RW::READ, RW::READ },																			//	0x4c
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x4d
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0x4e
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0x4f
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x50
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0x51
	{ RW::READ },																								//	0x52
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },						//	0x53
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x54
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x55
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0x56
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0x57
	{ RW::READ,  RW::READ },																					//	0x58
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0x59
	{ RW::READ,  RW::READ },																					//	0x5a
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },								//	0x5b
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0x5c
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0x5d
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },								//	0x5e
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },								//	0x5f
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0x60
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0x61
	{ RW::READ },																								//	0x62
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },						//	0x63
	{ RW::READ,  RW::READ, RW::READ },																			//	0x64
	{ RW::READ,  RW::READ, RW::READ },																			//	0x65
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE, RW::WRITE },													//	0x66
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE, RW::WRITE },													//	0x67
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x68
	{ RW::READ,  RW::READ },																					//	0x69
	{ RW::READ,  RW::READ },																					//	0x6a
	{ RW::READ },																								//	0x6b
	{ RW::READ,  RW::READ },																					//	0x6c
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x6d
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0x6e
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0x6f
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x70
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0x71
	{ RW::READ },																								//	0x72
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },						//	0x73
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x74
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x75
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0x76
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0x77
	{ RW::READ,  RW::READ },																					//	0x78
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0x79
	{ RW::READ,  RW::READ },																					//	0x7a
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },								//	0x7b
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0x7c
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0x7d
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },								//	0x7e
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },								//	0x7f
	{ RW::READ,  RW::READ },																					//	0x80
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE },											//	0x81
	{ RW::READ,  RW::READ },																					//	0x82
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE },											//	0x83
	{ RW::READ,  RW::READ, RW::WRITE },																			//	0x84
	{ RW::READ,  RW::READ, RW::WRITE },																			//	0x85
	{ RW::READ,  RW::READ, RW::WRITE },																			//	0x86
	{ RW::READ,  RW::READ, RW::WRITE },																			//	0x87
	{ RW::READ,  RW::READ },																					//	0x88
	{ RW::READ,  RW::READ },																					//	0x89
	{ RW::READ,  RW::READ },																					//	0x8a
	{ RW::READ },																								//	0x8b
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE },																//	0x8c	
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE },																//	0x8d
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE },																//	0x8e
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE },																//	0x8f
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0x90
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE },											//	0x91
	{ RW::READ },																								//	0x92
	{ RW::READ },																								//	0x93
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE },																//	0x94
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE },																//	0x95
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE },																//	0x96
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE },																//	0x97
	{ RW::READ,  RW::READ },																					//	0x98
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE },														//	0x99
	{ RW::READ,  RW::READ },																					//	0x9a
	{ RW::READ },																								//	0x9b
	{ RW::READ },																								//	0x9c
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE },														//	0x9d
	{ RW::READ },																								//	0x9e
	{ RW::READ },																								//	0x9f
	{ RW::READ,  RW::READ },																					//	0xa0
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0xa1
	{ RW::READ,  RW::READ },																					//	0xa2
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0xa3
	{ RW::READ,  RW::READ, RW::READ },																			//	0xa4
	{ RW::READ,  RW::READ, RW::READ },																			//	0xa5
	{ RW::READ,  RW::READ, RW::READ },																			//	0xa6
	{ RW::READ,  RW::READ, RW::READ },																			//	0xa7
	{ RW::READ,  RW::READ },																					//	0xa8
	{ RW::READ,  RW::READ },																					//	0xa9
	{ RW::READ,  RW::READ },																					//	0xaa
	{ RW::READ,  RW::READ },																					//	0xab
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xac
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xad
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xae
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xaf
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xb0
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0xb1
	{ RW::READ },																								//	0xb2
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0xb3
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xb4
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xb5
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xb6
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xb7
	{ RW::READ,  RW::READ },																					//	0xb8
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0xb9
	{ RW::READ,  RW::READ },																					//	0xba
	{ RW::READ },																								//	0xbb
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0xbc
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0xbd
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0xbe
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0xbf
	{ RW::READ,  RW::READ },																					//	0xc0
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0xc1
	{ RW::READ,  RW::READ },																					//	0xc2
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },						//	0xc3
	{ RW::READ,  RW::READ, RW::READ },																			//	0xc4
	{ RW::READ,  RW::READ, RW::READ },																			//	0xc5
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE, RW::WRITE },													//	0xc6
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE, RW::WRITE },													//	0xc7
	{ RW::READ,  RW::READ },																					//	0xc8
	{ RW::READ,  RW::READ },																					//	0xc9
	{ RW::READ,  RW::READ },																					//	0xca
	{ RW::READ },																								//	0xcb
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xcc
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xcd
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0xce
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0xcf
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xd0
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0xd1
	{ RW::READ },																								//	0xd2
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },						//	0xd3
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xd4
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xd5
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0xd6
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0xd7
	{ RW::READ,  RW::READ },																					//	0xd8
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0xd9
	{ RW::READ,  RW::READ },																					//	0xda
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },								//	0xdb
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0xdc
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0xdd
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },								//	0xde
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },								//	0xdf
	{ RW::READ,  RW::READ },																					//	0xe0
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0xe1
	{ RW::READ,  RW::READ },																					//	0xe2
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },						//	0xe3
	{ RW::READ,  RW::READ, RW::READ },																			//	0xe4
	{ RW::READ,  RW::READ, RW::READ },																			//	0xe5
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE, RW::WRITE },													//	0xe6
	{ RW::READ,  RW::READ, RW::READ, RW::WRITE, RW::WRITE },													//	0xe7
	{ RW::READ,  RW::READ },																					//	0xe8
	{ RW::READ,  RW::READ },																					//	0xe9
	{ RW::READ,  RW::READ },																					//	0xea
	{ RW::READ,  RW::READ },																					//	0xeb
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xec
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xed
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0xee
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0xef
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xf0
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ },											//	0xf1
	{ RW::READ },																								//	0xf2
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },						//	0xf3
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xf4
	{ RW::READ,  RW::READ, RW::READ, RW::READ },																//	0xf5
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0xf6
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },											//	0xf7
	{ RW::READ,  RW::READ },																					//	0xf8
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0xf9
	{ RW::READ,  RW::READ },																					//	0xfa
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },								//	0xfb
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0xfc
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ },														//	0xfd
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE },								//	0xfe
	{ RW::READ,  RW::READ, RW::READ, RW::READ, RW::READ, RW::WRITE, RW::WRITE }									//	0xff
};
