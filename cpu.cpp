#pragma once
#include "cpu.h"
#include "mmu.h"
#include <map>

typedef uint8_t u8;
typedef uint16_t u16;
typedef int8_t i8;
typedef int16_t i16;

u16 PC_L = 0x0000, PC_H = 0x0000, VAL = 0x0000;
u16 PC = 0xc000;
u8 SP_ = 0xfd;
u8 SUB_CYC = 1;
u8 CURRENT_OPCODE = 0x00;
Registers registers;
Status status;

bool irq = false;
bool irq_running = false;
bool nmi = false;
bool nmi_running = false;

void setCarry() {				//	Only for hooking C64 LOAD routines for filechecking
	status.carry = 1;
}

void clearCarry() {				//	Only for hooking C64 LOAD routines for filechecking
	status.carry = 0;
}


/*
	CYCLE ACCURATE REWRITE - START
*/

//	HELPER
u16 getAdr() {
	return ((PC_H & 0xff) << 8) | (PC_L & 0xff);
}

/*
*	INSTRUCTIONS
*/

//	NOP FUNCTIONS
void NOP(u8 val) {
}

//	READ FUNCTIONS
void LDA(u8& tar, u8 val) {
	tar = val;
	status.setZero(val == 0);
	status.setNegative(val >> 7);
}
void LDA(u8 val) {
	LDA(registers.A, val);
}
void LDX(u8 val) {
	LDA(registers.X, val);
}
void LDY(u8 val) {
	LDA(registers.Y, val);
}
void EOR(u8 val) {
	registers.A ^= val;
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
}
void AND(u8 val) {
	registers.A &= val;
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
}
void ORA(u8 val) {
	registers.A |= val;
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
}
void ADD(u8 val) {
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
void ADC(u8 val) {
	ADD(val);
}
void SBC(u8 val) {
	ADD(~val);
}
void CMP(u8& tar, u8 val) {
	status.setCarry(tar >= val);
	status.setZero(val == tar);
	status.setNegative(((tar - val) & 0xff) >> 7);
}
void CMP(u8 val) {
	CMP(registers.A, val);
}
void CPX(u8 val) {
	CMP(registers.X, val);
}
void CPY(u8 val) {
	CMP(registers.Y, val);
}
void BIT(u8 val) {
	status.setNegative(val >> 7);
	status.setOverflow((val >> 6) & 1);
	status.setZero((registers.A & val) == 0);
}

//	RMW FUNCTIONS
u8 ASL(u8 val) {
	status.setCarry(val >> 7);
	u8 res = val << 1;
	status.setZero(res == 0);
	status.setNegative(res >> 7);
	return res;
}
u8 LSR(u8 val) {
	status.setCarry(val & 1);
	u8 res = val >> 1;
	status.setZero(res == 0);
	status.setNegative(res >> 7);
	return res;
}
u8 ROL(u8 val) {
	uint8_t tmp = status.carry;
	status.setCarry(val >> 7);
	u8 res = (val << 1) | tmp;
	status.setZero(res == 0);
	status.setNegative(res >> 7);
	return res;
}
u8 ROR(u8 val) {
	uint8_t tmp = val & 1;
	u8 res = (val >> 1) | (status.carry << 7);
	status.setCarry(tmp);
	status.setZero(res == 0);
	status.setNegative(res >> 7);
	return res;
}
u8 INC(u8 val) {
	u8 res = val + 1;
	status.setZero(res == 0);
	status.setNegative(res >> 7);
	return res;
}
u8 DEC(u8 val) {
	u8 res = val - 1;
	status.setZero(res == 0);
	status.setNegative(res >> 7);
	return res;
}

//	WRITE FUNCTIONS
u8 STA() {
	return registers.A;
}
u8 STX() {
	return registers.X;
}
u8 STY() {
	return registers.Y;
}

//	BRANCH FUNCTIONS
bool BCC() {
	return status.carry == 0;
}
bool BCS() {
	return status.carry == 1;
}
bool BEQ() {
	return status.zero == 1;
}
bool BNE() {
	return status.zero == 0;
}
bool BPL() {
	return status.negative == 0;
}
bool BMI() {
	return status.negative == 1;
}
bool BVC() {
	return status.overflow == 0;
}
bool BVS() {
	return status.overflow == 1;
}

//	IMPLICIT FUNCTIONS
void TAX() {
	registers.X = registers.A;
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
}
void TAY() {
	registers.Y = registers.A;
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
}
void TXA() {
	registers.A = registers.X;
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
}
void TYA() {
	registers.A = registers.Y;
	status.setZero(registers.A == 0);
	status.setNegative(registers.A >> 7);
}
void TSX() {
	registers.X = SP_;
	status.setZero(registers.X == 0);
	status.setNegative(registers.X >> 7);
}
void TXS() {
	SP_ = registers.X;
}
void INX() {
	registers.X++;
	status.setZero(registers.X == 0);
	status.setNegative(registers.X >> 7);
}
void INY() {
	registers.Y++;
	status.setZero(registers.Y == 0);
	status.setNegative(registers.Y >> 7);
}
void DEY() {
	registers.Y--;
	status.setZero(registers.Y == 0);
	status.setNegative(registers.Y >> 7);
}
void DEX() {
	registers.X--;
	status.setZero(registers.X == 0);
	status.setNegative(registers.X >> 7);
}
void SEC() {
	status.setCarry(1);
}
void CLC() {
	status.setCarry(0);
}
void SEI() {
	status.setInterruptDisable(1);
}
void CLI() {
	status.setInterruptDisable(0);
}
void CLV() {
	status.setOverflow(0);
}
void SED() {
	status.setDecimal(1);
}
void CLD() {
	status.setDecimal(0);
}

//	ILLEGAL / UNOFFICIAL FUNCTIONS

void LAX(u8 val) {
	LDA(val);
	LDX(val);
}
u8 SLO(u8 val) {
	u8 res = ASL(val);
	ORA(res);
	return res;
}
u8 RRA(u8 val) {
	u8 res = ROR(val);
	ADC(res);
	return res;
}
u8 RLA(u8 val) {
	u8 res = ROL(val);
	AND(res);
	return res;
}
u8 SAX() {
	return STA() & STX();
}
u8 SRE(u8 val) {
	u8 res = LSR(val);
	EOR(res);
	return res;
}
u8 DCP(u8 val) {
	u8 res = DEC(val);
	CMP(res);
	return res;
}
u8 ISC(u8 val) {
	u8 res = INC(val);
	SBC(res);
	return res;
}



/*
*	ADDRESSING
*/

//	ABSOLUTE ADDRESSING

void ABS_READ(void (*instruction)(u8)) {
	switch (SUB_CYC) {
	case 2: PC_L = getByte(PC++); break;																					//	READ
	case 3: PC_H = getByte(PC++); break;																					//	READ
	case 4: instruction(getByte(getAdr())); SUB_CYC = 0; break;																//	READ
	default: break;
	}
}

void ABS_RMW(u8(*instruction)(u8)) {
	switch (SUB_CYC)
	{
	case 2: PC_L = getByte(PC++); break;																					//	READ
	case 3: PC_H = getByte(PC++); break;																					//	READ
	case 4: VAL = getByte(getAdr()); break;																					//	READ
	case 5: writeByte(getAdr(), VAL); VAL = instruction(VAL); break;														//	WRITE
	case 6: writeByte(getAdr(), VAL); SUB_CYC = 0; break;																	//	WRITE
	default:
		break;
	}
}

void ABS_WRITE(u8(*instruction)()) {
	switch (SUB_CYC)
	{
	case 2: PC_L = getByte(PC++); break;																					//	READ
	case 3: PC_H = getByte(PC++); break;																					//	READ
	case 4: writeByte(getAdr(), instruction()); SUB_CYC = 0; break;															//	WRITE				
	default:
		break;
	}
}


//	ZEROPAGE ADDRESSING

void ZP_READ(void (*instruction)(u8)) {
	switch (SUB_CYC)
	{
	case 2: PC_L = getByte(PC++); break;																					//	READ
	case 3: instruction(getByte(PC_L)); SUB_CYC = 0; break;																	//	READ
	default:
		break;
	}
}

void ZP_RMW(u8(*instruction)(u8)) {
	switch (SUB_CYC)
	{
	case 2: PC_L = getByte(PC++); break;																					//	READ
	case 3: VAL = getByte(PC_L); break;																						//	READ
	case 4: writeByte(PC_L, VAL); VAL = instruction(VAL); break;															//	WRITE
	case 5: writeByte(PC_L, VAL); SUB_CYC = 0; break;																		//	WRITE
	default:
		break;
	}
}

void ZP_WRITE(u8(*instruction)()) {
	switch (SUB_CYC)
	{
	case 2: PC_L = getByte(PC++); break;																					//	READ
	case 3: writeByte(PC_L, instruction()); SUB_CYC = 0; break;																//	WRITE				
	default:
		break;
	}
}


//	ZEROPAGE INDEXED ADDRESSING

void ZP_IND_READ(void (*instruction)(u8), u8 index) {
	switch (SUB_CYC)
	{
	case 2: PC_L = getByte(PC++); break;																					//	READ
	case 3: VAL = getByte((PC_L + index) % 0x100); break;																	//	READ
	case 4: instruction(VAL); SUB_CYC = 0; break;																			//	READ
	default:
		break;
	}
}
void ZP_IND_X_READ(void (*instruction)(u8)) {
	ZP_IND_READ(instruction, registers.X);
}
void ZP_IND_Y_READ(void (*instruction)(u8)) {
	ZP_IND_READ(instruction, registers.Y);
}

void ZP_IND_RMW(u8(*instruction)(u8), u8 index) {
	switch (SUB_CYC)
	{
	case 2: PC_L = getByte(PC++); break;																					//	READ
	case 3: PC_L = (PC_L + index) % 0x100; break;																			//	READ
	case 4: VAL = getByte(PC_L); break;																						//	READ
	case 5: writeByte(PC_L, VAL); VAL = instruction(VAL); break;															//	WRITE
	case 6: writeByte(PC_L, VAL); SUB_CYC = 0; break;																		//	WRITE
	default:
		break;
	}
}
void ZP_IND_X_RMW(u8(*instruction)(u8)) {
	ZP_IND_RMW(instruction, registers.X);
}
void ZP_IND_Y_RMW(u8(*instruction)(u8)) {
	ZP_IND_RMW(instruction, registers.Y);
}

void ZP_IND_WRITE(u8(*instruction)(), u8 index) {
	switch (SUB_CYC)
	{
	case 2: PC_L = getByte(PC++); break;																					//	READ
	case 3: PC_L = (PC_L + index) % 0x100; break;																			//	READ
	case 4: writeByte(PC_L, instruction()); SUB_CYC = 0; break;																//	WRITE
	default:
		break;
	}
}
void ZP_IND_X_WRITE(u8(*instruction)()) {
	ZP_IND_WRITE(instruction, registers.X);
}
void ZP_IND_Y_WRITE(u8(*instruction)()) {
	ZP_IND_WRITE(instruction, registers.Y);
}


//	ABSOLUTE INDEXED ADDRESSING

void ABS_IND_READ(void (*instruction)(u8), u8 index) {
	switch (SUB_CYC)
	{
	case 2: PC_L = getByte(PC++); break;																					//	READ
	case 3: PC_H = getByte(PC++); PC_L += index; break;																		//	READ
	case 4: instruction(getByte(getAdr())); SUB_CYC *= PC_L > 0xff; PC_H += (PC_L >> 8); break;								//	READ
	case 5: instruction(getByte(getAdr())); SUB_CYC = 0; break;																//	READ | PAGEBREACH CYCLE
	default:
		break;
	}
}
void ABS_IND_X_READ(void (*instruction)(u8)) {
	ABS_IND_READ(instruction, registers.X);
}
void ABS_IND_Y_READ(void (*instruction)(u8)) {
	ABS_IND_READ(instruction, registers.Y);
}

void ABS_IND_RMW(u8(*instruction)(u8), u8 index) {
	switch (SUB_CYC)
	{
	case 2: PC_L = getByte(PC++); break;																					//	READ
	case 3: PC_H = getByte(PC++); PC_L += index; break;																		//	READ
	case 4: VAL = getByte(getAdr()); PC_H += (PC_L >> 8);  break;															//	READ
	case 5: VAL = getByte(getAdr()); break;																					//	READ
	case 6: writeByte(getAdr(), VAL); VAL = instruction(VAL); break;														//	WRITE
	case 7: writeByte(getAdr(), VAL); SUB_CYC = 0; break;																	//	WRITE
	default:
		break;
	}
}
void ABS_IND_X_RMW(u8(*instruction)(u8)) {
	ABS_IND_RMW(instruction, registers.X);
}
void ABS_IND_Y_RMW(u8(*instruction)(u8)) {
	ABS_IND_RMW(instruction, registers.Y);
}

void ABS_IND_WRITE(u8(*instruction)(), u8 index) {
	switch (SUB_CYC)
	{
	case 2: PC_L = getByte(PC++); break;																					//	READ
	case 3: PC_H = getByte(PC++); PC_L += index; break;																		//	READ
	case 4: VAL = getByte(getAdr()); PC_H += (PC_L >> 8);  break;															//	READ
	case 5: writeByte(getAdr(), instruction()); SUB_CYC = 0; break;															//	WRITE
	default:
		break;
	}
}
void ABS_IND_X_WRITE(u8(*instruction)()) {
	ABS_IND_WRITE(instruction, registers.X);
}
void ABS_IND_Y_WRITE(u8(*instruction)()) {
	ABS_IND_WRITE(instruction, registers.Y);
}


//	RELATIVE ADDRESSING

void REL_READ(bool (*instruction)()) {
	switch (SUB_CYC)
	{
	case 2:							 																						//	READ		
		VAL = getByte(PC++);
		getByte(PC);
		if (instruction()) {
			PC_L = PC + (i8)VAL;
			PC_H = (PC + (i8)VAL) >> 8;
		}
		else {
			SUB_CYC = 0;
		}
		break;
	case 3:																													//	READ
		getByte(PC);
		if ((PC_L & 0xff00) != (PC & 0xff00)) {																				//	FIX HIGH BYTE IF NECESSARY
			PC = PC_L;
		}
		else {
			PC = PC_L;
			SUB_CYC = 0;
		}
		break;
	case 4:																													//	READ | BRANCH TAKEN CYCLE
		getByte(PC); SUB_CYC = 0; break;
	default:
		break;
	}
}


//	INDEXED X ADDRESSING

void IND_X_READ(void (*instruction)(u8)) {
	switch (SUB_CYC)
	{
	case 2: VAL = getByte(PC++); break;																						//	READ
	case 3: VAL = VAL + registers.X; getByte(PC); break;																	//	READ
	case 4: PC_L = getByte(VAL % 0x100); break;																				//	READ
	case 5: PC_H = getByte((VAL + 1) % 0x100); break;																		//	READ
	case 6: instruction(getByte(getAdr())); SUB_CYC = 0; break;																//	READ
	default:
		break;
	}
}

void IND_X_RMW(u8(*instruction)(u8)) {
	switch (SUB_CYC)
	{
	case 2: VAL = getByte(PC++); break;																						//	READ
	case 3: VAL = VAL + registers.X; getByte(PC); break;																	//	READ
	case 4: PC_L = getByte(VAL % 0x100); break;																				//	READ
	case 5: PC_H = getByte((VAL + 1) % 0x100); break;																		//	READ
	case 6: VAL = getByte(getAdr()); break;								 													//	READ
	case 7: writeByte(getAdr(), VAL); VAL = instruction(VAL); break;														//	WRITE
	case 8: writeByte(getAdr(), VAL); SUB_CYC = 0; break;																	//	WRITE
	default:
		break;
	}
}

void IND_X_WRITE(u8(*instruction)()) {
	switch (SUB_CYC)
	{
	case 2: VAL = getByte(PC++); break;																						//	READ
	case 3: VAL = VAL + registers.X; getByte(PC); break;																	//	READ
	case 4: PC_L = getByte(VAL % 0x100); break;																				//	READ
	case 5: PC_H = getByte((VAL + 1) % 0x100); break;																		//	READ
	case 6: writeByte(getAdr(), instruction());	SUB_CYC = 0; break;															//	WRITE
	default:
		break;
	}
}


//	INDEXED Y ADDRESSING

void IND_Y_READ(void (*instruction)(u8)) {
	switch (SUB_CYC)
	{
	case 2: VAL = getByte(PC++); break;																						//	READ
	case 3: PC_L = getByte(VAL % 0x100); break;																				//	READ
	case 4: PC_H = getByte((VAL + 1) % 0x100); PC_L += registers.Y; break;													//	READ
	case 5: instruction(getByte(getAdr())); PC_H = ((PC_H << 8) + PC_L) >> 8; SUB_CYC *= PC_L > 0xff; break;				//	READ
	case 6: instruction(getByte(getAdr())); SUB_CYC = 0; break;																//	READ | PAGEBREACH CYCLE
	default:
		break;
	}
}

void IND_Y_RMW(u8(*instruction)(u8)) {
	switch (SUB_CYC)
	{
	case 2: VAL = getByte(PC++); break;																						//	READ
	case 3: PC_L = getByte(VAL % 0x100); break;																				//	READ
	case 4: PC_H = getByte((VAL + 1) % 0x100); PC_L += registers.Y; break;													//	READ
	case 5: getByte(getAdr()); PC_H = ((PC_H << 8) + PC_L) >> 8; break;														//	READ
	case 6: VAL = getByte(getAdr()); break;																					//	READ
	case 7: writeByte(getAdr(), VAL); VAL = instruction(VAL); break;														//	WRITE
	case 8: writeByte(getAdr(), VAL); SUB_CYC = 0; break;																	//	WRITE
	default:
		break;
	}
}

void IND_Y_WRITE(u8(*instruction)()) {
	switch (SUB_CYC)
	{
	case 2: VAL = getByte(PC++); break;																						//	READ
	case 3: PC_L = getByte(VAL % 0x100); break;																				//	READ
	case 4: PC_H = getByte((VAL + 1) % 0x100); PC_L += registers.Y; break;													//	READ
	case 5: getByte(getAdr()); PC_H = ((PC_H << 8) + PC_L) >> 8; break;														//	READ
	case 6: writeByte(getAdr(), instruction()); SUB_CYC = 0; break;															//	WRITE
	default:
		break;
	}
}


//	IMMEDIATE ADDRESSING

void IMM_READ(void (*instruction)(u8)) {
	switch (SUB_CYC)
	{
	case 2: instruction(getByte(PC++)); SUB_CYC = 0; break;																	//	READ
	default:
		break;
	}
}


//	ABSOLUTE INDIRECT ADDRESSING

void ABS_IND_JMP() {
	switch (SUB_CYC)
	{
	case 2: PC_L = getByte(PC++); break;																					//	READ
	case 3: PC_H = getByte(PC++); break;																					//	READ
	case 4: getByte(getAdr()); break;																						//	READ
	case 5: PC = (getByte((PC_H << 8) | ((PC_L + 1) & 0xff)) << 8) | getByte(getAdr()); SUB_CYC = 0; break;					//	READ
	default:
		break;
	}
}


//	IMPLICIT ADDRESSING

void IMP_READ(void (*instruction)()) {
	switch (SUB_CYC)
	{
	case 2: getByte(PC); instruction(); SUB_CYC = 0; break;																	//	READ
	default:
		break;
	}
}
void IMP_READ(void (*instruction)(u8)) {
	switch (SUB_CYC)
	{
	case 2: getByte(PC); instruction(0); SUB_CYC = 0; break;																//	READ
	default:
		break;
	}
}

//	ACCUMULATOR ADDRESSING

void ACC_READ(u8(*instruction)(u8)) {
	switch (SUB_CYC)
	{
	case 2: registers.A = instruction(registers.A); getByte(PC); SUB_CYC = 0; break;										//	READ
	default:
		break;
	}
}


//	STACK ACCESSING INSTRUCTIONS

void STACK_RTI() {
	switch (SUB_CYC)
	{
	case 2:	getByte(PC); break;																								//	READ
	case 3:	SP_++; break;																									//	READ
	case 4:	status.setStatus(getByte(SP_ + 0x100));	SP_++; break;															//	READ
	case 5:	PC_L = getByte(SP_ + 0x100); SP_++;	break;																		//	READ
	case 6:	PC_H = getByte(SP_ + 0x100); PC = getAdr(); SUB_CYC = 0; break;													//	READ
	default: break;
	}
}

void STACK_RTS() {
	switch (SUB_CYC)
	{
	case 2:	getByte(PC); break;																								//	READ
	case 3:	SP_++; break;																									//	READ
	case 4:	PC_L = getByte(SP_ + 0x100); SP_++;	break;																		//	READ
	case 5:	PC_H = getByte(SP_ + 0x100); break;																				//	READ
	case 6: PC = getAdr() + 1; SUB_CYC = 0; break;																			//	READ
	default: break;
	}
}

void STACK_PHA() {
	switch (SUB_CYC)
	{
	case 2:	getByte(PC); break;																								//	READ
	case 3: writeByte(SP_ + 0x100, registers.A); SP_--; SUB_CYC = 0; break;													//	READ
	default: break;
	}
}

void STACK_PHP() {
	switch (SUB_CYC)
	{
	case 2:	getByte(PC); break;																								//	READ
	case 3: writeByte(SP_ + 0x100, status.status | 0x30); SP_--; SUB_CYC = 0; break;										//	READ
	default: break;
	}
}

void STACK_PLA() {
	switch (SUB_CYC)
	{
	case 2:	getByte(PC); break;																								//	READ
	case 3:	SP_++; break;																									//	READ
	case 4:																													//	READ
		registers.A = getByte(SP_ + 0x100);
		status.setNegative(registers.A >> 7);
		status.setZero(registers.A == 0);
		SUB_CYC = 0;
		break;
	default: break;
	}
}

void STACK_PLP() {
	switch (SUB_CYC)
	{
	case 2:	getByte(PC); break;																								//	READ
	case 3:	SP_++; break;																									//	READ
	case 4:																													//	READ
		status.setStatus(getByte(SP_ + 0x100) & 0xef);
		SUB_CYC = 0;
		break;
	default: break;
	}
}

void STACK_NOP() {
	switch (SUB_CYC)
	{
	case 2: SUB_CYC = 0; break;																								//	READ
	default:
		break;
	}
}

void STACK_JSR() {
	switch (SUB_CYC)
	{
	case 2: PC_L = getByte(PC++); break;																					//	READ
	case 3: break;																											//	READ
	case 4:	writeByte(SP_ + 0x100, PC >> 8); SP_--; break;																	//	WRITE
	case 5: writeByte(SP_ + 0x100, PC & 0xff); SP_--; break;												   				//	WRITE
	case 6: PC_H = getByte(PC); PC = getAdr(); SUB_CYC = 0; break;															//	READ
	default:
		break;
	}
}

void STACK_JMP() {
	switch (SUB_CYC)
	{
	case 2: PC_L = getByte(PC++); break;																					//	READ
	case 3:	PC_H = getByte(PC); PC = getAdr(); SUB_CYC = 0; break;															//	READ
	default: break;
	}
}


/*
	CYCLE ACCURATE REWRITE - END
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
		SUB_CYC = 0;
	} break;
	default: break;
	}
}

void setIRQ(bool v) {
	irq = v;
}
bool tmpgo = false;
void IRQorBRK(uint8_t state) {
	if (tmpgo && 0)
		printf("IRQ Cycle %d\n", state);
	switch (state) {
	case 1: break;
	case 2: break;
	case 3: break;
	case 4: break;
	case 5: break;
	case 6: break;
	case 7: {
		if (tmpgo && 0)
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
		SUB_CYC = 0;
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
	if (SUB_CYC == 1 && nmi == true && nmi_running == false) {
		nmi_running = true;
	}
	if (SUB_CYC == 1 && irq == true && irq_running == false && status.interruptDisable == 0) {
		irq_running = true;
	}
	if (nmi == true && nmi_running == true) {
		NMI(SUB_CYC);
		SUB_CYC++;
		return 1;
	}
	if (irq == true && irq_running == true && status.interruptDisable == 0) {
		IRQorBRK(SUB_CYC);
		SUB_CYC++;
		return 1;
	}

	//	Fetch new opcode, if necessary
	if (SUB_CYC == 1) {
		CURRENT_OPCODE = getByte(PC++);
	}

	if (CURRENT_OPCODE == 0xc90000)
		tmpgo = true;
	if (tmpgo && 1)
		printf("%04x A:%02x X:%02x Y:%02x - P:%02x SP:%02x \n", PC, registers.A, registers.X, registers.Y, status.status, SP_);

	switch (CURRENT_OPCODE) {

	case 0x00: { status.setBrk(1); irq = true; printf("BREAK "); return 7; break; }
	case 0x01: IND_X_READ(ORA); break;
	case 0x03: IND_X_RMW(SLO); break;
	case 0x04: ZP_READ(NOP); break;
	case 0x05: ZP_READ(ORA); break;
	case 0x06: ZP_RMW(ASL); break;
	case 0x07: ZP_RMW(SLO); break;
	case 0x08: STACK_PHP(); break;
	case 0x09: IMM_READ(ORA); break;
	case 0x0a: ACC_READ(ASL); break;
	case 0x0c: ABS_READ(NOP); break;
	case 0x0d: ABS_READ(ORA); break;
	case 0x0e: ABS_RMW(ASL); break;
	case 0x0f: ABS_RMW(SLO); break;
	case 0x10: REL_READ(BPL); break;
	case 0x11: IND_Y_READ(ORA); break;
	case 0x13: IND_Y_RMW(SLO); break;
	case 0x14: ZP_IND_X_READ(NOP); break;
	case 0x15: ZP_IND_X_READ(ORA); break;
	case 0x16: ZP_IND_X_RMW(ASL); break;
	case 0x17: ZP_IND_X_RMW(SLO); break;
	case 0x18: IMP_READ(CLC); break;
	case 0x19: ABS_IND_Y_READ(ORA); break;
	case 0x1a: IMP_READ(NOP); break;
	case 0x1b: ABS_IND_Y_RMW(SLO); break;
	case 0x1c: ABS_IND_X_READ(NOP); break;
	case 0x1d: ABS_IND_X_READ(ORA); break;
	case 0x1e: ABS_IND_X_RMW(ASL); break;
	case 0x1f: ABS_IND_X_RMW(SLO); break;
	case 0x20: STACK_JSR(); break;
	case 0x21: IND_X_READ(AND); break;
	case 0x23: IND_X_RMW(RLA); break;
	case 0x24: ZP_READ(BIT); break;
	case 0x25: ZP_READ(AND); break;
	case 0x26: ZP_RMW(ROL); break;
	case 0x27: ZP_RMW(RLA); break;
	case 0x28: STACK_PLP(); break;
	case 0x29: IMM_READ(AND); break;
	case 0x2a: ACC_READ(ROL); break;
	case 0x2c: ABS_READ(BIT); break;
	case 0x2d: ABS_READ(AND); break;
	case 0x2e: ABS_RMW(ROL); break;
	case 0x2f: ABS_RMW(RLA); break;
	case 0x30: REL_READ(BMI); break;
	case 0x31: IND_Y_READ(AND); break;
	case 0x33: IND_Y_RMW(RLA); break;
	case 0x34: ZP_IND_X_READ(NOP); break;
	case 0x35: ZP_IND_X_READ(AND); break;
	case 0x36: ZP_IND_X_RMW(ROL); break;
	case 0x37: ZP_IND_X_RMW(RLA); break;
	case 0x38: IMP_READ(SEC); break;
	case 0x39: ABS_IND_Y_READ(AND); break;
	case 0x3a: IMP_READ(NOP); break;
	case 0x3b: ABS_IND_Y_RMW(RLA); break;
	case 0x3c: ABS_IND_X_READ(NOP); break;
	case 0x3d: ABS_IND_X_READ(AND); break;
	case 0x3e: ABS_IND_X_RMW(ROL); break;
	case 0x3f: ABS_IND_X_RMW(RLA); break;
	case 0x40: STACK_RTI(); break;
	case 0x41: IND_X_READ(EOR); break;
	case 0x43: IND_X_RMW(SRE); break;
	case 0x44: ZP_READ(NOP); break;
	case 0x45: ZP_READ(EOR); break;
	case 0x46: ZP_RMW(LSR); break;
	case 0x47: ZP_RMW(SRE); break;
	case 0x48: STACK_PHA(); break;
	case 0x49: IMM_READ(EOR); break;
	case 0x4a: ACC_READ(LSR); break;
	case 0x4c: STACK_JMP(); break;
	case 0x4d: ABS_READ(EOR); break;
	case 0x4e: ABS_RMW(LSR); break;
	case 0x4f: ABS_RMW(SRE); break;
	case 0x50: REL_READ(BVC); break;
	case 0x51: IND_Y_READ(EOR); break;
	case 0x53: IND_Y_RMW(SRE); break;
	case 0x54: ZP_IND_X_READ(NOP); break;
	case 0x55: ZP_IND_X_READ(EOR); break;
	case 0x56: ZP_IND_X_RMW(LSR); break;
	case 0x57: ZP_IND_X_RMW(SRE); break;
	case 0x58: IMP_READ(CLI); break;
	case 0x59: ABS_IND_Y_READ(EOR); break;
	case 0x5a: IMP_READ(NOP); break;
	case 0x5b: ABS_IND_X_RMW(SRE); break;
	case 0x5c: ABS_IND_X_READ(NOP); break;
	case 0x5d: ABS_IND_X_READ(EOR); break;
	case 0x5e: ABS_IND_X_RMW(LSR); break;
	case 0x5f: ABS_IND_X_RMW(SRE); break;
	case 0x60: STACK_RTS(); break;
	case 0x61: IND_X_READ(ADC); break;
	case 0x63: IND_X_RMW(RRA); break;
	case 0x64: ZP_READ(NOP); break;
	case 0x65: ZP_READ(ADC); break;
	case 0x66: ZP_RMW(ROR); break;
	case 0x67: ZP_RMW(RRA); break;
	case 0x68: STACK_PLA(); break;
	case 0x69: IMM_READ(ADC); break;
	case 0x6a: ACC_READ(ROR); break;
	case 0x6c: ABS_IND_JMP(); break;
	case 0x6d: ABS_READ(ADC); break;
	case 0x6e: ABS_RMW(ROR); break;
	case 0x6f: ABS_RMW(RRA); break;
	case 0x70: REL_READ(BVS); break;
	case 0x71: IND_Y_READ(ADC); break;
	case 0x73: IND_Y_RMW(RRA); break;
	case 0x74: ZP_IND_X_READ(NOP); break;
	case 0x75: ZP_IND_X_READ(ADC); break;
	case 0x76: ZP_IND_X_RMW(ROR); break;
	case 0x77: ZP_IND_X_RMW(RRA); break;
	case 0x78: IMP_READ(SEI); break;
	case 0x79: ABS_IND_Y_READ(ADC); break;
	case 0x7a: IMP_READ(NOP); break;
	case 0x7b: ABS_IND_Y_RMW(RRA); break;
	case 0x7c: ABS_IND_X_READ(NOP); break;
	case 0x7d: ABS_IND_X_READ(ADC); break;
	case 0x7e: ABS_IND_X_RMW(ROR); break;
	case 0x7f: ABS_IND_X_RMW(RRA); break;
	case 0x80: IMM_READ(NOP); break;
	case 0x81: IND_X_WRITE(STA); break;
	case 0x82: IMM_READ(NOP); break;
	case 0x83: IND_X_WRITE(SAX); break;
	case 0x84: ZP_WRITE(STY); break;
	case 0x85: ZP_WRITE(STA); break;
	case 0x86: ZP_WRITE(STX); break;
	case 0x87: ZP_WRITE(SAX); break;
	case 0x88: IMP_READ(DEY); break;
	case 0x89: IMM_READ(NOP); break;
	case 0x8a: IMP_READ(TXA); break;
	case 0x8c: ABS_WRITE(STY); break;
	case 0x8d: ABS_WRITE(STA); break;
	case 0x8e: ABS_WRITE(STX); break;
	case 0x8f: ABS_WRITE(SAX); break;
	case 0x90: REL_READ(BCC); break;
	case 0x91: IND_Y_WRITE(STA); break;
	case 0x94: ZP_IND_X_WRITE(STY); break;
	case 0x95: ZP_IND_X_WRITE(STA); break;
	case 0x96: ZP_IND_Y_WRITE(STX); break;
	case 0x97: ZP_IND_Y_WRITE(SAX); break;
	case 0x98: IMP_READ(TYA); break;
	case 0x99: ABS_IND_Y_WRITE(STA); break;
	case 0x9a: IMP_READ(TXS); break;
	case 0x9d: ABS_IND_X_WRITE(STA); break;
	case 0xa0: IMM_READ(LDY); break;
	case 0xa1: IND_X_READ(LDA); break;
	case 0xa2: IMM_READ(LDX); break;
	case 0xa3: IND_X_READ(LAX); break;
	case 0xa4: ZP_READ(LDY); break;
	case 0xa5: ZP_READ(LDA); break;
	case 0xa6: ZP_READ(LDX); break;
	case 0xa7: ZP_READ(LAX); break;
	case 0xa8: IMP_READ(TAY); break;
	case 0xa9: IMM_READ(LDA); break;
	case 0xaa: IMP_READ(TAX); break;
	case 0xab: IMM_READ(LAX); break;
	case 0xac: ABS_READ(LDY); break;
	case 0xad: ABS_READ(LDA); break;
	case 0xae: ABS_READ(LDX); break;
	case 0xaf: ABS_READ(LAX); break;
	case 0xb0: REL_READ(BCS); break;
	case 0xb1: IND_Y_READ(LDA); break;
	case 0xb3: IND_Y_READ(LAX); break;
	case 0xb4: ZP_IND_X_READ(LDY); break;
	case 0xb5: ZP_IND_X_READ(LDA); break;
	case 0xb6: ZP_IND_Y_READ(LDX); break;
	case 0xb7: ZP_IND_Y_READ(LAX); break;
	case 0xb8: IMP_READ(CLV); break;
	case 0xb9: ABS_IND_Y_READ(LDA); break;
	case 0xba: IMP_READ(TSX); break;
	case 0xbc: ABS_IND_X_READ(LDY); break;
	case 0xbd: ABS_IND_X_READ(LDA); break;
	case 0xbe: ABS_IND_Y_READ(LDX); break;
	case 0xbf: ABS_IND_Y_READ(LAX); break;
	case 0xc0: IMM_READ(CPY); break;
	case 0xc1: IND_X_READ(CMP); break;
	case 0xc2: IMM_READ(NOP); break;
	case 0xc3: IND_X_RMW(DCP); break;
	case 0xc4: ZP_READ(CPY); break;
	case 0xc5: ZP_READ(CMP); break;
	case 0xc6: ZP_RMW(DEC); break;
	case 0xc7: ZP_RMW(DCP); break;
	case 0xc8: IMP_READ(INY); break;
	case 0xc9: IMM_READ(CMP); break;
	case 0xca: IMP_READ(DEX); break;
	case 0xcc: ABS_READ(CPY); break;
	case 0xcd: ABS_READ(CMP); break;
	case 0xce: ABS_RMW(DEC); break;
	case 0xcf: ABS_RMW(DCP); break;
	case 0xd0: REL_READ(BNE); break;
	case 0xd1: IND_Y_READ(CMP); break;
	case 0xd3: IND_Y_RMW(DCP); break;
	case 0xd4: ZP_IND_X_READ(NOP); break;
	case 0xd5: ZP_IND_X_READ(CMP); break;
	case 0xd6: ZP_IND_X_RMW(DEC); break;
	case 0xd7: ZP_IND_X_RMW(DCP); break;
	case 0xd8: IMP_READ(CLD); break;
	case 0xd9: ABS_IND_Y_READ(CMP); break;
	case 0xda: IMP_READ(NOP); break;
	case 0xdb: ABS_IND_Y_RMW(DCP); break;
	case 0xdc: ABS_IND_X_READ(NOP); break;
	case 0xdd: ABS_IND_X_READ(CMP); break;
	case 0xde: ABS_IND_X_RMW(DEC); break;
	case 0xdf: ABS_IND_X_RMW(DCP); break;
	case 0xe0: IMM_READ(CPX); break;
	case 0xe1: IND_X_READ(SBC); break;
	case 0xe2: IMM_READ(NOP); break;
	case 0xe3: IND_X_RMW(ISC); break;
	case 0xe4: ZP_READ(CPX); break;
	case 0xe5: ZP_READ(SBC); break;
	case 0xe6: ZP_RMW(INC); break;
	case 0xe7: ZP_RMW(ISC); break;
	case 0xe8: IMP_READ(INX); break;
	case 0xe9: IMM_READ(SBC); break;
	case 0xea: STACK_NOP(); break;
	case 0xeb: IMM_READ(SBC); break;
	case 0xec: ABS_READ(CPX); break;
	case 0xed: ABS_READ(SBC); break;
	case 0xee: ABS_RMW(INC); break;
	case 0xef: ABS_RMW(ISC); break;
	case 0xf0: REL_READ(BEQ); break;
	case 0xf1: IND_Y_READ(SBC); break;
	case 0xf3: IND_Y_RMW(ISC); break;
	case 0xf4: ZP_IND_X_READ(NOP); break;
	case 0xf5: ZP_IND_X_READ(SBC); break;
	case 0xf6: ZP_IND_X_RMW(INC); break;
	case 0xf7: ZP_IND_X_RMW(ISC); break;
	case 0xf8: IMP_READ(SED); break;
	case 0xf9: ABS_IND_Y_READ(SBC); break;
	case 0xfa: IMP_READ(NOP); break;
	case 0xfb: ABS_IND_Y_RMW(ISC); break;
	case 0xfc: ABS_IND_X_READ(NOP); break;
	case 0xfd: ABS_IND_X_READ(SBC); break;
	case 0xfe: ABS_IND_X_RMW(INC); break;
	case 0xff: ABS_IND_X_RMW(ISC); break;
	default:
		printf("ERROR! Unimplemented opcode 0x%02x at address 0x%04x !\n", CURRENT_OPCODE, PC);
		std::exit(1);
		break;
	}
	SUB_CYC++;
	return 1;
}