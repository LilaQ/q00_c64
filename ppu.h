#pragma once
#include <string>
using namespace std;

struct IRQ_STATUS {
	/*
		Interrupt Request, Bit (1 = an)
		Lesen:
		Bit 7: IRQ durch VIC ausgel�st
		Bit 6..4: unbenutzt
		Bit 3: Anforderung durch Lightpen
		Bit 2: Anforderung durch Sprite-Sprite-Kollision (Reg. $D01E)
		Bit 1: Anforderung durch Sprite-Hintergrund-Kollision (Reg. $D01F)
		Bit 0: Anforderung durch Rasterstrahl (Reg. $D012)
		Schreiben:
		1 in jeweiliges Bit schreiben = zugeh�riges Interrupt-Flag l�schen
	*/
	bool irq_caused_by_vic = false;
	bool irq_req_by_sprite_sprite_collision = false;
	bool irq_req_by_sprite_bg_collision = false;
	bool irq_req_by_rasterline = false;

	void setFlags(uint8_t val) {
		irq_caused_by_vic = (val & 0x80) == 0x80;
		irq_req_by_sprite_sprite_collision = (val & 0x04) == 0x04;
		irq_req_by_sprite_bg_collision = (val & 0x02) == 0x02;
		irq_req_by_rasterline = (val & 0x01) == 0x01;
	}

	void clearFlags(uint8_t val) {
		//printf("clearing IRQ flags %x\n", val);
		if (val & 0x04)
			irq_req_by_sprite_sprite_collision = false;
		if (val & 0x02)
			irq_req_by_sprite_bg_collision = false;
		if (val & 0x01)
			irq_req_by_rasterline = false;
		//	clear "caused by VIC" flag when other flags are off
		if (!irq_req_by_sprite_sprite_collision &&
			!irq_req_by_sprite_bg_collision &&
			!irq_req_by_rasterline
			) {
			irq_caused_by_vic = false;
		}
	}

	uint8_t get() {
		uint8_t ret = 0x00;
		ret |= (uint8_t)irq_caused_by_vic << 7;
		ret |= (uint8_t)irq_req_by_sprite_sprite_collision << 2;
		ret |= (uint8_t)irq_req_by_sprite_bg_collision << 1;
		ret |= (uint8_t)irq_req_by_rasterline;
		return ret;
	}
};

struct IRQ_MASK {
	/*
		Interrupt Request: Maske, Bit (1 = an)
		Ist das entsprechende Bit hier und in $D019 gesetzt, wird ein IRQ ausgel�st und Bit 7 in $D019 gesetzt.
		Bit 7..4: unbenutzt
		Bit 3: IRQ wird durch Lightpen ausgel�st
		Bit 2: IRQ wird durch S-S-Kollision ausgel�st
		Bit 1: IRQ wird durch S-H-Kollision ausgel�st
		Bit 0: IRQ wird durch Rasterstrahl ausgel�st.
	*/
	bool irq_can_be_cause_by_sprite_sprite = false;
	bool irq_can_be_cause_by_sprite_bg = false;
	bool irq_can_be_cause_by_rasterline = false;

	void set(uint8_t val) {
		irq_can_be_cause_by_sprite_sprite = (val & 0x04) == 0x04;
		irq_can_be_cause_by_sprite_bg = (val & 0x02) == 0x02;
		irq_can_be_cause_by_rasterline = (val & 0x01) == 0x01;
	}

	uint8_t get() {
		uint8_t ret = 0x00;
		ret |= (uint8_t)irq_can_be_cause_by_sprite_sprite << 2;
		ret |= (uint8_t)irq_can_be_cause_by_sprite_bg << 1;
		ret |= (uint8_t)irq_can_be_cause_by_rasterline;
		return ret;
	}
};

enum class GRAPHICMODE {
	LOWRES_TEXTMODE,
	HIRES_BIMAPMODE,
};

enum class SCREEN_POS {
	SCREEN,
	BORDER_LR,
	BORDER_TB,
	NO_RENDER,
	ROW_38_AREA,
	COL_38_AREA
};

enum class COL_MODE {
	COL_38,
	COL_40
};

enum class UI_SCREEN_SIZE {
	ORIG,
	X2
};

void stepPPU(uint8_t cpu_cycles);
void initPPU(string name);
void writeVICregister(uint16_t adr, uint8_t val);
uint8_t readVICregister(uint16_t adr);
void setScreenSize(UI_SCREEN_SIZE scr_s);


//	DEBUG
void logDraw();
uint8_t readVICregister(uint16_t adr);
uint8_t currentScanline();


//	REWRITE
bool VIC_isBadline();
void VIC_dataRefresh();
void VIC_nextScanline();
void VIC_fetchGraphicsData(uint8_t amount);
void VIC_fetchSpritePointer(uint8_t sprite_nr);
bool VIC_checkRasterIRQ();
uint8_t VIC_getCycle();
void VIC_tick();