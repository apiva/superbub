#pragma once
#include <inttypes.h>

struct GsSPRITE {
	unsigned long attribute;
	short x, y;
	unsigned short w, h;
	unsigned short tpage;
	unsigned char u, v;
	short palette;
	unsigned char r, g, b;
	short mx, my;
	short scalex, scaley;
	long rotate;
};

int MWbload(const char* filename, void* memory);
void GetPadBuf(volatile unsigned char** buf1, volatile unsigned char** buf2);

short SsUtKeyOff(short voice, short vabId, short prog, short tone, short note);
short SsUtKeyOn(short vabId, short prog, short tone, short note, short fine, short voll, short volr);
short SsVabTransfer(void* vh_addr, void* vb_addr, short vabid, short i_flag);
void SsSetMVol(short voll, short volr);
void SsUtAllKeyOff(short mode);
void SsVabClose(short vab_id);
