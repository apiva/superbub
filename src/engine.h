#pragma once
#include <inttypes.h>

struct GsSPRITE {
	unsigned long attribute;
	short x, y;
	unsigned short w, h;
	unsigned short tpage;
	unsigned char u, v;
	short cx, cy;
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

struct GsIMAGE {
	short pmode;
	short px, py;
	unsigned short pw, ph;
	unsigned long* pixel;
	short cx, cy;
	unsigned short cw, ch;
	unsigned long* clut;
};

struct RECT {
	short x, y;
	short w, h;
};

struct DISPENV {
	RECT disp;
	RECT screen;
	unsigned char isinter;
	unsigned char isrgb24;
	unsigned char pad0, pad1;
};

struct PACKET {
	int reserved;
};

extern long MODE_NTSC;
extern long MODE_PAL;
extern uintptr_t _end;
extern uintptr_t _stack_addr;
extern size_t _stack_size;

void InitHeap(void* head, size_t size);
int GsGetActiveBuff();
void GsSetWorkBase(PACKET* base_addr);
void GsClearOt(unsigned short offset, unsigned short point, GsOT* otp);
void SsUtAllKeyOff(short mode);
PACKET* GsGetWorkBase();
void FntPrint(const char* format, ...);
void GsSortClear(unsigned char r, unsigned char g, unsigned char b, GsOT* otp);
void GsDrawOt(GsOT* otp);
void GsSortFastSprite(GsSPRITE* sp, GsOT* otp, unsigned short pri);
short SsUtKeyOff(short voice, short vabId, short prog, short tone, short note);
short SsUtKeyOn(short vabId, short prog, short tone, short note, short fine, short voll, short volr);
void GetPadBuf(volatile unsigned char** buf1, volatile unsigned char** buf2);
void FntLoad(int tx, int ty);
int FntOpen(int x, int y, int w, int h, int isbg, int n);
void SsVabClose(short vab_id);
void SsSetMVol(short voll, short volr);
long SetVideoMode(long mode);
void GsInitGraph(int x_res, int y_res, int intl, int dither, int vram);
void GsDefDispBuff(int x0, int y0, int x1, int y1);
int ClearImage(RECT* recp, unsigned char r, unsigned char g, unsigned char b);
void GsGetTimInfo(unsigned long* tim, GsIMAGE* im);
int MWbload(const char* filename, void* memory);
unsigned short GetTPage(int tp, int abr, int x, int y);
unsigned short GetClut(int x, int y);
int LoadImage(RECT* recp, unsigned long* p);
short SsVabTransfer(void* vh_addr, void* vb_addr, short vabid, short i_flag);
int StoreImage(RECT* recp, unsigned long* p);
int DrawSync(int mode);
int VSync(int mode);
unsigned long* FntFlush(int id);
void GsSwapDispBuff();
