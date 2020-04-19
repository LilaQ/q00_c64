#include <vector>
#include "sid.h"
#include "mmu.h"
#include "SDL2/include/SDL.h"
#define internal static

int SamplesPerSecond = 44100;			//	resolution
bool SoundIsPlaying = false;
std::vector<Channel> channels;
std::vector<float> Mixbuf;
u16 filter_cutoff = 0;
u8 pot_x = 0;
u8 pot_y = 0;
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
	SDL_Init(SDL_INIT_AUDIO);

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

	channels[0].setNPST(0b00010001);
	channels[0].tick();
}

void SID_step() {

	//	set volume
	volume = readFromMem(0xd418) / 15.;

	channels[0].tick();
	channels[1].tick();
	channels[2].tick();

	/*if (SID_V1.buffer.size() >= 100 && SID_V2.buffer.size() >= 100 && SID_V3.buffer.size() >= 100 && SID_V4.buffer.size() >= 100) {

		for (int i = 0; i < 100; i++) {
			float res = 0;
			if (SID_V1.enabled)
				res += SID_V1.buffer.at(i) * volume;
			if (SID_V2.enabled)
				res += SID_V2.buffer.at(i) * volume;
			if (SID_V3.enabled)
				res += SID_V3.buffer.at(i) * volume;
			if (SID_V4.enabled)
				res += SID_V4.buffer.at(i) * volume;
			Mixbuf.push_back(res);
		}
		//	send audio data to device; buffer is times 4, because we use floats now, which have 4 bytes per float, and buffer needs to have information of amount of bytes to be used
		SDL_QueueAudio(1, Mixbuf.data(), Mixbuf.size() * 4);

		SID_V1.buffer.clear();
		SID_V2.buffer.clear();
		SID_V3.buffer.clear();
		SID_V4.buffer.clear();
		Mixbuf.clear();

		//TODO: we could, instead of just idling everything until music buffer is drained, at least call stepPPU(0), to have a constant draw cycle, and maybe have a smoother drawing?
		while (SDL_GetQueuedAudioSize(1) > 4096 * 4) {}
	}*/

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

void SID_setPotentiometerX(u8 val) {
	pot_x = val;
}

void SID_setPotentiometerY(u8 val) {
	pot_y = val;
}