#include <vector>
#include <cmath>
#include <math.h>
#include <assert.h>
#pragma once

typedef uint8_t u8;
typedef uint16_t u16;
typedef int8_t i8;
typedef int16_t i16;
typedef uint32_t u32;

void SID_init();
void SID_step(u32 steps);
void SID_setChannelFreqLo(u8 channel, u8 val);
void SID_setChannelFreqHi(u8 channel, u8 val);
void SID_setChannelPulseWidthLo(u8 channel, u8 val);
void SID_setChannelPulseWidthHi(u8 channel, u8 val);
void SID_setChannelAttackDecay(u8 channel, u8 val);
void SID_setChannelSustainRelease(u8 channel, u8 val);
void SID_setChannelNPST(u8 channel, u8 val);
void SID_setFilterCutoffLo(u8 val);
void SID_setFilterCutoffHi(u8 val);
void SID_setFilterResonance(u8 val);
void SID_setModeVolume(u8 val);
u8 SID_getPotentiometerX();
u8 SID_getPotentiometerY();
u8 SID_getEnvelope();

//	6502 PAL runs at 0,985249 MHz
//	We sample at 44100 Hz
//	985249 / 44100 = 22,34
//	Alle 22,34 Cycles brauchen nehmen wir ein Sample

const double pi = std::acos(-1);

class Channel {

	private:
		void tickTriangle(float step, float vol) {
			const float real_freq = floor( freq * 0.0596 );
			float value = (std::abs(0.5f - fmod((real_freq * step), 1.f)) * 4.0 - 1.0f) * vol;
			buffer.push_back(value);
			buffer.push_back(value);
		}
		void tickSawtooth(float step, float vol) {
			const float real_freq = floor( freq * 0.0596 );
			float value = fmod((step * real_freq), 1.0f) * vol;
			buffer.push_back(value);
			buffer.push_back(value);
		}
		void tickPulse(float step, float vol) {
			//printf("Pulse (Rectangle)\n");
			/*const float real_freq = freq * 0.0596;
			float value = (std::sin(2 * pi * step * real_freq)) * vol;
			buffer.push_back(value);
			buffer.push_back(value);*/
			buffer.push_back(0);
			buffer.push_back(0);
		}
		void tickNoise(float step, float vol) {
			//printf("Noise\n");
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
		u8 pot_x = 0x00;
		u8 pot_y = 0x00;
		std::vector<float> buffer;
		void (Channel::*process)(float, float);

		void tick(float steps, float vol) {
			if (enabled) {
				(this->*process)(steps, vol);
			}
			else {
				//	twice, because stereo
				buffer.push_back(0);
				buffer.push_back(0);
			}
		}
		
		void setNPST(u8 val) {
			enabled = val & 1;
			switch ((val >> 4) & 0b1111)
			{
			case 1: 
				printf("Setting channel to Triangle\n");
				process = &Channel::tickTriangle;
				break;
			case 2:
				printf("Setting channel to Sawtooth\n");
				process = &Channel::tickSawtooth;
				break;
			case 4:
				printf("Setting channel to Pulse\n");
				process = &Channel::tickPulse;
				break;
			case 8:
				printf("Setting channel to Noise\n");
				process = &Channel::tickNoise;
				break;
			default:
				//assert(false);
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