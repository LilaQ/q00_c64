#pragma once
#include <string>
using namespace std;
void stepPPU(uint8_t cpu_cycles);
void initPPU(string name);
void setRasterIRQlow(uint8_t val);
void setRasterIRQhi(uint8_t val);
uint8_t getCurrentScanline();

struct IRQ_STATUS {
	/*
		Interrupt Request, Bit (1 = an)
		Lesen:
		Bit 7: IRQ durch VIC ausgelöst
		Bit 6..4: unbenutzt
		Bit 3: Anforderung durch Lightpen
		Bit 2: Anforderung durch Sprite-Sprite-Kollision (Reg. $D01E)
		Bit 1: Anforderung durch Sprite-Hintergrund-Kollision (Reg. $D01F)
		Bit 0: Anforderung durch Rasterstrahl (Reg. $D012)
		Schreiben:
		1 in jeweiliges Bit schreiben = zugehöriges Interrupt-Flag löschen
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
		if (val & 0x80)
			irq_caused_by_vic = false;
		if (val & 0x04)
			irq_req_by_sprite_sprite_collision = false;
		if (val & 0x02)
			irq_req_by_sprite_bg_collision = false;
		if (val & 0x01)
			irq_req_by_rasterline = false;
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
		Ist das entsprechende Bit hier und in $D019 gesetzt, wird ein IRQ ausgelöst und Bit 7 in $D019 gesetzt.
		Bit 7..4: unbenutzt
		Bit 3: IRQ wird durch Lightpen ausgelöst
		Bit 2: IRQ wird durch S-S-Kollision ausgelöst
		Bit 1: IRQ wird durch S-H-Kollision ausgelöst
		Bit 0: IRQ wird durch Rasterstrahl ausgelöst.
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

enum GRAPHICMODE {
	LOWRES_TEXTMODE,
	HIRES_BIMAPMODE,
};

void setIRQMask(uint8_t val);
uint8_t getIRQMask();
void clearIRQStatus(uint8_t val);
uint8_t getIRQStatus();