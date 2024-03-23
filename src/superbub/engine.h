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

struct GsOT_TAG {
	unsigned p : 24;
	unsigned char num : 8;
};

struct GsOT {
	unsigned short length;
	GsOT_TAG* org;
	unsigned short offset;
	unsigned short point;
	GsOT_TAG* tag;
};

struct PACKET;
///////////////////////
// Display functions //
///////////////////////

int DrawSync(int mode);
int VSync(int mode);

/////////////////////
// Image functions //
/////////////////////

int MWbload(const char* filename, void* memory);

///////////////////////
// Gamepad functions //
///////////////////////

void GetPadBuf(volatile unsigned char** buf1, volatile unsigned char** buf2);

////////////////////////
// Graphics functions //
////////////////////////

int GsGetActiveBuff();
PACKET* GsGetWorkBase();
void GsClearOt(unsigned short offset, unsigned short point, GsOT* otp);
void GsDrawOt(GsOT* otp);
void GsSetWorkBase(PACKET* base_addr);
void GsSortClear(unsigned char r, unsigned char g, unsigned char b, GsOT* otp);
void GsSortFastSprite(GsSPRITE* sp, GsOT* otp, unsigned short pri);
void GsSwapDispBuff();

/////////////////////
// Audio functions //
/////////////////////

short SsUtKeyOff(short voice, short vabId, short prog, short tone, short note);
short SsUtKeyOn(short vabId, short prog, short tone, short note, short fine, short voll, short volr);
short SsVabTransfer(void* vh_addr, void* vb_addr, short vabid, short i_flag);
void SsSetMVol(short voll, short volr);
void SsUtAllKeyOff(short mode);
void SsVabClose(short vab_id);
