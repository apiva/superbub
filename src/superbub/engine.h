#pragma once
#include <inttypes.h>
#include <array>

class Tim;

struct GsSPRITE {
	const Tim* tim;
	unsigned long attribute;
	short x, y;
	unsigned short w, h;
	unsigned char u, v;
	short palette;
	unsigned char r, g, b;
};
