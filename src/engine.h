#pragma once
#include <inttypes.h>

typedef struct {
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
} GsSPRITE;

typedef struct {
	unsigned p : 24;
	unsigned char num : 8;
} GsOT_TAG;

typedef struct {
	unsigned short length;
	GsOT_TAG* org;
	unsigned short offset;
	unsigned short point;
	GsOT_TAG* tag;
} GsOT;

typedef struct {
	short pmode;
	short px, py;
	unsigned short pw, ph;
	unsigned long* pixel;
	short cx, cy;
	unsigned short cw, ch;
	unsigned long* clut;
} GsIMAGE;

typedef struct {
	short x, y;
	short w, h;
} RECT;

typedef struct {
	RECT disp;
	RECT screen;
	unsigned char isinter;
	unsigned char isrgb24;
	unsigned char pad0, pad1;
} DISPENV;

extern long MODE_NTSC;
extern long MODE_PAL;

typedef struct {
	int reserved;
} PACKET;

extern uintptr_t _end;
extern uintptr_t _stack_addr;
extern size_t _stack_size;
