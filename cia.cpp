#pragma once
#include <stdint.h>
#include "cia.h"
#include "cpu.h"

CIA1_IRQ_STATUS cia1_irq_status;
CIA2_NMI_STATUS cia2_nmi_status;
TIMER cia1_timerA;
TIMER cia1_timerB;
TIMER cia2_timerA;
TIMER cia2_timerB;
uint8_t cia1_data_port_A;
uint8_t cia1_data_port_B;
bool cia1_port_A_RW = false;
bool cia1_port_B_RW = false;
uint8_t* KEYS;

void setKeyboardInput(uint8_t* _KEYS) {
	KEYS = _KEYS;
}

//	CIA 1

void writeCIA1DataPortA(uint8_t val) {
	cia1_data_port_B = 0xff;
	for (int j = 0; j < 8; j++) {
		if (!(val & 1)) {
			for (int i = 0; i < 8; i++) {
				if (KEYS[SDL_SCANCODE_LEFT]) {
					if(j == 0)
						cia1_data_port_B &= ~(1 << 2);
					if(j == 1)
						cia1_data_port_B &= ~(1 << 7);
				}
				if (KEYS[SDL_SCANCODE_UP]) {
					if (j == 0)
						cia1_data_port_B &= ~(1 << 7);
					if (j == 1)
						cia1_data_port_B &= ~(1 << 7);
				}
				if (KEYS[KEYMAP[j][i]]) {
					cia1_data_port_B &= ~(1 << i);
				}
			}
		}
		val >>= 1;
	}
}

void writeCIA1DataPortB(uint8_t val) {
}

void setCIA1PortARW(uint8_t val) {
	cia1_port_A_RW = (val > 0);
}

void setCIA1PortBRW(uint8_t val) {
	cia1_port_B_RW = (val > 0);
}

uint8_t readCIA1DataPortA() {
	return cia1_data_port_A;
}

uint8_t readCIA1DataPortB() {
	return cia1_data_port_B | cia1_data_port_A;
}

void setCIA1timerAlatchHi(uint8_t val) {
	cia1_timerA.timer_latch = (cia1_timerA.timer_latch & 0x00ff) | (val << 8);
	printf("Timer A Latch High: 0x%02x\n", val);
}

void setCIA1timerAlatchLo(uint8_t val) {
	cia1_timerA.timer_latch = (cia1_timerA.timer_latch & 0xff00) | val;
	printf("Timer A Latch Low: 0x%02x\n", val);
}

void setCIA1timerBlatchHi(uint8_t val) {
	cia1_timerB.timer_latch = (cia1_timerB.timer_latch & 0x00ff) | (val << 8);
}

void setCIA1timerBlatchLo(uint8_t val) {
	cia1_timerB.timer_latch = (cia1_timerB.timer_latch & 0xff00) | val;
}

uint8_t readCIA1timerALo() {
	return cia1_timerA.timer_value & 0xff;
}

uint8_t readCIA1timerAHi() {
	return cia1_timerA.timer_value >> 8;
}

uint8_t readCIA1timerBLo() {
	return cia1_timerB.timer_value & 0xff;
}

uint8_t readCIA1timerBHi() {
	return cia1_timerB.timer_value >> 8;
}

uint8_t readCIA1IRQStatus() {
	return cia1_irq_status.get();
}

void setCIA1IRQcontrol(uint8_t val) {
	/*
		Write: (Bit 0..4 = INT MASK, Interrupt mask)
		Bit 0: 1 = Interrupt release through timer A underflow
		Bit 1: 1 = Interrupt release through timer B underflow
		Bit 2: 1 = Interrupt release if clock=alarmtime
		Bit 3: 1 = Interrupt release if a complete byte has been received/sent.
		Bit 4: 1 = Interrupt release if a positive slope occurs at the FLAG-Pin.
		Bit 5..6: unused
		Bit 7: Source bit. 0 = set bits 0..4 are clearing the according mask bit. 1 = set bits 0..4 are setting the according mask bit. If all bits 0..4 are cleared, there will be no change to the mask.
	*/
	cia1_irq_status.set(val);
	printf("Setting CIA1 IRQ Control: %02x\n", val);
}

void setCIA1TimerAControl(uint8_t val) {
	/*
		Bit 0: 0 = Stop timer; 1 = Start timer
		Bit 1: 1 = Indicates a timer underflow at port B in bit 6.
		Bit 2: 0 = Through a timer overflow, bit 6 of port B will get high for one cycle , 1 = Through a timer underflow, bit 6 of port B will be inverted
		Bit 3: 0 = Timer-restart after underflow (latch will be reloaded), 1 = Timer stops after underflow.
		Bit 4: 1 = Load latch into the timer once.
		Bit 5: 0 = Timer counts system cycles, 1 = Timer counts positive slope at CNT-pin
		Bit 6: Direction of the serial shift register, 0 = SP-pin is input (read), 1 = SP-pin is output (write)
		Bit 7: Real Time Clock, 0 = 60 Hz, 1 = 50 Hz
	*/
	cia1_timerA.set(val, cia1_irq_status);
	printf("Setting CIA1 Timer A Control: %02x\n", val);
}

void setCIA1TimerBControl(uint8_t val) {
	printf("Setting CIA1 Timer B Control: %02x\n", val);
}

//	CIA 2
void setCIA2timerAlatchHi(uint8_t val) {
	cia2_timerA.timer_latch = (cia2_timerA.timer_latch & 0x00ff) | (val << 8);
	printf("Timer A Latch High: 0x%02x\n", val);
}

void setCIA2timerAlatchLo(uint8_t val) {
	cia2_timerA.timer_latch = (cia2_timerA.timer_latch & 0xff00) | val;
	printf("Timer A Latch Low: 0x%02x\n", val);
}

void setCIA2timerBlatchHi(uint8_t val) {
	cia2_timerB.timer_latch = (cia2_timerB.timer_latch & 0x00ff) | (val << 8);
}

void setCIA2timerBlatchLo(uint8_t val) {
	cia2_timerB.timer_latch = (cia2_timerB.timer_latch & 0xff00) | val;
}

uint8_t readCIA2timerALo() {
	return cia2_timerA.timer_value & 0xff;
}

uint8_t readCIA2timerAHi() {
	return cia2_timerA.timer_value >> 8;
}

uint8_t readCIA2timerBLo() {
	return cia2_timerB.timer_value & 0xff;
}

uint8_t readCIA2timerBHi() {
	return cia2_timerB.timer_value >> 8;
}

uint8_t readCIA2NMIStatus() {
	return cia2_nmi_status.get();
}

void setCIA2NMIcontrol(uint8_t val) {
	cia2_nmi_status.set(val);
	printf("Setting CIA2 IRQ Control: %02x\n", val);
}

void setCIA2TimerAControl(uint8_t val) {
	cia2_timerA.set(val, cia2_nmi_status);
	printf("Setting CIA2 Timer A Control: %02x\n", val);
}

void setCIA2TimerBControl(uint8_t val) {
	printf("Setting CIA2 Timer B Control: %02x\n", val);
}

//	MAIN

void tickAllTimers(uint8_t cycles) {
	//	CIA 1 Timers
	if (cia1_timerA.tick(cycles)) {
		if (cia1_irq_status.IRQ_on_timerA_underflow) {
			cia1_irq_status.IRQ_occured_general_flag = true;
			cia1_irq_status.IRQ_occured_by_underflow_timerA = true;
			setIRQ(true);
		}
	}
	if (cia1_timerB.tick(cycles)) {
		if (cia1_irq_status.IRQ_on_timerB_underflow) {
			cia1_irq_status.IRQ_occured_general_flag = true;
			cia1_irq_status.IRQ_occured_by_underflow_timerB = true;
			setIRQ(true);
		}
	}
	//	CIA 2 Timers
	if (cia2_timerA.tick(cycles)) {
		if (cia2_nmi_status.NMI_on_timerA_underflow) {
			cia2_nmi_status.NMI_occured_general_flag = true;
			cia2_nmi_status.NMI_occured_by_underflow_timerA = true;
			//setGO();
			setNMI(true);
		}
	}
	if (cia2_timerB.tick(cycles)) {
		if (cia2_nmi_status.NMI_on_timerB_underflow) {
			cia2_nmi_status.NMI_occured_general_flag = true;
			cia2_nmi_status.NMI_occured_by_underflow_timerB = true;
			setNMI(true);
		}
	}
}