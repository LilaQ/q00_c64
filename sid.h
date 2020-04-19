#include <vector>
#include <assert.h>
#pragma once

typedef uint8_t u8;
typedef uint16_t u16;
typedef int8_t i8;
typedef int16_t i16;

void SID_init();
void SID_step();
void SID_setChannelFreqLo(u8 channel, u8 val);
void SID_setChannelFreqHi(u8 channel, u8 val);
void SID_setChannelPulseWidthLo(u8 channel, u8 val);
void SID_setChannelPulseWidthHi(u8 channel, u8 val);
void SID_setChannelAttackDecay(u8 channel, u8 val);
void SID_setChannelSustainRelease(u8 channel, u8 val);
void SID_setChannelNPST(u8 channel, u8 val);
void SID_setFilterCutoffLo(u8 val);
void SID_setFilterCutoffHi(u8 val);
void SID_setPotentiometerX(u8 val);
void SID_setPotentiometerY(u8 val);

//	6502 PAL runs at 0,985249 MHz
//	We sample at 44100 Hz
//	985249 / 44100 = 22,34
//	Alle 22,34 Cycles brauchen nehmen wir ein Sample


class Channel {

	private:
		void tickTriangle() {
			printf("Tri\n");
		}
		void tickSawtooth() {
			printf("Saw\n");
		}
		void tickRectangle() {
			printf("Rec\n");
		}
		void tickNoise() {
			printf("Noise\n");
		}

	public: 
		//	config
		bool enabled = false;
		u16 freq = 0x0000;						//	Frequency
		u16 pulse_width = 0x0000;				//	Pulse width for Rectangle
		u8 attack = 0x00;
		u8 decay = 0x00;
		u8 sustain = 0x00;
		u8 release = 0x00;
		std::vector<float> buffer;
		void (Channel::*process)();

		void tick() {
			if (enabled) {
				(this->*process)();
			}
			else {
				buffer.push_back(0);
			}
		}
		
		void setNPST(u8 val) {
			enabled = val & 1;
			switch ((val >> 4) & 0b1111)
			{
			case 1: 
				process = &Channel::tickTriangle;
				break;
			case 2:
				process = &Channel::tickSawtooth;
				break;
			case 4:
				process = &Channel::tickRectangle;
				break;
			case 8:
				process = &Channel::tickNoise;
				break;
			default:
				assert(false);
				break;
			}
		}

		void setAttackDecay(u8 val) {
			attack = (val >> 4) & 0b1111;
			decay = val & 0b1111;
		}

		void setSustainRelease(u8 val) {
			sustain = (val >> 4) & 0b1111;
			release = val & 0b1111;
		}

		void setFreqLo(u8 val) {
			freq = (freq & 0b1111111100000000) | val;
		}

		void setFreqHi(u8 val) {
			freq = (freq & 0b0000000011111111) | (val << 8);
		}

		void setPulseWidthLo(u8 val) {
			pulse_width = (pulse_width & 0b1111111100000000) | val;
		}

		void setPulseWidthHi(u8 val) {
			pulse_width = (pulse_width & 0b0000000011111111) | ((val & 0b1111) << 8);
		}
};