#include <stdint.h>
#include "SDL2/include/SDL.h"
#include "cpu.h"

uint8_t data_port_A;
uint8_t data_port_B;
bool port_A_RW = false;
bool port_B_RW = false;
uint8_t* KEYS;

const SDL_Scancode KEYMAP[8][8] = {
	{SDL_SCANCODE_BACKSPACE, SDL_SCANCODE_RETURN, SDL_SCANCODE_RIGHT, SDL_SCANCODE_F7, SDL_SCANCODE_F1, SDL_SCANCODE_F3, SDL_SCANCODE_F5, SDL_SCANCODE_DOWN},					//	PA0
	{SDL_SCANCODE_3, SDL_SCANCODE_W, SDL_SCANCODE_A, SDL_SCANCODE_4, SDL_SCANCODE_Z, SDL_SCANCODE_S, SDL_SCANCODE_E, SDL_SCANCODE_LSHIFT},										//	PA1
	{SDL_SCANCODE_5, SDL_SCANCODE_R, SDL_SCANCODE_D, SDL_SCANCODE_6, SDL_SCANCODE_C, SDL_SCANCODE_F, SDL_SCANCODE_T, SDL_SCANCODE_X},											//	PA2
	{SDL_SCANCODE_7, SDL_SCANCODE_Y, SDL_SCANCODE_G, SDL_SCANCODE_8, SDL_SCANCODE_B, SDL_SCANCODE_H, SDL_SCANCODE_U, SDL_SCANCODE_V},											//	PA3
	{SDL_SCANCODE_9, SDL_SCANCODE_I, SDL_SCANCODE_J, SDL_SCANCODE_0, SDL_SCANCODE_M, SDL_SCANCODE_K, SDL_SCANCODE_O, SDL_SCANCODE_N},											//	PA4
	{SDL_SCANCODE_KP_PLUS, SDL_SCANCODE_P, SDL_SCANCODE_L, SDL_SCANCODE_MINUS, SDL_SCANCODE_PERIOD, SDL_SCANCODE_KP_DIVIDE, SDL_SCANCODE_KP_AT, SDL_SCANCODE_COMMA},			//	PA5
	{SDL_SCANCODE_KP_HASH, SDL_SCANCODE_KP_MULTIPLY, SDL_SCANCODE_SEMICOLON, SDL_SCANCODE_HOME, SDL_SCANCODE_RSHIFT, SDL_SCANCODE_EQUALS, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_SLASH},	//	PA6
	{SDL_SCANCODE_1, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_COMPUTER, SDL_SCANCODE_2, SDL_SCANCODE_SPACE, SDL_SCANCODE_TAB, SDL_SCANCODE_Q, SDL_SCANCODE_END}							//	PA7
};

void setKeyboardInput(uint8_t* _KEYS) {
	KEYS = _KEYS;
}

void writeDataPortA(uint8_t val) {
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

void writeDataPortB(uint8_t val) {
}

void setPortARW(uint8_t val) {
	port_A_RW = (val > 0);
}

void setPortBRW(uint8_t val) {
	port_B_RW = (val > 0);
}

uint8_t readDataPortA() {
	return data_port_A;
}

uint8_t readDataPortB() {
	return data_port_B;
}

struct CIA1_IRQ_STATUS {
	bool IRQ_on_timerA_underflow = false;
	bool IRQ_on_timerB_underflow = false;
	bool IRQ_on_clock_eq_alarm = false;
	bool IRQ_on_complete_byte = false;
	bool IRQ_on_flag_pin = false;
	uint8_t b0 = 0;			//	underflow timer A
	uint8_t b1 = 0 << 1;	//	underflow timer B
	uint8_t b2 = 0 << 2;	//	clock equals alarm
	uint8_t b3 = 0 << 3;	//	complete byte transferred
	uint8_t b4 = 0 << 4;	//	FLAG pin neg edge occured (casette tape, serial port)
	uint8_t b7 = 0 << 7;	//	IRQ occured; atleast one bit in MASK and DATA is the same

	void set(uint8_t val) {
		//	check if bits need clearing, or setting#
		bool set_or_clear = (val & 0x80) == 0x80;
		if((val & 0x01) == 0x01)
			IRQ_on_timerA_underflow = set_or_clear;
		if((val & 0x02) == 0x02)
			IRQ_on_timerB_underflow = set_or_clear;
		if((val & 0x04) == 0x04)
			IRQ_on_clock_eq_alarm = set_or_clear;
		if((val & 0x08) == 0x08)
			IRQ_on_complete_byte = set_or_clear;
		if((val & 0x10) == 0x10)
			IRQ_on_flag_pin = set_or_clear;
	}

	uint8_t get() {
		b1 = b1 << 1;
		b2 = b2 << 2;
		b3 = b3 << 3;
		b4 = b4 << 4;
		b7 = b7 << 7;
		return b7 | b4 | b3 | b2 | b1 | b0;
	}

	void flagUnderflowTimerB() {

	}
};

struct TIMER {
	bool timer_running = false;
	bool timer_underflow_port_b_bit_6_invert = false;
	bool timer_stop_timer_after_underflow = false;
	bool timer_counts_cnt_slopes = false;	//	false = count system cycles instead
	bool timer_rtc_50hz = false;			//	false = 60hz;
	uint16_t timer_latch = 0;
	int16_t timer_value = 0;

	void set(uint8_t val, CIA1_IRQ_STATUS &status) {
		timer_running = ((val & 0x01) == 0x01);
		if ((val & 0x02) == 0x02)
			status.flagUnderflowTimerB();
		timer_underflow_port_b_bit_6_invert = ((val & 0x04) == 0x04);
		timer_stop_timer_after_underflow = ((val & 0x08) == 0x08);
		if ((val & 0x10) == 0x10)
			timer_value = timer_latch;
		timer_counts_cnt_slopes = ((val & 0x20) == 0x20);
		timer_rtc_50hz = ((val & 0x80) == 0x80);
	}

	bool tick(uint8_t cycles) {
		while (cycles--) {
			if (timer_running) {
				if (!timer_counts_cnt_slopes) {
					//	0,9852486 MHz PAL
					timer_value--;
					if (timer_value <= 0) {
						if (timer_stop_timer_after_underflow)
							timer_running = false;
						else
							timer_value = timer_latch;
						return true;
					}
				}
			}
		}
		return false;
	}
};

CIA1_IRQ_STATUS cia1_irq_status;
TIMER timerA;
TIMER timerB;

void setCIA1timerAlatchHi(uint8_t val) {
	timerA.timer_latch = (timerA.timer_latch & 0x00ff) | (val << 8);
	printf("Timer A Latch High: 0x%02x\n", val);
}

void setCIA1timerAlatchLo(uint8_t val) {
	timerA.timer_latch = (timerA.timer_latch & 0xff00) | val;
	printf("Timer A Latch Low: 0x%02x\n", val);
}

void setCIA1timerBlatchHi(uint8_t val) {
	timerB.timer_latch = (timerB.timer_latch & 0x00ff) | (val << 8);
}

void setCIA1timerBlatchLo(uint8_t val) {
	timerB.timer_latch = (timerB.timer_latch & 0xff00) | val;
}

uint8_t readCIA1timerALo() {
	return timerA.timer_value & 0xff;
}

uint8_t readCIA1timerAHi() {
	return timerA.timer_value >> 8;
}

uint8_t readCIA1timerBLo() {
	return timerB.timer_value & 0xff;
}

uint8_t readCIA1timerBHi() {
	return timerB.timer_value >> 8;
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
	timerA.set(val, cia1_irq_status);
	printf("Setting CIA1 Timer A Control: %02x\n", val);
}

void setCIA1TimerBControl(uint8_t val) {
	printf("Setting CIA1 Timer B Control: %02x\n", val);
}

void tickAllTimers(uint8_t cycles) {
	if (timerA.tick(cycles)) {
		cia1_irq_status.b1 = 1;
		if (cia1_irq_status.IRQ_on_timerA_underflow) {
			setIRQ(true);
		}
	}
	if (timerA.tick(cycles)) {
		cia1_irq_status.b2 = 1;
		if (cia1_irq_status.IRQ_on_timerB_underflow) {
			setNMI(true);
		}
	}
}