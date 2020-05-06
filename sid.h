#include <vector>
#include <cmath>
#include <math.h>
#include <algorithm>
#include <assert.h>
#pragma once
#define SAMPLE_STEPS 22
#define CLOCK_RATE 985250

typedef uint8_t u8;
typedef uint16_t u16;
typedef int8_t i8;
typedef int16_t i16;
typedef uint32_t u32;

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
void SID_setFilterResonance(u8 val);
void SID_setModeVolume(u8 val);
void SID_toggleChannel1();
void SID_toggleChannel2();
void SID_toggleChannel3();
u8 SID_getPotentiometerX();
u8 SID_getPotentiometerY();
u8 SID_getEnvelope();

const float ATTACK[16]		= { 2, 8, 16, 24, 38, 56, 68, 80, 100, 250, 500, 800, 1000, 3000, 5000, 8000 };
const float DECAY[16]		= { 6, 24, 48, 72, 114, 168, 204, 240, 300, 750, 1500, 2400, 3000, 9000, 15000, 24000 };
const float RELEASE[16]		= { 6, 24, 48, 72, 114, 168, 204, 240, 300, 750, 1500, 2400, 3000, 9000, 15000, 24000 };

//	6502 PAL runs at 0,985249 MHz
//	We sample at 44100 Hz
//	985249 / 44100 = 22,34
//	Alle 22,34 Cycles brauchen nehmen wir ein Sample

const double pi = std::acos(-1);

enum class ADSR_MODE {
	ATTACK,
	DECAY,
	SUSTAIN,
	RELEASE
};

class Channel {

	private:
		void tickTriangle(float vol) {
			float step = ticks / (float)CLOCK_RATE;
			float value = (std::abs(0.5f - fmod((real_freq * step), 1.f)) * 4.0 * (-1.0f) + 1.0f) * vol;
			buffer.push_back(value);
			buffer.push_back(value);
		}
		void tickSawtooth(float vol) {
			float step = ticks / (float)CLOCK_RATE;
			float value = fmod((step * real_freq), 1.0f) * vol;
			buffer.push_back(value);
			buffer.push_back(value);
		}
		void tickPulse(float vol) {
			float step = fmod((ticks / (float)CLOCK_RATE * real_freq), 1.0f);
			float value = (step <= ((float)pulse_width / 4096.0)) ? -1.0 * vol : 1.0 * vol;
			buffer.push_back(value);
			buffer.push_back(value);
		}
		void tickNoise(float vol) {
			//printf("Noise\n");
		}
		void tickSilence(float vol) {
			buffer.push_back(0.f);
			buffer.push_back(0.f);
		}

	public: 
		//	config
		u32 ticks = 0;
		bool GATE = false;
		bool SYNC = false;
		bool RING = false;
		bool TEST = false;
		u16 real_freq = 0;					//	Real frequency
		u16 freq = 0x0000;						//	Frequency value
		u16 pulse_width = 0x0000;				//	Pulse width for Rectangle
		u8 attack = 0x00;
		u8 decay = 0x00;
		u8 sustain = 0x00;
		u8 release = 0x00;
		u8 pot_x = 0x00;
		u8 pot_y = 0x00;
		float ADSR_timer = 0.0f;
		ADSR_MODE ADSR_mode = ADSR_MODE::RELEASE;
		std::vector<float> buffer;
		Channel* syncTo_channel;
		void (Channel::* process)(float) = &Channel::tickSilence;
		//	used for SYNC
		float val = 0.0f;
		float prev_val = 0.0f;

		void tick(float vol) {
			//	handle SYNC (if set)
			val = sin(2 * pi * (ticks / (float)CLOCK_RATE) * real_freq);
			if (syncTo_channel->SYNC && ((prev_val < 0 && val >= 0) || (prev_val <= 0 && val > 0))) {
				syncTo_channel->ticks = 0;
			}
			prev_val = val;

			//	handle actual soundwave
			ticks = (ticks + SAMPLE_STEPS) % CLOCK_RATE;
			float adsr_vol = vol;
			ADSR_timer += ((float)SAMPLE_STEPS / (float)CLOCK_RATE) * 1000.f;
			switch (ADSR_mode)
			{
			case ADSR_MODE::ATTACK:
				adsr_vol = (ADSR_timer / ATTACK[attack]) * vol;
				//printf("adsrvol: %f (vol : %f, sustain: %f)\n", adsr_vol, vol, vol / (sustain / 15.f));
				if (ADSR_timer > ATTACK[attack]) {
					ADSR_mode = ADSR_MODE::DECAY;
					ADSR_timer = fmod(ADSR_timer, ATTACK[attack]);
				}
				break;
			case ADSR_MODE::DECAY:
				adsr_vol = vol - (( vol - ( vol * (sustain / 15.0))) * (ADSR_timer / DECAY[decay]));
				//printf("adsrvol: %f (vol : %f, sustain: %f)\n", adsr_vol, vol, vol * (sustain / 15.f));
				if (ADSR_timer > DECAY[decay]) {
					ADSR_mode = ADSR_MODE::SUSTAIN;
					ADSR_timer = 0.0f;
				}
				break;
			case ADSR_MODE::SUSTAIN:
				adsr_vol = vol * (sustain / 15.f);
				break;
			case ADSR_MODE::RELEASE:
 				adsr_vol = std::fmax((float)sustain - std::fmax((float)sustain * (ADSR_timer / RELEASE[release]), 0.0f), 0.0f) / 15.f;
				break;
			default:
				break;
			}
			(this->*process)(pow(adsr_vol, 3));		//	pow 3 is a close-ish estimate to logarithim increase/decrease in volume
		}
		
		void setNPST(u8 val) {
			//	if GATE is set, ATTACK is inited, else RELEASE is inited
			//	This is EDGE-SENSITIVE!
			if (!GATE && val & 0b0001) {
				ADSR_timer = 0.0f;
				ADSR_mode = ADSR_MODE::ATTACK;
			}
			else if(GATE && (val & 0b0001) == 0x0) {
				ADSR_timer = 0.0f;
				ADSR_mode = ADSR_MODE::RELEASE;
			}
			GATE = val & 0b0001;
			SYNC = val & 0b0010;
			RING = val & 0b0100;
			TEST = val & 0b1000;
			switch ((val >> 4) & 0b1111)
			{
			case 1: 
				process = &Channel::tickTriangle;
				break;
			case 2:
				process = &Channel::tickSawtooth;
				break;
			case 4:
				process = &Channel::tickPulse;
				break;
			case 8:
				process = &Channel::tickNoise;
				break;
			default:
				process = &Channel::tickSilence;
				break;
			}
			//	if TEST is set, TICKS are reset, so the oscillator starts fresh
			if (TEST) {
				ticks = 0;
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
			real_freq = floor(freq * 0.0596);				//	set real frequency
		}

		void setFreqHi(u8 val) {
			freq = (freq & 0b0000000011111111) | (val << 8);
			real_freq = floor(freq * 0.0596);				//	set real frequency
		}

		void setPulseWidthLo(u8 val) {
			pulse_width = (pulse_width & 0b1111111100000000) | val;
		}

		void setPulseWidthHi(u8 val) {
			pulse_width = (pulse_width & 0b0000000011111111) | ((val & 0b1111) << 8);
		}

		void setSyncToChannel(Channel* channel) {
			syncTo_channel = channel;
		}
};