#pragma once
#include "cpu.h"
#include "mmu.h"
#include "main.h"
#include "ppu.h"
#include <map>

//	set up vars
uint16_t PC_L = 0x0000, PC_H = 0x0000;
uint16_t PC = 0xc000;
uint8_t SP_ = 0xff;
Registers registers;
Status status;

bool irq = false;
bool irq_running = false;
bool nmi = false;
bool nmi_running = false;
uint8_t fr = 1;

void setCarry() {				//	Only for hooking C64 LOAD routines for filechecking
	status.carry = 1;
}

void clearCarry() {				//	Only for hooking C64 LOAD routines for filechecking
	status.carry = 0;
}

/*
		STATE MACHINE REWRITE OPCODES | START
*/

void _ROLA(uint8_t state) {
	switch (state)
	{
	case 1: break;
	case 2: {
		uint8_t tmp = status.carry;
		status.setCarry(registers.A >> 7);
		registers.A = (registers.A << 1) | tmp;
		status.setZero(registers.A == 0);
		status.setNegative(registers.A >> 7);
		fr = 0;
		PC++;
		break;
	}
	default:
		break;
	}
}

void _RORA(uint8_t state) {
	switch (state)
	{
	case 1: break;
	case 2: {
		uint8_t tmp = registers.A & 1;
		registers.A = (registers.A >> 1) | (status.carry << 7);
		status.setCarry(tmp);
		status.setZero(registers.A == 0);
		status.setNegative(registers.A >> 7);
		fr = 0;
		PC++;
		break;
	}
	default:
		break;
	}
}

void _LSRA(uint8_t state) {
	switch (state)
	{
	case 1: break;
	case 2: {
		status.setCarry(registers.A & 1);
		registers.A = (registers.A >> 1);
		status.setZero(registers.A == 0);
		status.setNegative(registers.A >> 7);
		fr = 0;
		PC++;
		break;
	}
	default:
		break;
	}
}

void _RTI(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3:	SP_++; break;
	case 4:	status.setStatus(readFromMem(SP_ + 0x100));	SP_++; break;
	case 5:	PC_L = readFromMem(SP_ + 0x100); SP_++;	break;
	case 6:	PC_H = readFromMem(SP_ + 0x100); PC = (PC_H << 8) | PC_L; fr = 0; break;
	default: break;
	}
}

void _RTS(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3:	SP_++; break;
	case 4:	PC_L = readFromMem(SP_ + 0x100); SP_++;	break;
	case 5:	PC_H = readFromMem(SP_ + 0x100); break;
	case 6: PC = (PC_H << 8) | PC_L; PC++; fr = 0; break;
	default: break;
	}
}

void _PHA(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: {
		writeToMem(SP_ + 0x100, registers.A); 
		SP_--;
		PC++;
		fr = 0;
	} break;
	default: break;
	}
}

void _PHP(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: {
		writeToMem(SP_ + 0x100, status.status | 0x30); 
		SP_--;
		PC++;
		fr = 0;
	} break;
	default: break;
	}
}

void _PLA(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3:	SP_++; break;
	case 4:	registers.A = readFromMem(SP_ + 0x100); status.setNegative(registers.A >> 7); status.setZero(registers.A == 0);	fr = 0; PC++; break;
	default: break;
	}
}

void _PLP(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3:	SP_++; break;
	case 4:	status.setStatus(readFromMem(SP_ + 0x100) & 0xef); fr = 0; PC++; break;
	default: break;
	}
}

void _JSR(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: break;
	case 4:	writeToMem(SP_ + 0x100, (PC + 2) >> 8); SP_--; break;
	case 5: writeToMem(SP_ + 0x100, (PC + 2) & 0xff); SP_--; break;
	case 6: PC = getAbsolute(PC); fr = 0; break;
	default: break;
	}
}

void _JMP(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: if (mode == ADDR_MODE::ABSOLUT)		{ PC = getAbsolute(PC); fr = 0; } break;
	case 4: break;
	case 5: if (mode == ADDR_MODE::INDIRECT)	{ PC = getIndirect(PC); fr = 0; } break;
	default: break;
	}
}

void _mLDX(uint8_t &tar, uint16_t adr) {
	tar = readFromMem(adr);
	status.setZero(tar == 0);
	status.setNegative(tar >> 7);
}

void _LDX(uint8_t state, uint8_t& tar, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	if		(mode == ADDR_MODE::IMMEDIATE)	{ _mLDX(tar, getImmediate(PC)); PC += 2; fr = 0; } break;
	case 3: if		(mode == ADDR_MODE::ZEROPAGE)	{ _mLDX(tar, getZeropage(PC)); PC += 2;	fr = 0;	} break;
	case 4: if		(mode == ADDR_MODE::ABSOLUT)	{ _mLDX(tar, getAbsolute(PC)); PC += 3;	fr = 0;	} 
			else if (mode == ADDR_MODE::ZEROPAGE_X) { _mLDX(tar, getZeropageXIndex(PC, registers.X)); PC += 2; fr = 0; }
			else if (mode == ADDR_MODE::ZEROPAGE_Y) { _mLDX(tar, getZeropageYIndex(PC, registers.Y)); PC += 2; fr = 0; } 
			else if (mode == ADDR_MODE::ABSOLUT_X)	{
				uint16_t _t = ((readFromMem(PC + 2) << 8) | readFromMem(PC + 1)); //	BUGFIX? added PC+1 | PC to PC+2 | PC+1
				if ((_t & 0xff00) == ((_t + registers.X) & 0xff00)) {	//	PB not crossed
					_mLDX(tar, getAbsoluteXIndex(PC, registers.X));
					PC += 3;
					fr = 0;
				}
			} 
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{
				uint16_t _t = ((readFromMem(PC + 2) << 8) | readFromMem(PC + 1)); //	BUGFIX? added PC+1 | PC to PC+2 | PC+1
				if ((_t & 0xff00) == ((_t + registers.Y) & 0xff00)) {	//	PB not crossed
					_mLDX(tar, getAbsoluteYIndex(PC, registers.Y));
					PC += 3;
					fr = 0;
				}
			} break;
	//	ADDITIONAL CYCLE, ADDR_MODE::ABSOLUT X / Y
	case 5: if		(mode == ADDR_MODE::ABSOLUT_X)	{ _mLDX(tar, getAbsoluteXIndex(PC, registers.X)); PC += 3; fr = 0; } 
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{ _mLDX(tar, getAbsoluteYIndex(PC, registers.Y)); PC += 3; fr = 0; }
			else if (mode == ADDR_MODE::INDIRECT_Y) {
				uint16_t _t = ((readFromMem((readFromMem(PC + 1) + 1) % 0x100) << 8) | (readFromMem(readFromMem(PC + 1))));
				if ((_t & 0xff00) == ((_t + registers.Y) & 0xff00)) {
					_mLDX(tar, getIndirectYIndex(PC, registers.Y));
					PC += 2;
					fr = 0;
				}
			} break;
	case 6: if		(mode == ADDR_MODE::INDIRECT_X)	{ _mLDX(tar, getIndirectXIndex(PC, registers.X)); PC += 2; fr = 0; }
			else if (mode == ADDR_MODE::INDIRECT_Y) { _mLDX(tar, getIndirectYIndex(PC, registers.Y)); PC += 2; fr = 0; } break;
	default: break;
	}
}

void  _mEOR(uint16_t adr) {
	registers.A ^= readFromMem(adr);
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
}

void _EOR(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	if		(mode == ADDR_MODE::IMMEDIATE)	{ _mEOR(getImmediate(PC)); PC += 2; fr = 0; } break;
	case 3: if		(mode == ADDR_MODE::ZEROPAGE)	{ _mEOR(getZeropage(PC)); PC += 2; fr = 0; } break;
	case 4: if		(mode == ADDR_MODE::ABSOLUT)	{ _mEOR(getAbsolute(PC)); PC += 3; fr = 0; } 
			else if (mode == ADDR_MODE::ZEROPAGE_X) { _mEOR(getZeropageXIndex(PC, registers.X)); PC += 2; fr = 0; }
			else if (mode == ADDR_MODE::ABSOLUT_X)	{
				uint16_t _t = ((readFromMem(PC + 2) << 8) | readFromMem(PC + 1));
				if ((_t & 0xff00) == ((_t + registers.X) & 0xff00)) {	//	PB not crossed
					_mEOR(getAbsoluteXIndex(PC, registers.X));
					PC += 3;
					fr = 0;
				}
			}
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{
				uint16_t _t = ((readFromMem(PC + 2) << 8) | readFromMem(PC + 1));
				if ((_t & 0xff00) == ((_t + registers.Y) & 0xff00)) {	//	PB not crossen
					_mEOR(getAbsoluteYIndex(PC, registers.Y));
					PC += 3;
					fr = 0;
				}
			} break;
	case 5: if		(mode == ADDR_MODE::ABSOLUT_X)	{ _mEOR(getAbsoluteXIndex(PC, registers.X)); PC += 3; fr = 0; } 
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{ _mEOR(getAbsoluteYIndex(PC, registers.Y)); PC += 3; fr = 0; }
			else if (mode == ADDR_MODE::INDIRECT_Y) {
				uint16_t _t = ((readFromMem((readFromMem(PC + 1) + 1) % 0x100) << 8) | (readFromMem(readFromMem(PC + 1))));
				if ((_t & 0xff00) == ((_t + registers.Y) & 0xff00)) {
					_mEOR(getIndirectYIndex(PC, registers.Y));
					PC += 2;
					fr = 0;
				}
			} break;
	case 6: if		(mode == ADDR_MODE::INDIRECT_X) { _mEOR(getIndirectXIndex(PC, registers.X)); PC += 2; fr = 0; }
			else if (mode == ADDR_MODE::INDIRECT_Y) { _mEOR(getIndirectYIndex(PC, registers.Y)); PC += 2; fr = 0; }	break;
	default: break;
	}
}

void _mAND(uint16_t adr) {
	registers.A &= readFromMem(adr);
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
}

void _AND(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	if		(mode == ADDR_MODE::IMMEDIATE)	{ _mAND(getImmediate(PC)); PC += 2; fr = 0; } break;
	case 3: if		(mode == ADDR_MODE::ZEROPAGE)	{ _mAND(getZeropage(PC)); PC += 2; fr = 0; } break;
	case 4: if		(mode == ADDR_MODE::ABSOLUT)	{ _mAND(getAbsolute(PC)); PC += 3; fr = 0; } 
			else if (mode == ADDR_MODE::ZEROPAGE_X) { _mAND(getZeropageXIndex(PC, registers.X)); PC += 2; fr = 0; }
			else if (mode == ADDR_MODE::ABSOLUT_X)	{
				uint16_t _t = ((readFromMem(PC + 2) << 8) | readFromMem(PC + 1));
				if ((_t & 0xff00) == ((_t + registers.X) & 0xff00)) {	//	PB not crossed
					_mAND(getAbsoluteXIndex(PC, registers.X));
					PC += 3;
					fr = 0;
				}
			}
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{
				uint16_t _t = ((readFromMem(PC + 2) << 8) | readFromMem(PC + 1));
				if ((_t & 0xff00) == ((_t + registers.Y) & 0xff00)) {	//	PB not crossen
					_mAND(getAbsoluteYIndex(PC, registers.Y));
					PC += 3;
					fr = 0;
				}
			} break;
	case 5: if		(mode == ADDR_MODE::ABSOLUT_X)	{ _mAND(getAbsoluteXIndex(PC, registers.X)); PC += 3; fr = 0; }
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{ _mAND(getAbsoluteYIndex(PC, registers.Y)); PC += 3; fr = 0; }
			else if (mode == ADDR_MODE::INDIRECT_Y) {
				uint16_t _t = ((readFromMem((readFromMem(PC + 1) + 1) % 0x100) << 8) | (readFromMem(readFromMem(PC + 1))));
				if ((_t & 0xff00) == ((_t + registers.Y) & 0xff00)) {
					_mAND(getIndirectYIndex(PC, registers.Y));
					PC += 2;
					fr = 0;
				}
			} break;
	case 6: if (mode == ADDR_MODE::INDIRECT_X)		{ _mAND(getIndirectXIndex(PC, registers.X)); PC += 2; fr = 0; }
			else if (mode == ADDR_MODE::INDIRECT_Y) { _mAND(getIndirectYIndex(PC, registers.Y)); PC += 2; fr = 0; } break;
	default: break;
	}
}

void _mORA(uint16_t adr) {
	registers.A |= readFromMem(adr);
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
}

void _ORA(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	if		(mode == ADDR_MODE::IMMEDIATE)	{ _mORA(getImmediate(PC)); PC += 2; fr = 0; } break;
	case 3: if		(mode == ADDR_MODE::ZEROPAGE)	{ _mORA(getZeropage(PC)); PC += 2; fr = 0; } break;
	case 4: if		(mode == ADDR_MODE::ABSOLUT)	{ _mORA(getAbsolute(PC)); PC += 3; fr = 0; } 
			else if (mode == ADDR_MODE::ZEROPAGE_X) { _mORA(getZeropageXIndex(PC, registers.X)); PC += 2; fr = 0; }
			else if (mode == ADDR_MODE::ABSOLUT_X)	{
				uint16_t _t = ((readFromMem(PC + 2) << 8) | readFromMem(PC + 1));
				if ((_t & 0xff00) == ((_t + registers.X) & 0xff00)) {	//	PB not crossed
					_mORA(getAbsoluteXIndex(PC, registers.X));
					PC += 3;
					fr = 0;
				}
			}
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{
				uint16_t _t = ((readFromMem(PC + 2) << 8) | readFromMem(PC + 1));
				if ((_t & 0xff00) == ((_t + registers.Y) & 0xff00)) {	//	PB not crossen
					_mORA(getAbsoluteYIndex(PC, registers.Y));
					PC += 3;
					fr = 0;
				}
			} break;
	case 5: if		(mode == ADDR_MODE::ABSOLUT_X)	{ _mORA(getAbsoluteXIndex(PC, registers.X)); PC += 3; fr = 0; } 
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{ _mORA(getAbsoluteYIndex(PC, registers.Y)); PC += 3; fr = 0; }
			else if (mode == ADDR_MODE::INDIRECT_Y) {
				uint16_t _t = ((readFromMem((readFromMem(PC + 1) + 1) % 0x100) << 8) | (readFromMem(readFromMem(PC + 1))));
				if ((_t & 0xff00) == ((_t + registers.Y) & 0xff00)) {
					_mORA(getIndirectYIndex(PC, registers.Y));
					PC += 2;
					fr = 0;
				}
			} break;
	case 6: if		(mode == ADDR_MODE::INDIRECT_X) { _mORA(getIndirectXIndex(PC, registers.X)); PC += 2; fr = 0; } 
			else if (mode == ADDR_MODE::INDIRECT_Y) { _mORA(getIndirectYIndex(PC, registers.Y)); PC += 2; fr = 0; } break;
	default: break;
	}
}

void _mADD(uint8_t val) {
	//	hex mode
	if (status.decimal == 0) {
		uint16_t sum = registers.A + val + status.carry;
		status.setOverflow((~(registers.A ^ val) & (registers.A ^ sum) & 0x80) > 0);
		status.setCarry(sum > 0xff);
		registers.A = sum & 0xff;
		status.setZero(registers.A == 0);
		status.setNegative(registers.A >> 7);
	}
	//	BCD mode
	else {
		uint16_t v1 = (registers.A / 16) * 10 + (registers.A % 16);
		int16_t v2 = ((int8_t)val / 16) * 10 + ((int8_t)val % 16);
		uint16_t sum = (v1 + v2 + status.carry);
		status.setCarry(sum > 99);
		registers.A = ((sum / 10) * 16) + (sum % 10);
	}
}

void _ADC(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	if		(mode == ADDR_MODE::IMMEDIATE)	{ _mADD(readFromMem(getImmediate(PC))); PC += 2; fr = 0; } break;
	case 3: if		(mode == ADDR_MODE::ZEROPAGE)	{ _mADD(readFromMem(getZeropage(PC))); PC += 2; fr = 0; } break;
	case 4: if		(mode == ADDR_MODE::ABSOLUT)	{ _mADD(readFromMem(getAbsolute(PC))); PC += 3;	fr = 0; } 
			else if (mode == ADDR_MODE::ZEROPAGE_X) { _mADD(readFromMem(getZeropageXIndex(PC, registers.X))); PC += 2; fr = 0; }
			else if (mode == ADDR_MODE::ABSOLUT_X)	{
				uint16_t _t = ((readFromMem(PC + 2) << 8) | readFromMem(PC + 1));
				if ((_t & 0xff00) == ((_t + registers.X) & 0xff00)) {	//	PB not crossed
					_mADD(readFromMem(getAbsoluteXIndex(PC, registers.X)));
					PC += 3;
					fr = 0;
				}
			}
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{
				uint16_t _t = ((readFromMem(PC + 2) << 8) | readFromMem(PC + 1));
				if ((_t & 0xff00) == ((_t + registers.Y) & 0xff00)) {	//	PB not crossen
					_mADD(readFromMem(getAbsoluteYIndex(PC, registers.Y)));
					PC += 3;
					fr = 0;
				}
			} break;
	case 5: if		(mode == ADDR_MODE::ABSOLUT_X)	{ _mADD(readFromMem(getAbsoluteXIndex(PC, registers.X))); PC += 3; fr = 0; }
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{ _mADD(readFromMem(getAbsoluteYIndex(PC, registers.Y))); PC += 3; fr = 0; }
			else if (mode == ADDR_MODE::INDIRECT_Y) {
				uint16_t _t = ((readFromMem((readFromMem(PC + 1) + 1) % 0x100) << 8) | (readFromMem(readFromMem(PC + 1))));
				if ((_t & 0xff00) == ((_t + registers.Y) & 0xff00)) {
					_mADD(readFromMem(getIndirectYIndex(PC, registers.Y)));
					PC += 2;
					fr = 0;
				}
			} break;
	case 6: if		(mode == ADDR_MODE::INDIRECT_X)	{ _mADD(readFromMem(getIndirectXIndex(PC, registers.X))); PC += 2; fr = 0; } 
			else if (mode == ADDR_MODE::INDIRECT_Y) { _mADD(readFromMem(getIndirectYIndex(PC, registers.Y))); PC += 2; fr = 0; } break;
	default: break;
	}
}

void _SBC(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	if		(mode == ADDR_MODE::IMMEDIATE)	{ _mADD(~readFromMem(getImmediate(PC))); PC += 2; fr = 0; } break;
	case 3: if		(mode == ADDR_MODE::ZEROPAGE)	{ _mADD(~readFromMem(getZeropage(PC))); PC += 2; fr = 0; } break;
	case 4: if		(mode == ADDR_MODE::ABSOLUT)	{ _mADD(~readFromMem(getAbsolute(PC)));	PC += 3; fr = 0; }
			else if (mode == ADDR_MODE::ZEROPAGE_X) { _mADD(~readFromMem(getZeropageXIndex(PC, registers.X))); PC += 2;	fr = 0;	} 
			else if (mode == ADDR_MODE::ABSOLUT_X)	{
				uint16_t _t = ((readFromMem(PC + 2) << 8) | readFromMem(PC + 1));
				if ((_t & 0xff00) == ((_t + registers.X) & 0xff00)) {	//	PB not crossed
					_mADD(~readFromMem(getAbsoluteXIndex(PC, registers.X)));
					PC += 3;
					fr = 0;
				}
			}
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{
				uint16_t _t = ((readFromMem(PC + 2) << 8) | readFromMem(PC + 1));
				if ((_t & 0xff00) == ((_t + registers.Y) & 0xff00)) {	//	PB not crossen
					_mADD(~readFromMem(getAbsoluteYIndex(PC, registers.Y)));
					PC += 3;
					fr = 0;
				}
			} break;
	case 5: if		(mode == ADDR_MODE::ABSOLUT_X)	{ _mADD(~readFromMem(getAbsoluteXIndex(PC, registers.X))); PC += 3;	fr = 0;	}
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{ _mADD(~readFromMem(getAbsoluteYIndex(PC, registers.Y))); PC += 3;	fr = 0;	} 
			else if (mode == ADDR_MODE::INDIRECT_Y) {
				uint16_t _t = ((readFromMem((readFromMem(PC + 1) + 1) % 0x100) << 8) | (readFromMem(readFromMem(PC + 1))));
				if ((_t & 0xff00) == ((_t + registers.Y) & 0xff00)) {
					_mADD(~readFromMem(getIndirectYIndex(PC, registers.Y)));
					PC += 2;
					fr = 0;
				}
			} break;
	case 6: if		(mode == ADDR_MODE::INDIRECT_X) { _mADD(~readFromMem(getIndirectXIndex(PC, registers.X))); PC += 2;	fr = 0; } 
			else if (mode == ADDR_MODE::INDIRECT_Y) { _mEOR(~readFromMem(getIndirectYIndex(PC, registers.Y))); PC += 2;	fr = 0;	} break;
	default: break;
	}
}

void _mCMP(uint8_t& tar, uint16_t adr) {
	status.setCarry(tar >= readFromMem(adr));
	status.setZero(readFromMem(adr) == tar);
	status.setNegative(((tar - readFromMem(adr)) & 0xff) >> 7);
}

void _CMP(uint8_t state, uint8_t &tar, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	if		(mode == ADDR_MODE::IMMEDIATE)	{ _mCMP(tar, getImmediate(PC)); PC += 2; fr = 0; } break;
	case 3: if		(mode == ADDR_MODE::ZEROPAGE)	{ _mCMP(tar, getZeropage(PC)); PC += 2;	fr = 0; } break;
	case 4: if		(mode == ADDR_MODE::ABSOLUT)	{ _mCMP(tar, getAbsolute(PC)); PC += 3;	fr = 0; }
			else if (mode == ADDR_MODE::ZEROPAGE_X) { _mCMP(tar, getZeropageXIndex(PC, registers.X)); PC += 2; fr = 0; } 
			else if (mode == ADDR_MODE::ABSOLUT_X)	{
				uint16_t _t = ((readFromMem(PC + 2) << 8) | readFromMem(PC + 1));
				if ((_t & 0xff00) == ((_t + registers.X) & 0xff00)) {	//	PB not crossed
					_mCMP(tar, getAbsoluteXIndex(PC, registers.X));
					PC += 3;
					fr = 0;
				}
			}
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{
				uint16_t _t = ((readFromMem(PC + 2) << 8) | readFromMem(PC + 1));
				if ((_t & 0xff00) == ((_t + registers.Y) & 0xff00)) {	//	PB not crossen
					_mCMP(tar, getAbsoluteYIndex(PC, registers.Y));
					PC += 3;
					fr = 0;
				}
			} break;
	case 5: if		(mode == ADDR_MODE::ABSOLUT_X)	{ _mCMP(tar, getAbsoluteXIndex(PC, registers.X)); PC += 3; fr = 0; }
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{ _mCMP(tar, getAbsoluteYIndex(PC, registers.Y)); PC += 3; fr = 0; } 
			else if (mode == ADDR_MODE::INDIRECT_Y) {
				uint16_t _t = ((readFromMem((readFromMem(PC + 1) + 1) % 0x100) << 8) | (readFromMem(readFromMem(PC + 1))));
				if ((_t & 0xff00) == ((_t + registers.Y) & 0xff00)) {
					_mCMP(tar, getIndirectYIndex(PC, registers.Y));
					PC += 2;
					fr = 0;
				}
			} break;
	case 6: if		(mode == ADDR_MODE::INDIRECT_X)	{ _mCMP(tar, getIndirectXIndex(PC, registers.X)); PC += 2; fr = 0; } 
			else if (mode == ADDR_MODE::INDIRECT_Y) { _mCMP(tar, getIndirectYIndex(PC, registers.Y)); PC += 2; fr = 0; } break;
	default: break;
	}
}

void _mBIT(uint16_t adr) {
	status.setNegative(readFromMem(adr) >> 7);
	status.setOverflow((readFromMem(adr) >> 6) & 1);
	status.setZero((registers.A & readFromMem(adr)) == 0);
}

void _BIT(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: if (mode == ADDR_MODE::ZEROPAGE)	{ _mBIT(getZeropage(PC)); PC += 2; fr = 0; } break;
	case 4: if (mode == ADDR_MODE::ABSOLUT)		{ _mBIT(getAbsolute(PC)); PC += 3; fr = 0; } break;
	default: break;
	}
}

void _mLAX(uint16_t adr) {
	registers.A = readFromMem(adr);
	registers.X = readFromMem(adr);
	status.setNegative(readFromMem(adr) >> 7);
	status.setZero((registers.A & readFromMem(adr)) == 0);
}

void _LAX(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: if		(mode == ADDR_MODE::ZEROPAGE)	{ _mLAX(getZeropage(PC)); PC += 2; fr = 0; } break;
	case 4: if		(mode == ADDR_MODE::ABSOLUT)	{ _mLAX(getAbsolute(PC)); PC += 2; fr = 0; }
			else if (mode == ADDR_MODE::ZEROPAGE_Y) { _mLAX(getZeropageYIndex(PC, registers.Y)); PC += 2; fr = 0; } 
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{ 
				uint16_t _t = ((readFromMem(PC + 2) << 8) | readFromMem(PC + 1));
				if ((_t & 0xff00) == ((_t + registers.Y) & 0xff00)) {	//	PB not crossed
					_mLAX(getAbsoluteYIndex(PC, registers.Y));
					PC += 2;
					fr = 0;
				}
			}
	break;
	case 5: if		(mode == ADDR_MODE::ABSOLUT_Y)	{ _mLAX(getAbsoluteYIndex(PC, registers.Y)); PC += 2; fr = 0; } break;
	default: break;
	}
}

void _mASL(uint16_t adr) {
	status.setCarry(readFromMem(adr) >> 7);
	writeToMem(adr, (readFromMem(adr) << 1));
	status.setZero(readFromMem(adr) == 0);
	status.setNegative(readFromMem(adr) >> 7);
}

void _ASL(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: break;
	case 4: break;
	case 5: if		(mode == ADDR_MODE::ZEROPAGE)	{ _mASL(getZeropage(PC)); PC += 2; fr = 0; } break;
	case 6: if		(mode == ADDR_MODE::ABSOLUT)	{ _mASL(getAbsolute(PC)); PC += 3; fr = 0; }
			else if (mode == ADDR_MODE::ZEROPAGE_X) { _mASL(getZeropageXIndex(PC, registers.X)); PC += 2; fr = 0; } break;
	case 7: if		(mode == ADDR_MODE::ABSOLUT_X)	{ _mASL(getAbsoluteXIndex(PC, registers.X)); PC += 3; fr = 0; } break;
	default: break;
	}
}

void _ASLA(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: { status.setCarry(registers.A >> 7); registers.A = registers.A << 1; status.setZero(registers.A == 0); status.setNegative(registers.A >> 7); PC++; fr = 0; } break;
	default: break;
	}
}

void _mLSR(uint16_t adr) {
	status.setCarry(readFromMem(adr) & 1);
	writeToMem(adr, (readFromMem(adr) >> 1));
	status.setZero(readFromMem(adr) == 0);
	status.setNegative(readFromMem(adr) >> 7);
}

void _LSR(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: break;
	case 4: break;
	case 5: if		(mode == ADDR_MODE::ZEROPAGE)	{ _mLSR(getZeropage(PC)); PC += 2; fr = 0; } break;
	case 6: if		(mode == ADDR_MODE::ABSOLUT)	{ _mLSR(getAbsolute(PC)); PC += 3; fr = 0; }
			else if (mode == ADDR_MODE::ZEROPAGE_X) { _mLSR(getZeropageXIndex(PC, registers.X)); PC += 2; fr = 0; } break;
	case 7: if		(mode == ADDR_MODE::ABSOLUT_X)	{ _mLSR(getAbsoluteXIndex(PC, registers.X)); PC += 3; fr = 0; } break;
	default: break;
	}
}

void _mROL(uint16_t adr) {
	uint8_t tmp = status.carry;
	status.setCarry(readFromMem(adr) >> 7);
	writeToMem(adr, (readFromMem(adr) << 1) | tmp);
	status.setZero(readFromMem(adr) == 0);
	status.setNegative(readFromMem(adr) >> 7);
}

void _ROL(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: break;
	case 4: break;
	case 5: if		(mode == ADDR_MODE::ZEROPAGE)	{ _mROL(getZeropage(PC)); PC += 2; fr = 0; } break;
	case 6: if		(mode == ADDR_MODE::ABSOLUT)	{ _mROL(getAbsolute(PC)); PC += 3; fr = 0; }
			else if (mode == ADDR_MODE::ZEROPAGE_X) { _mROL(getZeropageXIndex(PC, registers.X)); PC += 2; fr = 0; } break;
	case 7: if		(mode == ADDR_MODE::ABSOLUT_X)	{ _mROL(getAbsoluteXIndex(PC, registers.X)); PC += 3; fr = 0; } break;
	default: break;
	}
}

void _mROR(uint16_t adr) {
	uint8_t tmp = readFromMem(adr) & 1;
	writeToMem(adr, (readFromMem(adr) >> 1) | (status.carry << 7));
	status.setCarry(tmp);
	status.setZero(readFromMem(adr) == 0);
	status.setNegative(readFromMem(adr) >> 7);
}

void _ROR(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: break;
	case 4: break;
	case 5: if		(mode == ADDR_MODE::ZEROPAGE)	{ _mROR(getZeropage(PC)); PC += 2; fr = 0; } break;
	case 6: if		(mode == ADDR_MODE::ABSOLUT)	{ _mROR(getAbsolute(PC)); PC += 3; fr = 0; }
			else if (mode == ADDR_MODE::ZEROPAGE_X) { _mROR(getZeropageXIndex(PC, registers.X)); PC += 2; fr = 0; } break;
	case 7: if		(mode == ADDR_MODE::ABSOLUT_X)	{ _mROR(getAbsoluteXIndex(PC, registers.X)); PC += 3; fr = 0; } break;
	default: break;
	}
}

void _NOP(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: { PC++;	fr = 0;	break; }
	default: break;
	}
}

void _mINC(uint16_t adr) {
	writeToMem(adr, readFromMem(adr) + 1);
	status.setZero(readFromMem(adr) == 0);
	status.setNegative(readFromMem(adr) >> 7);
}

void _INC(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: break;
	case 4: break;
	case 5: if		(mode == ADDR_MODE::ZEROPAGE)	{ _mINC(getZeropage(PC)); PC += 2; fr = 0; } break;
	case 6: if		(mode == ADDR_MODE::ABSOLUT)	{ _mINC(getAbsolute(PC)); PC += 3; fr = 0; }
			else if (mode == ADDR_MODE::ZEROPAGE_X) { _mINC(getZeropageXIndex(PC, registers.X)); PC += 2; fr = 0; } break;
	case 7: if		(mode == ADDR_MODE::ABSOLUT_X)	{ _mINC(getAbsoluteXIndex(PC, registers.X)); PC += 3; fr = 0; } break;
	default: break;
	}
}

void _mDEC(uint16_t adr) {
	writeToMem(adr, readFromMem(adr) - 1);
	status.setZero(readFromMem(adr) == 0);
	status.setNegative(readFromMem(adr) >> 7);
}

void _DEC(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: break;
	case 4: break;
	case 5: if		(mode == ADDR_MODE::ZEROPAGE)	{ _mDEC(getZeropage(PC)); PC += 2; fr = 0; } break;
	case 6: if		(mode == ADDR_MODE::ABSOLUT)	{ _mDEC(getAbsolute(PC)); PC += 3; fr = 0; }
			else if (mode == ADDR_MODE::ZEROPAGE_X) { _mDEC(getZeropageXIndex(PC, registers.X)); PC += 2; fr = 0; } break;
	case 7: if		(mode == ADDR_MODE::ABSOLUT_X)	{ _mDEC(getAbsoluteXIndex(PC, registers.X)); PC += 3; fr = 0; } break;
	default: break;
	}
}

void _SLO(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: break;
	case 4: break;
	case 5: if		(mode == ADDR_MODE::ZEROPAGE)	{ uint16_t _a = getZeropage(PC); _mASL(_a); _mORA(_a); PC += 2; fr = 0; } break;
	case 6: if		(mode == ADDR_MODE::ABSOLUT)	{ uint16_t _a = getAbsolute(PC); _mASL(_a);	_mORA(_a); PC += 2;	fr = 0; }
			else if (mode == ADDR_MODE::ZEROPAGE_X) { uint16_t _a = getZeropageXIndex(PC, registers.X); _mASL(_a); _mORA(_a); PC += 2; fr = 0; } break;
	case 7: if		(mode == ADDR_MODE::ABSOLUT_X)	{ uint16_t _a = getAbsoluteXIndex(PC, registers.X);	_mASL(_a); _mORA(_a); PC += 2; fr = 0; } 
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{ uint16_t _a = getAbsoluteYIndex(PC, registers.Y);	_mASL(_a); _mORA(_a); PC += 2; fr = 0; } break;
	case 8: if		(mode == ADDR_MODE::INDIRECT_X) { uint16_t _a = getIndirectXIndex(PC, registers.X);	_mASL(_a); _mORA(_a); PC += 2; fr = 0; } 
			else if (mode == ADDR_MODE::INDIRECT_Y) { uint16_t _a = getIndirectYIndex(PC, registers.Y);	_mASL(_a); _mORA(_a); PC += 2; fr = 0; } break;
	default: break;
	}
}

void _SRE(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: break;
	case 4: break;
	case 5: if		(mode == ADDR_MODE::ZEROPAGE)	{ uint16_t _a = getZeropage(PC); _mLSR(_a); _mEOR(_a); PC += 2; fr = 0; } break;
	case 6: if		(mode == ADDR_MODE::ABSOLUT)	{ uint16_t _a = getAbsolute(PC); _mLSR(_a); _mEOR(_a); PC += 2;	fr = 0; }
			else if (mode == ADDR_MODE::ZEROPAGE_X) { uint16_t _a = getZeropageXIndex(PC, registers.X);	_mLSR(_a); _mEOR(_a); PC += 2; fr = 0; } break;
	case 7: if		(mode == ADDR_MODE::ABSOLUT_X)	{ uint16_t _a = getAbsoluteXIndex(PC, registers.X); _mLSR(_a); _mEOR(_a); PC += 2; fr = 0; } 
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{ uint16_t _a = getAbsoluteYIndex(PC, registers.Y);	_mLSR(_a); _mEOR(_a); PC += 2; fr = 0; } break;
	case 8: if		(mode == ADDR_MODE::INDIRECT_X) { uint16_t _a = getIndirectXIndex(PC, registers.X);	_mLSR(_a); _mEOR(_a); PC += 2; fr = 0; } 
			else if (mode == ADDR_MODE::INDIRECT_Y) { uint16_t _a = getIndirectYIndex(PC, registers.Y);	_mLSR(_a); _mEOR(_a); PC += 2; fr = 0; } break;
	default: break;
	}
}

void _RLA(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: break;
	case 4: break;
	case 5: if		(mode == ADDR_MODE::ZEROPAGE)	{ uint16_t _a = getZeropage(PC); _mROL(_a); _mAND(_a); PC += 2; fr = 0; } break;
	case 6: if		(mode == ADDR_MODE::ABSOLUT)	{ uint16_t _a = getAbsolute(PC); _mROL(_a); _mAND(_a); PC += 2; fr = 0;	}
			else if (mode == ADDR_MODE::ZEROPAGE_X) { uint16_t _a = getZeropageXIndex(PC, registers.X);	_mROL(_a); _mAND(_a); PC += 2; fr = 0; } break;
	case 7: if		(mode == ADDR_MODE::ABSOLUT_X)	{ uint16_t _a = getAbsoluteXIndex(PC, registers.X); _mROL(_a); _mAND(_a); PC += 2; fr = 0; } 
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{ uint16_t _a = getAbsoluteYIndex(PC, registers.Y); _mROL(_a); _mAND(_a); PC += 2; fr = 0; } break;
	case 8: if		(mode == ADDR_MODE::INDIRECT_X) { uint16_t _a = getIndirectXIndex(PC, registers.X); _mROL(_a); _mAND(_a); PC += 2; fr = 0; }
			else if (mode == ADDR_MODE::INDIRECT_Y) { uint16_t _a = getIndirectYIndex(PC, registers.Y);	_mROL(_a); _mAND(_a); PC += 2; fr = 0; } break;
	default: break;
	}
}

void _RRA(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: break;
	case 4: break;
	case 5: if		(mode == ADDR_MODE::ZEROPAGE)	{ uint16_t _a = getZeropage(PC); _mROR(_a); _mADD(readFromMem(_a)); PC += 2; fr = 0; } break;
	case 6: if		(mode == ADDR_MODE::ABSOLUT)	{ uint16_t _a = getAbsolute(PC); _mROR(_a);	_mADD(readFromMem(_a));	PC += 2; fr = 0; }
			else if (mode == ADDR_MODE::ZEROPAGE_X) { uint16_t _a = getZeropageXIndex(PC, registers.X); _mROR(_a); _mADD(readFromMem(_a)); PC += 2;	fr = 0; } break;
	case 7: if		(mode == ADDR_MODE::ABSOLUT_X)	{ uint16_t _a = getAbsoluteXIndex(PC, registers.X);	_mROR(_a); _mADD(readFromMem(_a)); PC += 2;	fr = 0; } 
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{ uint16_t _a = getAbsoluteYIndex(PC, registers.Y); _mROR(_a); _mADD(readFromMem(_a)); PC += 2;	fr = 0; } break;
	case 8: if		(mode == ADDR_MODE::INDIRECT_X) { uint16_t _a = getIndirectXIndex(PC, registers.X); _mROR(_a); _mADD(readFromMem(_a)); PC += 2;	fr = 0; }
			else if (mode == ADDR_MODE::INDIRECT_Y) { uint16_t _a = getIndirectYIndex(PC, registers.Y);	_mROR(_a); _mADD(readFromMem(_a)); PC += 2; fr = 0; } break;
	default: break;
	}
}

void _ISC(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: break;
	case 4: break;
	case 5: if		(mode == ADDR_MODE::ZEROPAGE)	{ uint16_t _a = getZeropage(PC); writeToMem(_a, readFromMem(_a) + 1); _mADD(~readFromMem(_a)); PC += 2; fr = 0; } break;
	case 6: if		(mode == ADDR_MODE::ABSOLUT)	{ uint16_t _a = getAbsolute(PC); writeToMem(_a, readFromMem(_a) + 1); _mADD(~readFromMem(_a)); PC += 2; fr = 0;	}
			else if (mode == ADDR_MODE::ZEROPAGE_X) { uint16_t _a = getZeropageXIndex(PC, registers.X); writeToMem(_a, readFromMem(_a) + 1); _mADD(~readFromMem(_a)); PC += 2; fr = 0; } break;
	case 7: if		(mode == ADDR_MODE::ABSOLUT_X)	{ uint16_t _a = getAbsoluteXIndex(PC, registers.X); writeToMem(_a, readFromMem(_a) + 1); _mADD(~readFromMem(_a)); PC += 2; fr = 0; } 
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{ uint16_t _a = getAbsoluteYIndex(PC, registers.Y); writeToMem(_a, readFromMem(_a) + 1); _mADD(~readFromMem(_a)); PC += 2; fr = 0; } break;
	case 8: if		(mode == ADDR_MODE::INDIRECT_X) { uint16_t _a = getIndirectXIndex(PC, registers.X); writeToMem(_a, readFromMem(_a) + 1); _mADD(~readFromMem(_a)); PC += 2; fr = 0; }
			else if (mode == ADDR_MODE::INDIRECT_Y) { uint16_t _a = getIndirectYIndex(PC, registers.Y);	writeToMem(_a, readFromMem(_a) + 1); _mADD(~readFromMem(_a)); PC += 2; fr = 0; } break;
	default: break;
	}
}

void _DCP(uint8_t state, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: break;
	case 4: break;
	case 5: if		(mode == ADDR_MODE::ZEROPAGE)	{ uint16_t _a = getZeropage(PC); writeToMem(_a, readFromMem(_a) - 1); _mCMP(registers.A, _a); PC += 2; fr = 0; } break;
	case 6: if		(mode == ADDR_MODE::ABSOLUT)	{ uint16_t _a = getAbsolute(PC); writeToMem(_a, readFromMem(_a) - 1); _mCMP(registers.A, _a); PC += 2; fr = 0; }
			else if (mode == ADDR_MODE::ZEROPAGE_X) { uint16_t _a = getZeropageXIndex(PC, registers.X);	writeToMem(_a, readFromMem(_a) - 1); _mCMP(registers.A, _a); PC += 2; fr = 0; } break;
	case 7: if		(mode == ADDR_MODE::ABSOLUT_X)	{ uint16_t _a = getAbsoluteXIndex(PC, registers.X); writeToMem(_a, readFromMem(_a) - 1); _mCMP(registers.A, _a); PC += 2; fr = 0; } 
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{ uint16_t _a = getAbsoluteYIndex(PC, registers.Y); writeToMem(_a, readFromMem(_a) - 1); _mCMP(registers.A, _a); PC += 2; fr = 0; }	break;
	case 8: if		(mode == ADDR_MODE::INDIRECT_X) { uint16_t _a = getIndirectXIndex(PC, registers.X); writeToMem(_a, readFromMem(_a) - 1); _mCMP(registers.A, _a); PC += 2; fr = 0; }
			else if (mode == ADDR_MODE::INDIRECT_Y) { uint16_t _a = getIndirectYIndex(PC, registers.Y); writeToMem(_a, readFromMem(_a) - 1); _mCMP(registers.A, _a); PC += 2; fr = 0; } break;
	default: break;
	}
}

void _mSTX(uint8_t val, uint16_t adr) {
	writeToMem(adr, val);
}

void _STX(uint8_t state, uint8_t val, ADDR_MODE mode) {
	switch (state)
	{
	case 1:	break;
	case 2:	break;
	case 3: if		(mode == ADDR_MODE::ZEROPAGE)	{ _mSTX(val, getZeropage(PC)); PC += 2; fr = 0; } break;
	case 4: if		(mode == ADDR_MODE::ABSOLUT)	{ _mSTX(val, getAbsolute(PC)); PC += 3; fr = 0;	}
			else if (mode == ADDR_MODE::ZEROPAGE_X) { _mSTX(val, getZeropageXIndex(PC, registers.X)); PC += 2; fr = 0; }
			else if (mode == ADDR_MODE::ZEROPAGE_Y) { _mSTX(val, getZeropageYIndex(PC, registers.Y)); PC += 2; fr = 0; } break;
	case 5: if		(mode == ADDR_MODE::ABSOLUT_X)	{ _mSTX(val, getAbsoluteXIndex(PC, registers.X)); PC += 3; fr = 0; } 
			else if (mode == ADDR_MODE::ABSOLUT_Y)	{ _mSTX(val, getAbsoluteYIndex(PC, registers.Y)); PC += 3; fr = 0; } break;
	case 6: if		(mode == ADDR_MODE::INDIRECT_X)	{ _mSTX(val, getIndirectXIndex(PC, registers.X)); PC += 2; fr = 0; }
			else if (mode == ADDR_MODE::INDIRECT_Y) { _mSTX(val, getIndirectYIndex(PC, registers.Y)); PC += 2; fr = 0; } break;
	default: break;
	}
}

void _DEX(uint8_t state, ADDR_MODE mode) {
	
	switch (state)
	{
	case 1:	break;
	case 2:	if (mode == ADDR_MODE::IMMEDIATE) { registers.X--; status.setZero(registers.X == 0); status.setNegative(registers.X >> 7); fr = 0; PC++; } break;
	default: break;
	}
}

void _BCC(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: { if (status.carry) { PC += 2; fr = 0; } } break;
	case 3: { if ((((PC + 2) + (int8_t)readFromMem(PC + 1)) & 0xff00) == ((PC + 2) & 0xff00)) { PC += 2 + (int8_t)readFromMem(PC + 1); fr = 0; } } break;
	case 4: { PC += 2 + (int8_t)readFromMem(PC + 1); fr = 0; } break;
	default: break;
	}
}

void _BCS(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: { if (!status.carry) { PC += 2;	fr = 0;	} } break;
	case 3: { if ((((PC + 2) + (int8_t)readFromMem(PC + 1)) & 0xff00) == ((PC + 2) & 0xff00)) { PC += 2 + (int8_t)readFromMem(PC + 1);	fr = 0;	} } break;
	case 4: { PC += 2 + (int8_t)readFromMem(PC + 1); fr = 0; } break;
	default: break;
	}
}

void _BEQ(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: { if (!status.zero) { PC += 2; fr = 0; } } break;
	case 3: { if ((((PC + 2) + (int8_t)readFromMem(PC + 1)) & 0xff00) == ((PC + 2) & 0xff00)) 
		{ 
			PC += 2 + (int8_t)readFromMem(PC + 1); 
			fr = 0;	
		} 
	} break;
	case 4: { PC += 2 + (int8_t)readFromMem(PC + 1); fr = 0; } break;
	default: break;
	}
}

void _BNE(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: { if (status.zero) { PC += 2; fr = 0; } } break;
	case 3: { if ((((PC + 2) + (int8_t)readFromMem(PC + 1)) & 0xff00) == ((PC + 2) & 0xff00)) { PC += 2 + (int8_t)readFromMem(PC + 1); fr = 0;	} } break;
	case 4: { PC += 2 + (int8_t)readFromMem(PC + 1); fr = 0; } break;
	default: break;
	}
}

void _BPL(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: { if (status.negative != 0) { PC += 2; fr = 0; } } break;
	case 3: { if ((((PC + 2) + (int8_t)readFromMem(PC + 1)) & 0xff00) == ((PC + 2) & 0xff00)) { PC += 2 + (int8_t)readFromMem(PC + 1);	fr = 0;	} } break;
	case 4: { PC += 2 + (int8_t)readFromMem(PC + 1); fr = 0; } break;
	default: break;
	}
}

void _BMI(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: { if (status.negative == 0) { PC += 2; fr = 0; } } break;
	case 3: { if ((((PC + 2) + (int8_t)readFromMem(PC + 1)) & 0xff00) == ((PC + 2) & 0xff00)) { PC += 2 + (int8_t)readFromMem(PC + 1); fr = 0;	} } break;
	case 4: { PC += 2 + (int8_t)readFromMem(PC + 1); fr = 0; } break;
	default: break;
	}
}

void _BVC(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: { if (status.overflow != 0) { PC += 2; fr = 0; } } break;
	case 3: { if ((((PC + 2) + (int8_t)readFromMem(PC + 1)) & 0xff00) == ((PC + 2) & 0xff00)) { PC += 2 + (int8_t)readFromMem(PC + 1);	fr = 0;	} } break;
	case 4: { PC += 2 + (int8_t)readFromMem(PC + 1); fr = 0; } break;
	default: break;
	}
}

void _BVS(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: { if (status.overflow == 0) { PC += 2; fr = 0; } } break;
	case 3: { if ((((PC + 2) + (int8_t)readFromMem(PC + 1)) & 0xff00) == ((PC + 2) & 0xff00)) { PC += 2 + (int8_t)readFromMem(PC + 1); fr = 0; } } break;
	case 4: { PC += 2 + (int8_t)readFromMem(PC + 1); fr = 0; } break;
	default: break;
	}
}

void _SEC(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: status.setCarry(1); fr = 0; PC++; break;
	default: break;
	}
}

void _CLC(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: status.setCarry(0); fr = 0; PC++; break;
	default: break;
	}
}

void _SEI(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: status.setInterruptDisable(1); fr = 0; PC++; break;
	default: break;
	}
}

void _CLI(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: status.setInterruptDisable(0); fr = 0; PC++; break;
	default: break;
	}
}

void _CLV(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: status.setOverflow(0); fr = 0; PC++; break;
	default: break;
	}
}

void _SED(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: status.setDecimal(1); fr = 0; PC++; break;
	default: break;
	}
}

void _CLD(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: status.setDecimal(0); fr = 0; PC++; break;
	default: break;
	}
}

void _TAY(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: { registers.Y = registers.A; status.setZero(registers.A == 0); status.setNegative(registers.A >> 7); fr = 0; PC++; break; } 
	default: break;
	}
}

void _TAX(uint8_t state) {
	
	switch (state)
	{
	case 1:	break;
	case 2: { registers.X = registers.A; status.setZero(registers.A == 0); status.setNegative(registers.A >> 7); fr = 0; PC++; break; }
	default: break;
	}
}

void _TXA(uint8_t state) {

	switch (state)
	{
	case 1:	break;
	case 2: { registers.A = registers.X; status.setZero(registers.A == 0); status.setNegative(registers.A >> 7); fr = 0; PC++; break; }
	default: break;
	}
}

void _TYA(uint8_t state) {

	switch (state)
	{
	case 1:	break;
	case 2: { registers.A = registers.Y; status.setZero(registers.A == 0); status.setNegative(registers.A >> 7); fr = 0; PC++; break; }
	default: break;
	}
}

void _TSX(uint8_t state) {

	switch (state)
	{
	case 1:	break;
	case 2: { registers.X = SP_; status.setZero(registers.X == 0); status.setNegative(registers.X >> 7); fr = 0; PC++; break; }
	default: break;
	}
}

void _TXS(uint8_t state) {

	switch (state)
	{
	case 1:	break;
	case 2: { SP_ = registers.X; fr = 0; PC++; break; }
	default: break;
	}
}

void _INX(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: { registers.X++; status.setZero(registers.X == 0); status.setNegative(registers.X >> 7); fr = 0; PC++; break; }
	default: break;
	}
}

void _INY(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: { registers.Y++; status.setZero(registers.Y == 0); status.setNegative(registers.Y >> 7); fr = 0; PC++; break; }
	default: break;
	}
}

void _DEY(uint8_t state) {
	switch (state)
	{
	case 1:	break;
	case 2: { registers.Y--; status.setZero(registers.Y == 0); status.setNegative(registers.Y >> 7); fr = 0; PC++; break; }
	default: break;
	}
}


/*
		STATE MACHINE REWRITE OPCODES | END
*/


void resetCPU() {
	resetMMU();
	PC = readFromMem(0xfffd) << 8 | readFromMem(0xfffc);
	printf("Reset CPU, starting at PC: %x\n", PC);
}

void setNMI(bool v) {
	nmi = v;
}

void NMI(uint8_t state) {
	switch (state) {
		case 1: break;
		case 2: break;
		case 3: break;
		case 4: break;
		case 5: break;
		case 6: break;
		case 7: {
			printf("NMI EXECUTING\n");
			writeToMem(SP_ + 0x100, PC >> 8);
			SP_--;
			writeToMem(SP_ + 0x100, PC & 0xff);
			SP_--;
			writeToMem(SP_ + 0x100, status.status);
			SP_--;
			PC = (readFromMem(0xfffb) << 8) | readFromMem(0xfffa);
			status.setInterruptDisable(1);
			nmi_running = false;
			nmi = false;
			fr = 0;
		} break;
		default: break;
	}
}

void setIRQ(bool v) {
	irq = v;
}
bool tmpgo = false;
void IRQorBRK(uint8_t state) {
	if (tmpgo)
		printf("IRQ Cycle %d\n", state);
	switch (state) {
		case 1: break;
		case 2: break;
		case 3: break;
		case 4: break;
		case 5: break;
		case 6: break;
		case 7: {
			if (tmpgo)
				printf("IRQ Last Cycle\n");
			writeToMem(SP_ + 0x100, PC >> 8);
			SP_--;
			writeToMem(SP_ + 0x100, PC & 0xff);
			SP_--;
			writeToMem(SP_ + 0x100, status.status);
			SP_--;
			PC = (readFromMem(0xffff) << 8) | readFromMem(0xfffe);
			status.setInterruptDisable(1);
			irq_running = false;
			irq = false;
			fr = 0;
		} break;
		default: break;
	}
}

Registers getCPURegs() {
	return registers;
}

void printLog() {
	printf("%04x $%02x $%02x $%02x A:%02x X:%02x Y:%02x P:%02x SP:%02x \n", PC, readFromMem(PC), readFromMem(PC + 1), readFromMem(PC + 2), registers.A, registers.X, registers.Y, status.status, SP_);
}

bool logNow = false;
void setLog(bool v) {
	logNow = v;
}

bool pendingIRQ() {
	return irq;
}


uint8_t CPU_executeInstruction() {
	
	//	Check interrupt hijacking (NMI hijacking IRQ)
	if (fr == 1 && nmi == true && nmi_running == false) {
		nmi_running = true;
	}
	if (fr == 1 && irq == true && irq_running == false && status.interruptDisable == 0) {
		irq_running = true;
	}
	if (nmi == true && nmi_running == true) {
		NMI(fr);
		fr++;
		return 1;
	}
	if (irq == true && irq_running == true && status.interruptDisable == 0) {
		IRQorBRK(fr);
		fr++;
		return 1;
	}

	if (PC == 0x0c00)
		tmpgo = true;
	if (tmpgo && 0)
		printf("%04x $%02x $%02x $%02x A:%02x X:%02x Y:%02x - VIC_Y:%03d(%02x) VIC_X:%03d(%02x) CycNo:%03d(%02x) - P:%02x SP:%02x - D012/11: 0x%04x \n", PC, readFromMem(PC), readFromMem(PC + 1), readFromMem(PC + 2), registers.A, registers.X, registers.Y, currentScanline(), currentScanline(), currentPixel(), currentPixel(), currentCycle(), currentCycle(), status.status, SP_, (readVICregister(0xd011) << 8) | readVICregister(0xd012));

	switch (readFromMem(PC)) {

	case 0x00: { status.setBrk(1); irq = true; printf("BREAK "); return 7; break; }
	case 0x01: _ORA(fr, ADDR_MODE::INDIRECT_X); break;
	case 0x03: _SLO(fr, ADDR_MODE::INDIRECT_X); break;
	case 0x05: _ORA(fr, ADDR_MODE::ZEROPAGE); break;
	case 0x06: _ASL(fr, ADDR_MODE::ZEROPAGE); break;
	case 0x07: _SLO(fr, ADDR_MODE::ZEROPAGE); break;
	case 0x08: _PHP(fr); break;
	case 0x09: _ORA(fr, ADDR_MODE::IMMEDIATE); break;
	case 0x0a: _ASLA(fr); break;
	case 0x0d: _ORA(fr, ADDR_MODE::ABSOLUT);  break;
	case 0x0e: _ASL(fr, ADDR_MODE::ABSOLUT); break;
	case 0x0f: _SLO(fr, ADDR_MODE::ABSOLUT); break;
	case 0x10: _BPL(fr); break;
	case 0x11: _ORA(fr, ADDR_MODE::INDIRECT_Y); break;
	case 0x13: _SLO(fr, ADDR_MODE::INDIRECT_Y); break;
	case 0x15: _ORA(fr, ADDR_MODE::ZEROPAGE_X); break;
	case 0x16: _ASL(fr, ADDR_MODE::ZEROPAGE_X); break;
	case 0x17: _SLO(fr, ADDR_MODE::ZEROPAGE_X); break;
	case 0x18: _CLC(fr); break;
	case 0x19: _ORA(fr, ADDR_MODE::ABSOLUT_Y); break;
	case 0x1b: _SLO(fr, ADDR_MODE::ABSOLUT_Y); break;
	case 0x1d: _ORA(fr, ADDR_MODE::ABSOLUT_X); break;
	case 0x1e: _ASL(fr, ADDR_MODE::ABSOLUT_X); break;
	case 0x1f: _SLO(fr, ADDR_MODE::ABSOLUT_X); break;
	case 0x20: _JSR(fr); break;
	case 0x21: _AND(fr, ADDR_MODE::INDIRECT_X); break;
	case 0x23: _RLA(fr, ADDR_MODE::INDIRECT_X); break;
	case 0x24: _BIT(fr, ADDR_MODE::ZEROPAGE); break;
	case 0x25: _AND(fr, ADDR_MODE::ZEROPAGE); break;
	case 0x26: _ROL(fr, ADDR_MODE::ZEROPAGE); break;
	case 0x27: _RLA(fr, ADDR_MODE::ZEROPAGE); break;
	case 0x28: _PLP(fr); break;
	case 0x29: _AND(fr, ADDR_MODE::IMMEDIATE); break;
	case 0x2a: _ROLA(fr); break;
	case 0x2c: _BIT(fr, ADDR_MODE::ABSOLUT); break;
	case 0x2d: _AND(fr, ADDR_MODE::ABSOLUT); break;
	case 0x2e: _ROL(fr, ADDR_MODE::ABSOLUT); break;
	case 0x2f: _RLA(fr, ADDR_MODE::ABSOLUT); break;
	case 0x30: _BMI(fr); break;
	case 0x31: _AND(fr, ADDR_MODE::INDIRECT_Y); break;
	case 0x33: _RLA(fr, ADDR_MODE::INDIRECT_Y); break;
	case 0x35: _AND(fr, ADDR_MODE::ZEROPAGE_X); break;
	case 0x36: _ROL(fr, ADDR_MODE::ZEROPAGE_X); break;
	case 0x37: _RLA(fr, ADDR_MODE::ZEROPAGE_X); break;
	case 0x38: _SEC(fr); break;
	case 0x39: _AND(fr, ADDR_MODE::ABSOLUT_Y); break;
	case 0x3b: _RLA(fr, ADDR_MODE::ABSOLUT_Y); break;
	case 0x3d: _AND(fr, ADDR_MODE::ABSOLUT_X); break;
	case 0x3e: _ROL(fr, ADDR_MODE::ABSOLUT_X); break;
	case 0x3f: _RLA(fr, ADDR_MODE::ABSOLUT_X); break;
	case 0x40: _RTI(fr); break;
	case 0x41: _EOR(fr, ADDR_MODE::INDIRECT_X); break;
	case 0x43: _SRE(fr, ADDR_MODE::INDIRECT_X); break;
	case 0x45: _EOR(fr, ADDR_MODE::ZEROPAGE); break;
	case 0x46: _LSR(fr, ADDR_MODE::ZEROPAGE); break;
	case 0x47: _SRE(fr, ADDR_MODE::ZEROPAGE); break;
	case 0x48: _PHA(fr); break;
	case 0x49: _EOR(fr, ADDR_MODE::IMMEDIATE); break;
	case 0x4a: _LSRA(fr); break;
	case 0x4c: _JMP(fr, ADDR_MODE::ABSOLUT); break;
	case 0x4d: _EOR(fr, ADDR_MODE::ABSOLUT); break;
	case 0x4e: _LSR(fr, ADDR_MODE::ABSOLUT); break;
	case 0x4f: _SRE(fr, ADDR_MODE::ABSOLUT); break;
	case 0x50: _BVC(fr); break;
	case 0x51: _EOR(fr, ADDR_MODE::INDIRECT_Y); break;
	case 0x53: _SRE(fr, ADDR_MODE::INDIRECT_Y); break;
	case 0x55: _EOR(fr, ADDR_MODE::ZEROPAGE_X); break;
	case 0x56: _LSR(fr, ADDR_MODE::ZEROPAGE_X); break;
	case 0x57: _SRE(fr, ADDR_MODE::ZEROPAGE_X); break;
	case 0x58: _CLI(fr); break;
	case 0x59: _EOR(fr, ADDR_MODE::ABSOLUT_Y); break;
	case 0x5b: _SRE(fr, ADDR_MODE::ABSOLUT_Y); break;
	case 0x5d: _EOR(fr, ADDR_MODE::ABSOLUT_X); break;
	case 0x5e: _LSR(fr, ADDR_MODE::ABSOLUT_X); break;
	case 0x5f: _SRE(fr, ADDR_MODE::ABSOLUT_X); break;
	case 0x60: _RTS(fr); break;
	case 0x61: _ADC(fr, ADDR_MODE::INDIRECT_X); break;
	case 0x63: _RRA(fr, ADDR_MODE::INDIRECT_X); break;
	case 0x65: _ADC(fr, ADDR_MODE::ZEROPAGE); break;
	case 0x66: _ROR(fr, ADDR_MODE::ZEROPAGE); break;
	case 0x67: _RRA(fr, ADDR_MODE::ZEROPAGE); break;
	case 0x68: _PLA(fr); break;
	case 0x69: _ADC(fr, ADDR_MODE::IMMEDIATE); break;
	case 0x6a: _RORA(fr); break;
	case 0x6c: _JMP(fr, ADDR_MODE::INDIRECT); break;
	case 0x6d: _ADC(fr, ADDR_MODE::ABSOLUT); break;
	case 0x6e: _ROR(fr, ADDR_MODE::ABSOLUT); break;
	case 0x6f: _RRA(fr, ADDR_MODE::ABSOLUT); break;
	case 0x70: _BVS(fr); break;
	case 0x71: _ADC(fr, ADDR_MODE::INDIRECT_Y); break;
	case 0x73: _RRA(fr, ADDR_MODE::INDIRECT_Y); break;
	case 0x75: _ADC(fr, ADDR_MODE::ZEROPAGE_X); break;
	case 0x76: _ROR(fr, ADDR_MODE::ZEROPAGE_X); break;
	case 0x77: _RRA(fr, ADDR_MODE::ZEROPAGE_X); break;
	case 0x78: _SEI(fr); break;
	case 0x79: _ADC(fr, ADDR_MODE::ABSOLUT_Y); break;
	case 0x7b: _RRA(fr, ADDR_MODE::ABSOLUT_Y); break;
	case 0x7d: _ADC(fr, ADDR_MODE::ABSOLUT_X); break;
	case 0x7e: _ROR(fr, ADDR_MODE::ABSOLUT_X); break;
	case 0x7f: _RRA(fr, ADDR_MODE::ABSOLUT_X); break;
	case 0x81: _STX(fr, registers.A, ADDR_MODE::INDIRECT_X); break;
	case 0x83: _STX(fr, registers.A & registers.X, ADDR_MODE::INDIRECT_X); break;
	case 0x84: _STX(fr, registers.Y, ADDR_MODE::ZEROPAGE); break;
	case 0x85: _STX(fr, registers.A, ADDR_MODE::ZEROPAGE); break;
	case 0x86: _STX(fr, registers.X, ADDR_MODE::ZEROPAGE); break;
	case 0x87: _STX(fr, registers.A & registers.X, ADDR_MODE::ZEROPAGE); break;
	case 0x88: _DEY(fr); break;
	case 0x8a: _TXA(fr); break;
	case 0x8c: _STX(fr, registers.Y, ADDR_MODE::ABSOLUT); break;
	case 0x8d: _STX(fr, registers.A, ADDR_MODE::ABSOLUT); break;
	case 0x8e: _STX(fr, registers.X, ADDR_MODE::ABSOLUT); break;
	case 0x8f: _STX(fr, registers.X & registers.A, ADDR_MODE::ABSOLUT); break;
	case 0x90: _BCC(fr); break;
	case 0x91: _STX(fr, registers.A, ADDR_MODE::INDIRECT_Y); break;
	case 0x94: _STX(fr, registers.Y, ADDR_MODE::ZEROPAGE_X); break;
	case 0x95: _STX(fr, registers.A, ADDR_MODE::ZEROPAGE_X); break;
	case 0x96: _STX(fr, registers.X, ADDR_MODE::ZEROPAGE_Y); break;
	case 0x98: _TYA(fr); break;
	case 0x99: _STX(fr, registers.A, ADDR_MODE::ABSOLUT_Y); break;
	case 0x9a: _TXS(fr); break;
	case 0x9d: _STX(fr, registers.A, ADDR_MODE::ABSOLUT_X); break;
	case 0xa0: _LDX(fr, registers.Y, ADDR_MODE::IMMEDIATE); break;
	case 0xa1: _LDX(fr, registers.A, ADDR_MODE::INDIRECT_X); break;
	case 0xa2: _LDX(fr, registers.X, ADDR_MODE::IMMEDIATE); break;
	case 0xa3: _LAX(fr, ADDR_MODE::INDIRECT_X); break;
	case 0xa4: _LDX(fr, registers.Y, ADDR_MODE::ZEROPAGE); break;
	case 0xa5: _LDX(fr, registers.A, ADDR_MODE::ZEROPAGE); break;
	case 0xa6: _LDX(fr, registers.X, ADDR_MODE::ZEROPAGE); break;
	case 0xa7: _LAX(fr, ADDR_MODE::ZEROPAGE); break;
	case 0xa8: _TAY(fr); break;
	case 0xa9: _LDX(fr, registers.A, ADDR_MODE::IMMEDIATE); break;
	case 0xaa: _TAX(fr); break;
	case 0xac: _LDX(fr, registers.Y, ADDR_MODE::ABSOLUT); break;
	case 0xad: _LDX(fr, registers.A, ADDR_MODE::ABSOLUT); break;
	case 0xae: _LDX(fr, registers.X, ADDR_MODE::ABSOLUT); break;
	case 0xaf: _LAX(fr, ADDR_MODE::ABSOLUT); break;
	case 0xb0: _BCS(fr);  break;
	case 0xb1: _LDX(fr, registers.A, ADDR_MODE::INDIRECT_Y); break;
	case 0xb4: _LDX(fr, registers.Y, ADDR_MODE::ZEROPAGE_X); break;
	case 0xb5: _LDX(fr, registers.A, ADDR_MODE::ZEROPAGE_X); break;
	case 0xb6: _LDX(fr, registers.X, ADDR_MODE::ZEROPAGE_Y); break;
	case 0xb7: _LAX(fr, ADDR_MODE::ZEROPAGE_Y); break;
	case 0xb8: _CLV(fr); break;
	case 0xb9: _LDX(fr, registers.A, ADDR_MODE::ABSOLUT_Y); break;
	case 0xba: _TSX(fr); break;
	case 0xbc: _LDX(fr, registers.Y, ADDR_MODE::ABSOLUT_X); break;
	case 0xbd: _LDX(fr, registers.A, ADDR_MODE::ABSOLUT_X); break;
	case 0xbe: _LDX(fr, registers.X, ADDR_MODE::ABSOLUT_Y); break;
	case 0xbf: _LAX(fr, ADDR_MODE::ABSOLUT_Y); break;
	case 0xc0: _CMP(fr, registers.Y, ADDR_MODE::IMMEDIATE); break;
	case 0xc1: _CMP(fr, registers.A, ADDR_MODE::INDIRECT_X); break;
	case 0xc3: _DCP(fr, ADDR_MODE::INDIRECT_X); break;
	case 0xc4: _CMP(fr, registers.Y, ADDR_MODE::ZEROPAGE); break;
	case 0xc5: _CMP(fr, registers.A, ADDR_MODE::ZEROPAGE); break;
	case 0xc6: _DEC(fr, ADDR_MODE::ZEROPAGE); break;
	case 0xc7: _DCP(fr, ADDR_MODE::ZEROPAGE); break;
	case 0xc8: _INY(fr); break;
	case 0xc9: _CMP(fr, registers.A, ADDR_MODE::IMMEDIATE); break;
	case 0xca: _DEX(fr, ADDR_MODE::IMMEDIATE); break;
	case 0xcc: _CMP(fr, registers.Y, ADDR_MODE::ABSOLUT); break;
	case 0xcd: _CMP(fr, registers.A, ADDR_MODE::ABSOLUT); break;
	case 0xce: _DEC(fr, ADDR_MODE::ABSOLUT); break;
	case 0xcf: _DCP(fr, ADDR_MODE::ABSOLUT); break;
	case 0xd0: _BNE(fr); break;
	case 0xd1: _CMP(fr, registers.A, ADDR_MODE::INDIRECT_Y); break;
	case 0xd3: _DCP(fr, ADDR_MODE::INDIRECT_Y); break;
	case 0xd5: _CMP(fr, registers.A, ADDR_MODE::ZEROPAGE_X); break;
	case 0xd6: _DEC(fr, ADDR_MODE::ZEROPAGE_X); break;
	case 0xd7: _DCP(fr, ADDR_MODE::ZEROPAGE_X); break;
	case 0xd8: _CLD(fr); break;
	case 0xd9: _CMP(fr, registers.A, ADDR_MODE::ABSOLUT_Y); break;
	case 0xdb: _DCP(fr, ADDR_MODE::ABSOLUT_Y); break;
	case 0xdd: _CMP(fr, registers.A, ADDR_MODE::ABSOLUT_X); break;
	case 0xde: _DEC(fr, ADDR_MODE::ABSOLUT_X); break;
	case 0xdf: _DCP(fr, ADDR_MODE::ABSOLUT_X); break;
	case 0xe0: _CMP(fr, registers.X, ADDR_MODE::IMMEDIATE); break;
	case 0xe1: _SBC(fr, ADDR_MODE::INDIRECT_X); break;
	case 0xe3: _ISC(fr, ADDR_MODE::INDIRECT_X); break;
	case 0xe4: _CMP(fr, registers.X, ADDR_MODE::ZEROPAGE); break;
	case 0xe5: _SBC(fr, ADDR_MODE::ZEROPAGE); break;
	case 0xe6: _INC(fr, ADDR_MODE::ZEROPAGE); break;
	case 0xe7: _ISC(fr, ADDR_MODE::ZEROPAGE); break;
	case 0xe8: _INX(fr); break;
	case 0xe9: _SBC(fr, ADDR_MODE::IMMEDIATE); break;
	case 0xea: _NOP(fr);
	case 0xec: _CMP(fr, registers.X, ADDR_MODE::ABSOLUT); break;
	case 0xed: _SBC(fr, ADDR_MODE::ABSOLUT); break;
	case 0xee: _INC(fr, ADDR_MODE::ABSOLUT); break;
	case 0xef: _ISC(fr, ADDR_MODE::ABSOLUT); break;
	case 0xf0: _BEQ(fr); break;
	case 0xf1: _SBC(fr, ADDR_MODE::INDIRECT_Y); break;
	case 0xf3: _ISC(fr, ADDR_MODE::INDIRECT_Y); break;
	case 0xf5: _SBC(fr, ADDR_MODE::ZEROPAGE_X); break;
	case 0xf6: _INC(fr, ADDR_MODE::ZEROPAGE_X); break;
	case 0xf7: _ISC(fr, ADDR_MODE::ZEROPAGE_X); break;
	case 0xf8: _SED(fr); break;
	case 0xf9: _SBC(fr, ADDR_MODE::ABSOLUT_Y); break;
	case 0xfb: _ISC(fr, ADDR_MODE::ABSOLUT_Y); break;
	case 0xfd: _SBC(fr, ADDR_MODE::ABSOLUT_X); break;
	case 0xfe: _INC(fr, ADDR_MODE::ABSOLUT_X); break;
	case 0xff: _ISC(fr, ADDR_MODE::ABSOLUT_X); break;

	default:
		printf("ERROR! Unimplemented opcode 0x%02x at address 0x%04x !\n", readFromMem(PC), PC);
		std::exit(1);
		break;
	}
	fr++;
	return 1;
}