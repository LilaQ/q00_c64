#include <vector>
#include "sid.h"
#include "mmu.h"
#include "wmu.h"
#include "SDL2/include/SDL.h"
#define internal static
#pragma once

const int SamplesPerSecond = 44770;			//	resolution
const int resolution = 985249 / SamplesPerSecond;
int res_count = 0;
bool SoundIsPlaying = false;
std::vector<Channel> channels;
std::vector<float> Mixbuf;
u16 filter_cutoff = 0;
u8 filter_resonance = 0;
u8 pot_x = 0;
u8 pot_y = 0;
u8 mode = 0;
int cycle_count = 0;
float volume = 0.5;


internal void SDLInitAudio(int32_t SamplesPerSecond, int32_t BufferSize)
{
	SDL_AudioSpec AudioSettings = { 0 };

	AudioSettings.freq = SamplesPerSecond;
	AudioSettings.format = AUDIO_F32SYS;		//	One of the modes that doesn't produce a high frequent pitched tone when having silence
	AudioSettings.channels = 2;
	AudioSettings.samples = BufferSize;

	SDL_OpenAudio(&AudioSettings, 0);

}

void SID_init() {
	SDL_setenv("SDL_AUDIODRIVER", "directsound", 1);
	//SDL_setenv("SDL_AUDIODRIVER", "disk", 1);

	// Open our audio device; Sample Rate will dictate the pace of our synthesizer
	SDLInitAudio(44100, 1024);

	if (!SoundIsPlaying)
	{
		SDL_PauseAudio(0);
		SoundIsPlaying = true;
	}

	channels.push_back(Channel());
	channels.push_back(Channel());
	channels.push_back(Channel());

}

void SID_step(u32 steps) {

	//	get curren position in period
	float cur_step = (float)steps / 985250.0;

	/*if (++c >= 23) {
		c = 0;
		Mixbuf.push_back(sin(2 * pi * cur_step * 940));
		Mixbuf.push_back(sin(2 * pi * cur_step * 940));
		if (Mixbuf.size() >= 100) {
			SDL_QueueAudio(1, Mixbuf.data(), Mixbuf.size() * sizeof(float));
			Mixbuf.clear();
			while (SDL_GetQueuedAudioSize(1) > 4096 * 6) {}
		}
	}*/

	if (++res_count >= 22) {
		res_count = 0;

		//	tick channels
		channels[0].tick(cur_step, volume);
		channels[1].tick(cur_step, volume);
		channels[2].tick(cur_step, volume);

		//printf("cur_step: %f vak %f freq: %f vol: %f\n", cur_step, channels[0].buffer.back(), channels[0].freq * 0.0596, volume );
		if (cur_step == 0)
			printf("BRAP!!\n");

		//	play last 100 samples of all channels
		if (channels[0].buffer.size() >= 100 && channels[1].buffer.size() >= 100 && channels[2].buffer.size() >= 100) {

			for (int i = 0; i < 100; i++) {
				float res = 0;
				res += channels[0].buffer.at(i);
				res += channels[1].buffer.at(i);
				res += channels[2].buffer.at(i);
				Mixbuf.push_back(res);
			}
			//	send audio data to device; buffer is times 4, because we use floats now, which have 4 bytes per float, and buffer needs to have information of amount of bytes to be used
			SDL_QueueAudio(1, Mixbuf.data(), Mixbuf.size() * sizeof(float));
			//SDL_QueueAudio(1, channels[0].buffer.data(), channels[0].buffer.size() * 4);

			channels[0].buffer.clear();
			channels[1].buffer.clear();
			channels[2].buffer.clear();
			Mixbuf.clear();

			//TODO: we could, instead of just idling everything until music buffer is drained, at least call stepPPU(0), to have a constant draw cycle, and maybe have a smoother drawing?
			while (SDL_GetQueuedAudioSize(1) > 4096 * 8) {}
		}
		//printf("%d\n", SDL_GetQueuedAudioSize(1));
	}
}

void SID_setChannelFreqLo(u8 id, u8 val) {
	channels[id].setFreqLo(val);
}

void SID_setChannelFreqHi(u8 id, u8 val) {
	channels[id].setFreqHi(val);
}

void SID_setChannelAttackDecay(u8 id, u8 val) {
	channels[id].setAttackDecay(val);
}

void SID_setChannelSustainRelease(u8 id, u8 val) {
	channels[id].setSustainRelease(val);
}

void SID_setChannelNPST(u8 id, u8 val) {
	channels[id].setNPST(val);
}

void SID_setChannelPulseWidthLo(u8 id, u8 val) {
	channels[id].setPulseWidthLo(val);
}

void SID_setChannelPulseWidthHi(u8 id, u8 val) {
	channels[id].setPulseWidthHi(val);
}

void SID_setFilterCutoffLo(u8 val) {
	filter_cutoff = (filter_cutoff & 0b1111111100000000) | val;
}

void SID_setFilterCutoffHi(u8 val) {
	filter_cutoff = (filter_cutoff & 0b11111111) | (val << 8);
}

void SID_setFilterResonance(u8 val) {
	filter_resonance = val;
}

void SID_setModeVolume(u8 val) {
	volume = (val & 0xf) / 15.;
	mode = val >> 8;
}

u8 SID_getPotentiometerX() {
	return pot_x;
}

u8 SID_getPotentiometerY() {
	return pot_y;
}

u8 SID_getEnvelope() {
	char output[64];
	snprintf(output, sizeof(output), "SID_getEnvelope() not implemented yet!");
	printMsg("SID", "ERROR", string(output));
	//	TODO
	return 0x00;
}

