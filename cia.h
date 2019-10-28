#pragma once
#include <stdint.h>
#include "SDL2/include/SDL.h"

uint8_t readCIA1DataPortA();
uint8_t readCIA1DataPortB();
void writeCIA1DataPortA(uint8_t val);
void writeCIA1DataPortB(uint8_t val);
void setCIA1PortARW(uint8_t val);
void setCIA1PortBRW(uint8_t val);
uint8_t readCIA1timerALo();
uint8_t readCIA1timerAHi();
uint8_t readCIA1timerBLo();
uint8_t readCIA1timerBHi();
void setCIA1timerAlatchHi(uint8_t val);
void setCIA1timerAlatchLo(uint8_t val);
void setCIA1timerBlatchHi(uint8_t val);
void setCIA1timerBlatchLo(uint8_t val);
uint8_t readCIA1IRQStatus();
void setCIA1TimerAControl(uint8_t val);
void setCIA1TimerBControl(uint8_t val);
void setCIA1IRQcontrol(uint8_t val);

void setKeyboardInput(uint8_t* KEYS);
void tickAllTimers(uint8_t cycles);

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
		if ((val & 0x01) == 0x01)
			IRQ_on_timerA_underflow = set_or_clear;
		if ((val & 0x02) == 0x02)
			IRQ_on_timerB_underflow = set_or_clear;
		if ((val & 0x04) == 0x04)
			IRQ_on_clock_eq_alarm = set_or_clear;
		if ((val & 0x08) == 0x08)
			IRQ_on_complete_byte = set_or_clear;
		if ((val & 0x10) == 0x10)
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

	void set(uint8_t val, CIA1_IRQ_STATUS& status) {
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
						if (timer_stop_timer_after_underflow) {
							timer_running = false;
						}
						else {
							timer_value = timer_latch;
						}
						return true;
					}
				}
			}
		}
		return false;
	}
};
