#pragma once
#include <stdint.h>
#include "cia.h"
#include "cpu.h"

CIA1_IRQ_STATUS cia1_irq_status;
TIMER cia1_timerA;
TIMER cia1_timerB;
uint8_t data_port_A;
uint8_t data_port_B;
bool port_A_RW = false;
bool port_B_RW = false;
uint8_t* KEYS;

void setKeyboardInput(uint8_t* _KEYS) {
	KEYS = _KEYS;
}

void writeCIA1DataPortA(uint8_t val) {
	data_port_B = 0xff;
	for (int j = 0; j < 8; j++) {
		if (!(val & 1)) {
			for (int i = 0; i < 8; i++) {
				if (KEYS[SDL_SCANCODE_LEFT]) {
					if(j == 0)
						data_port_B &= ~(1 << 2);
					if(j == 1)
						data_port_B &= ~(1 << 7);
				}
				if (KEYS[SDL_SCANCODE_UP]) {
					if (j == 0)
						data_port_B &= ~(1 << 7);
					if (j == 1)
						data_port_B &= ~(1 << 7);
				}
				if (KEYS[KEYMAP[j][i]]) {
					data_port_B &= ~(1 << i);
				}
			}
		}
		val >>= 1;
	}
}

void writeCIA1DataPortB(uint8_t val) {
}

void setCIA1PortARW(uint8_t val) {
	port_A_RW = (val > 0);
}

void setCIA1PortBRW(uint8_t val) {
	port_B_RW = (val > 0);
}

uint8_t readCIA1DataPortA() {
	return data_port_A;
}

uint8_t readCIA1DataPortB() {
	return data_port_B;
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

void tickAllTimers(uint8_t cycles) {
	if (cia1_timerA.tick(cycles)) {
		cia1_irq_status.b1 = 1;
		if (cia1_irq_status.IRQ_on_timerA_underflow) {
			setIRQ(true);
		}
	}
	if (cia1_timerB.tick(cycles)) {
		cia1_irq_status.b2 = 1;
		if (cia1_irq_status.IRQ_on_timerB_underflow) {
			setNMI(true);
		}
	}
}