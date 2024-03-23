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

inline void GetPadBuf(volatile unsigned char** buf1, volatile unsigned char** buf2) {
	static std::array<unsigned char, 4096> a, b;
	a.fill(0);
	b.fill(0);
	*buf1 = a.data();
	*buf2 = b.data();
}

inline int MWbload(const char* filename, void* memory) { return 0; }
inline short SsUtKeyOff(short voice, short vabId, short prog, short tone, short note) { return -1; }
inline short SsUtKeyOn(short vabId, short prog, short tone, short note, short fine, short voll, short volr) { return -1; }
inline short SsVabTransfer(void* vh_addr, void* vb_addr, short vabid, short i_flag) { return -1; }
inline void SsSetMVol(short voll, short volr) {}
inline void SsUtAllKeyOff(short mode) {}
inline void SsVabClose(short vab_id) {}
