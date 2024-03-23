//
//	SUPER BUB CONTEST
//
//  Copyright (c) 1998 Alex Herbert
//


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "engine.h"
#include "qsincos.h"
#include <raylib.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <tim.hpp>
#include <map>
#include <gflags/gflags.h>

// Logs //

static std::shared_ptr<spdlog::sinks::sink> consoleSink;
static std::shared_ptr<spdlog::logger> raylibLog;
static std::shared_ptr<spdlog::logger> timLog;
static std::shared_ptr<spdlog::logger> gameLog;

// Types //



typedef struct {
	unsigned short mode, mode2, timer;
	short color;
	short dest_y;
	unsigned short len;
	struct {
		short x, y;
		short vx, vy;
		unsigned char u, v;
	} chr[12];
} ANI_TEXT;


#define MAP_W 8
#define MAP_H 14
#define MAP_H2 (MAP_H-3)

#define MAX_EATERS 32
#define MAX_ELIST (MAP_W*MAP_H)

typedef struct {
	unsigned short mode;
	unsigned short elist_pos, elist_end;
	unsigned short color;
	unsigned short bright;
	unsigned short x, y;
	short vx, vy;
	struct {
		unsigned char x, y;
	} elist[MAX_ELIST];
} EATER;

typedef struct {
	void (*mode_func)(unsigned short);
	unsigned short type;
	unsigned short timer;
	unsigned short pause;
	unsigned short wins;
	unsigned short score;
	struct {
		unsigned short select_flag;
		unsigned short type;
		unsigned short anim;
		short oy, vy;
	} monster;
	struct {
		unsigned short id;
		unsigned short oy;
		unsigned short ecolor;
	} map[MAP_H][MAP_W];
	unsigned short depth[MAP_W];
	unsigned short min_depth, max_depth;
	unsigned short empty;
	unsigned short eaters;
	EATER eater[MAX_EATERS];
	unsigned short chains;
	unsigned short eaten;
	unsigned short attack;
	unsigned short dump, dump_x, disp_dump;
	struct {
		unsigned short pos;
		unsigned short oy;
	} seq;
	struct {
		unsigned short time;
		unsigned short pause;
		unsigned short x;
		short ox;
		unsigned short id1, id2;
		unsigned short move, rotate;
	} aim;
	struct {
		unsigned short x, y;
		unsigned short oy;
	} magic[2];
	struct {
		unsigned short timer1, timer2;
		unsigned short dest_x;
		unsigned short rotate;
	} ai;
} PLAYER;


typedef struct {
	unsigned short brains;
	unsigned short think_time;
	unsigned short rotate_time;
	unsigned short fire_time;
} LEVEL;


typedef const struct {
	char* name;
	unsigned short color1, color2;
} RANKING;


// Prototypes //

void init_intro(void);
void mode_intro(void);
void init_title(void);
void mode_title(void);
void mode_select_players(void);
void mode_select_level(void);
void draw_title(short, short, unsigned short);
void mode_select_monster(void);
void plmode_select1(unsigned short);
void plmode_select2(unsigned short);
void plmode_select3(unsigned short);
void plmode_select4(unsigned short);
void mode_winner1(void);
void mode_winner2(void);
void init_ranking(void);
void mode_ranking1(void);
void mode_ranking2(void);
void init_rematch(void);
void mode_rematch(void);
void init_gameover(void);
void mode_gameover(void);
void display_monsticon(short, short, unsigned short, unsigned short, unsigned short);
void mode_play(void);
void update_player(unsigned short);
void plmode_load(unsigned short);
void cpu_player(unsigned short);
void plmode_aim_v(unsigned short);
void plmode_aim_h(unsigned short);
void poll_player(unsigned short);
void plmode_fire(unsigned short);
void plmode_firemagic(unsigned short);
void plmode_magic(unsigned short);
void plmode_cfloat(unsigned short);
void plmode_float(unsigned short);
void plmode_ceat(unsigned short);
void plmode_eat1(unsigned short);
void plmode_eat2(unsigned short);
void plmode_cdump(unsigned short);
void plmode_dump(unsigned short);
void lose_round(unsigned short);
void plmode_win(unsigned short);
void plmode_lose(unsigned short);
unsigned short get_match(unsigned short, unsigned short, unsigned short, unsigned short);
unsigned short get_score(unsigned short, unsigned short, unsigned short, unsigned short);
void get_map_info(unsigned short);
void draw_map(unsigned short);
void draw_sequence(unsigned short);
void draw_bubble(short, short, unsigned short);
void display_stats(unsigned short);
void draw_timebar(unsigned short);
void draw_foreground(void);
void draw_background(void);
void init_game(void);
void init_battle(void);
void init_round(void);
unsigned short random_bubble(void);
void kill_fx(void);
void create_magicfx(short, short, unsigned short);
void create_bubblefx(short, short);
void update_fx(void);
void kill_anitext(void);
void init_anitext(unsigned short, short, char*, short, unsigned short, unsigned short, unsigned short);
void update_anitext(void);
void display_text(short, short, char*, unsigned short);
void display_text2(short, short, char*, short);
void read_pads(void);
void process_pads(void);
void start_sfx(unsigned short);
void update_sfx(void);
void update_music(void);
void initialize(void);
void init_tims(void);
void init_sound(void);
void init_sprite(const Tim*, GsSPRITE*);
short load_vab(void*, char*, char*);

void draw_sprite(const GsSPRITE*);
void flush_sprites();

void get_pad_buffers(unsigned char** pad1, unsigned char** pad2);
void fetch_input();

DEFINE_bool(debug, false, "Enable debug");
DEFINE_bool(psm_characters, false, "Use PSM character images");
DEFINE_string(export_tims, "", "Export converted TIMs to specified folder");
DEFINE_string(import_tims, "", "Use the images in the specified folder to replace the TIMs");

// Screen & GPU Stuff //

#define SCREEN_W 384
#define SCREEN_H 256

#define SCREEN_W2 (SCREEN_W>>1)
#define SCREEN_H2 (SCREEN_H>>1)
#define SCREEN_W4 (SCREEN_W>>2)
#define SCREEN_H4 (SCREEN_H>>2)

unsigned long frame = 0;

// Controller Stuff //

#define PAD_L2 0x0001
#define PAD_R2 0x0002
#define PAD_L1 0x0004
#define PAD_R1 0x0008

#define PAD_TRIANGLE 0x0010
#define PAD_CIRCLE 0x0020
#define PAD_X 0x0040
#define PAD_SQUARE 0x0080

#define PAD_SELECT 0x0100
#define PAD_UNUSED1 0x0200
#define PAD_UNUSED2 0x0400
#define PAD_START 0x0800

#define PAD_UP 0x1000
#define PAD_RIGHT 0x2000
#define PAD_DOWN 0x4000
#define PAD_LEFT 0x8000

struct {
	unsigned char* buffer;
	unsigned char connected;
	unsigned char type;
	unsigned short keys;
	unsigned short previous;
	unsigned short pulse;
	unsigned short repeat;
	unsigned short timer;
} pad[2];

unsigned short master_pad;


// Dynamic Loading Stuff //

#define LOADAREA_SIZE 0x40000

void* load_addr;


// Audio Stuff //

#define VH0_SIZE 0x4000
#define VH1_SIZE 0x4000

struct {
	short id;
	void* vh_addr;
} vab[2];

#define MAX_SFX 18

#define SFX_FIRE 0
#define SFX_EAT 2
#define SFX_DUMP 4
#define SFX_ROTATE 6
#define SFX_PANIC 8
#define SFX_MONSTER 10
#define SFX_LOSE 14
#define SFX_DEAD 15
#define SFX_MAGIC 16

#define SFXMODE_MONO 1
#define SFXMODE_POLY 2
#define SFXMODE_SUSTAIN 3
#define SFXMODE_BLOCK 4

struct {
	unsigned short type;
	short prog, note;
	unsigned short timer;
} sfx_chan[24];

struct {
	short prog, note;
	short voll, volr;
	unsigned short time;
	unsigned short mode;
} sfx_tab[MAX_SFX] = {
	0,61,120,120,6,SFXMODE_MONO,		// fire (player 0)
	0,60,120,120,6,SFXMODE_MONO,		// fire (player 1)
	1,62,100,100,9,SFXMODE_SUSTAIN,		// eat (player 0)
	1,60,100,100,9,SFXMODE_SUSTAIN,		// eat (player 1)
	2,62,120,120,35,SFXMODE_MONO,		// dump (player 0)
	2,60,120,120,35,SFXMODE_MONO,		// dump (player 1)
	3,61,100,100,4,SFXMODE_MONO,		// rotate (player 0)
	3,60,100,100,4,SFXMODE_MONO,		// rotate (player 1)
	4,60,100,100,21,SFXMODE_MONO,		// panic (player 0)
	5,60,100,100,27,SFXMODE_MONO,		// panic (player 1)
	6,60,120,120,16,SFXMODE_POLY,		// monster 0
	7,60,120,120,31,SFXMODE_POLY,		// monster 1
	8,60,120,120,17,SFXMODE_POLY,		// monster 2
	9,60,120,120,26,SFXMODE_POLY,		// monster 3
	10,60,120,120,20,SFXMODE_MONO,		// lose
	11,60,120,120,15,SFXMODE_MONO,		// dead
	12,62,100,100,35,SFXMODE_MONO,		// magic (player 0)
	12,60,100,100,35,SFXMODE_MONO,		// magic (player 1)
};

#define MAX_MUSIC 24

struct {
	unsigned short step;
	unsigned short next;
	short chan[3];
} music;

struct {
	short prog;
	short vol;
} music_tab[MAX_MUSIC][3] = {
	9,100,	-1,0,	-1,0,	// step 0
	7,100,	-1,0,	-1,0,	// step 1
	7,100,	-1,0,	0,90,	// step 2
	8,127,	5,90,	0,90,	// step 3
	8,127,	5,90,	2,90,	// step 4
	7,95,	-1,0,	2,90,	// step 5
	10,127,	5,90,	3,90,	// step 6
	8,127,	5,90,	3,90,	// step 7
	8,127,	5,90,	4,50,	// step 8
	8,127,	5,90,	4,60,	// step 9
	6,120,	-1,0,	4,65,	// step 10
	11,127,	-1,0,	-1,0,	// step 11
	11,127,	-1,0,	2,70,	// step 12
	11,127,	4,30,	2,80,	// step 13
	10,127, 4,40,	2,90,	// step 14
	6,120,	4,50,	3,90,	// step 15
	8,127,	5,90,	1,60,	// step 16
	8,127,	5,90,	1,70,	// step 17
	10,127,	5,90,	1,75,	// step 18
	6,120,	3,60,	1,60,	// step 19
	9,100,	3,70,	1,50,	// step 20
	7,100,	3,80,	1,60,	// step 21
	8,127,	5,90,	1,70,	// step 22
	8,127,	5,90,	1,75,	// step 23
};


// Font Stuff //

Tim font_tim, bigfont_tim;
GsSPRITE font_spt, bigfont_spt;

#define ATMODE_NULL 0
#define ATMODE_UP 1
#define ATMODE_DOWN 2
#define ATMODE_HOLD 3
#define ATMODE_SPREAD 4

ANI_TEXT ani_text[3];


// Title Stuff //

Tim title_tim;
GsSPRITE title_spt;

char* title_name[3] = { "CONTEST","RATING","CHAMPION" };

#define DEMO_DELAY 500
#define DEMO_TIME 3000


// Options Stuff //

struct {
	unsigned short players;
	unsigned short level;
} option = { 1,1 };


// Ranking Stuff //

RANKING rank_tab[11] = {
	"GOLD",12,2,
	"SILVER",12,7,
	"BRONZE",2,1,
	"4TH",2,6,
	"5TH",2,7,
	"6TH",2,8,
	"7TH",3,8,
	"8TH",4,8,
	"9TH",5,8,
	"10TH",6,8,
	"UNCLASSIFIED",1,0
};


// Game Screen Stuff //

Tim screen_tim[2];
GsSPRITE screen_spt[7];

Tim bg_tim;
GsSPRITE bg_spt;
unsigned short bg_bright;

Tim timebar_tim;
GsSPRITE timebar_spt;

Tim wincup_tim;
GsSPRITE wincup_spt;


// Game Stuff //

struct {
	void (*mode_func)(void);
	unsigned short timer1, timer2;
	unsigned short players, level, colors;
	unsigned short round;
	unsigned short winner, loser;
	unsigned short rank;
} game;


// Player Stuff //

#define PLTYPE_CPU 0
#define PLTYPE_HUMAN 1

#define PLMOVE_NULL 0
#define PLMOVE_LEFT 1
#define PLMOVE_RIGHT 2

#define EMODE_NULL 0
#define EMODE_EAT 1
#define EMODE_FADE 2

PLAYER player[2];

#define SEQUENCE_LENGTH 400

unsigned char sequence[SEQUENCE_LENGTH];

#define MAX_PLANIM 6
#define PLANIM_NORMAL 0
#define PLANIM_WIN 1
#define PLANIM_DEAD 2
#define PLANIM_PANIC 3
#define PLANIM_TIME 4
#define PLANIM_NEARLY 5

const unsigned short anim_tab[MAX_PLANIM][16] = {
	0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,
	0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,2,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	4,5,4,5,4,5,4,5,4,5,4,5,4,5,4,5,
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
	0,0,0,0,0,0,7,7,7,7,7,7,7,7,7,7,
};

char* player_name[3] = { "PLAYER 1","PLAYER 2","CPU" };


// AI Level Stuff //

#define MAX_LEVEL 16

LEVEL level_tab[MAX_LEVEL] = {
	15,100,30,50,	// level 0 (easy)
	20,90,28,45,	// level 1
	25,80,26,40,	// level 2
	30,70,24,35,	// level 3
	40,50,22,30,	// level 4
	50,40,20,25,	// level 5
	60,30,15,20,	// level 6
	70,20,10,15,	// level 7
	30,40,18,30,	// level 8 (normal/hard)
	40,35,16,25,	// level 9
	50,30,14,20,	// level 10
	60,25,12,15,	// level 11
	70,20,10,10,	// level 12
	80,15,8,8,		// level 13
	90,12,5,5,		// level 14
	100,10,4,4,		// level 15
};


// Monster Stuff //

Tim monsticon_tim;
GsSPRITE monsticon_spt;

Tim monster_tim[4];
GsSPRITE monster_spt[4];

char* monster_name[4] = { "BOBO","GRIZZ","CHUMP","JASPER" };


// Bubble Stuff //

Tim bubble_tim[3];
GsSPRITE bubble_spt[3];
Tim eater_tim;
GsSPRITE eater_spt;

#define BLANK_ID 0
#define BUBBLE_ID 1
#define FLAME_ID 6
#define MAGIC_ID 11

#define BLANK_TYPE 0xffff
#define BUBBLE_TYPE 0
#define FLAME_TYPE 1
#define MAGIC_TYPE 2

const struct {
	unsigned short type;
	unsigned short color;
} id_tab[12] = {
	BLANK_TYPE,0,
	BUBBLE_TYPE,1,
	BUBBLE_TYPE,2,
	BUBBLE_TYPE,3,
	BUBBLE_TYPE,4,
	BUBBLE_TYPE,5,
	FLAME_TYPE,1,
	FLAME_TYPE,2,
	FLAME_TYPE,3,
	FLAME_TYPE,4,
	FLAME_TYPE,5,
	MAGIC_TYPE,0
};

const unsigned short score_tab[12][12] = {
	0,0,0,0,0,0,0,0,0,0,0,0,
	0,1,0,0,0,0,3,0,0,0,0,0,
	0,0,1,0,0,0,0,3,0,0,0,0,
	0,0,0,1,0,0,0,0,3,0,0,0,
	0,0,0,0,1,0,0,0,0,3,0,0,
	0,0,0,0,0,1,0,0,0,0,3,0,
	0,3,0,0,0,0,0,0,0,0,0,0,
	0,0,3,0,0,0,0,0,0,0,0,0,
	0,0,0,3,0,0,0,0,0,0,0,0,
	0,0,0,0,3,0,0,0,0,0,0,0,
	0,0,0,0,0,3,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0
};


// Effects Stuff //

#define MAX_MAGICFX 256

struct {
	short bright;
	unsigned short speed;
	short x, y;
} magicfx[MAX_MAGICFX];

unsigned short next_magicfx = 0;

Tim bubblefx_tim;
GsSPRITE bubblefx_spt;

#define MAX_BUBBLEFX 256

struct {
	short bright;
	short x, y;
	short vx, vy;
} bubblefx[MAX_BUBBLEFX];

unsigned short next_bubblefx = 0;



// Functions //

int main(int argc, char* argv[]) {
	gflags::ParseCommandLineFlags(&argc, &argv, false);

	initialize();

	frame = 0;

	init_intro();

	//	printf("Main Loop.\n");

	while (!WindowShouldClose()) {
		fetch_input();

		read_pads();
		if (pad[0].keys == (PAD_SELECT | PAD_START)) {
			SsUtAllKeyOff(0);
			return(0);
		}
		process_pads();
		if (pad[0].pulse == (PAD_L1 | PAD_R1))
			FLAGS_debug = !FLAGS_debug;

		update_music();
		update_sfx();

		update_anitext();

		(*game.mode_func)();

		gameLog->trace("FRAME: {}", frame);

		Camera2D camera;
		memset(&camera, 0, sizeof(Camera2D));

		const float actual_height = GetScreenHeight();
		const float factor = actual_height / float(SCREEN_H);
		const float actual_width = factor * float(SCREEN_W);

		camera.offset.x = (GetScreenWidth() - actual_width) / 2;
		camera.zoom = float(GetScreenHeight()) / float(SCREEN_H);

		BeginDrawing();
		ClearBackground(BLACK);

		BeginScissorMode(camera.offset.x, 0, actual_width, actual_height);
		BeginMode2D(camera);
		flush_sprites();
		EndMode2D();
		EndScissorMode();

		EndDrawing();

		frame++;
	}
}



void init_intro(void) {
	kill_fx();
	kill_anitext();
	music.next = 0;
	game.mode_func = mode_intro;
	game.timer1 = 0;
}


void mode_intro(void) {
	unsigned short ox, oy;

	game.timer1++;
	if (game.timer1 >= 350) {
		oy = (game.timer1 - 350) << 3;
		draw_title(SCREEN_W2, SCREEN_H + 40 - oy, 0);
		if (game.timer1 == 350)
			start_sfx(SFX_DUMP);
		if (game.timer1 >= 382)
			init_title();
	}
	else {
		oy = 0;
	}
	if (game.timer1 > 50) {
		display_text(SCREEN_W2, oy + 28, "MEET", 4);
		if (game.timer1 > 100) {
			display_text2(SCREEN_W2, oy + 52, monster_name[0], 12);
			ox = (strlen(monster_name[0]) + 1) << 3;
			monster_spt[0].x = SCREEN_W2 - ox - 32;
			monster_spt[0].y = oy + 52 - 4;
			monster_spt[0].u = anim_tab[PLANIM_NORMAL][(frame >> 1) & 15] << 5;
			monster_spt[0].palette = 1;
			draw_sprite(&monster_spt[0]);
			monster_spt[0].x = SCREEN_W2 + ox;
			monster_spt[0].palette++;
			draw_sprite(&monster_spt[0]);
			if (game.timer1 > 150) {
				display_text2(SCREEN_W2, oy + 84, monster_name[1], 12);
				ox = (strlen(monster_name[1]) + 1) << 3;
				monster_spt[1].x = SCREEN_W2 - ox - 32;
				monster_spt[1].y = oy + 84 - 4;
				monster_spt[1].u = anim_tab[PLANIM_NORMAL][(frame >> 1) & 15] << 5;
				monster_spt[1].palette = 1;
				draw_sprite(&monster_spt[1]);
				monster_spt[1].x = SCREEN_W2 + ox;
				monster_spt[1].palette++;
				draw_sprite(&monster_spt[1]);
				if (game.timer1 > 200) {
					display_text2(SCREEN_W2, oy + 116, monster_name[2], 12);
					ox = (strlen(monster_name[2]) + 1) << 3;
					monster_spt[2].x = SCREEN_W2 - ox - 32;
					monster_spt[2].y = oy + 116 - 4;
					monster_spt[2].u = anim_tab[PLANIM_NORMAL][(frame >> 1) & 15] << 5;
					monster_spt[2].palette = 1;
					draw_sprite(&monster_spt[2]);
					monster_spt[2].x = SCREEN_W2 + ox;
					monster_spt[2].palette++;
					draw_sprite(&monster_spt[2]);
					display_text(SCREEN_W2, oy + 148, "AND", 4);
					if (game.timer1 > 250) {
						display_text2(SCREEN_W2, oy + 172, monster_name[3], 12);
						ox = (strlen(monster_name[3]) + 1) << 3;
						monster_spt[3].x = SCREEN_W2 - ox - 32;
						monster_spt[3].y = oy + 172 - 4;
						monster_spt[3].u = anim_tab[PLANIM_NORMAL][(frame >> 1) & 15] << 5;
						monster_spt[3].palette = 1;
						draw_sprite(&monster_spt[3]);
						monster_spt[3].x = SCREEN_W2 + ox;
						monster_spt[3].palette++;
						draw_sprite(&monster_spt[3]);
						if (game.timer1 > 300) {
							display_text(SCREEN_W2, oy + 204, "IN THE", 4);
						}
					}
				}
			}
		}
	}

	if ((pad[0].pulse | pad[1].pulse) & (PAD_START | PAD_X))
		init_title();

	update_fx();
	draw_background();
}


void init_title(void) {
	kill_fx();
	kill_anitext();
	music.next = 0;
	game.mode_func = mode_title;
	game.timer1 = 0;
}


void mode_title(void) {
	if (frame & 32)
		display_text(SCREEN_W2, 182, "PRESS START", 1);

	if (frame & 256)
		display_text(SCREEN_W2, 232, "COPYRIGHT 1998 ALEX HERBERT", 4);
	else
		display_text(SCREEN_W2, 232, "VERSION 1.01.PAL.CW", 4);

	draw_title(SCREEN_W2, 40, 0);

	update_fx();
	draw_background();

	if (++game.timer1 > DEMO_DELAY) {
		game.players = 0;
		init_game();
	}
	else {
		if ((pad[0].pulse | pad[1].pulse) & (PAD_START | PAD_X)) {
			game.mode_func = mode_select_players;
			start_sfx(SFX_ROTATE);
		}
	}
}


void mode_select_players(void) {
	unsigned short p;

	display_text(SCREEN_W2, 172, "1 PLAYER", (option.players == 1 && (frame & 8)) ? 1 : 0);
	display_text(SCREEN_W2, 192, "2 PLAYERS", (option.players == 2 && (frame & 8)) ? 1 : 0);

	draw_title(SCREEN_W2, 40, 0);

	update_fx();
	draw_background();

	for (p = 0; p < 2; p++) {
		if (pad[p].pulse & PAD_UP && option.players > 1) {
			option.players = 1;
			start_sfx(SFX_ROTATE);
		}
		if (pad[p].pulse & PAD_DOWN && option.players < 2) {
			option.players = 2;
			start_sfx(SFX_ROTATE);
		}
		if (pad[p].pulse & (PAD_START | PAD_X)) {
			master_pad = p;
			game.players = option.players;
			if (option.players == 1) {
				game.mode_func = mode_select_level;
				start_sfx(SFX_ROTATE);
			}
			else {
				init_game();
			}
			break;
		}
	}
}


void mode_select_level(void) {
	display_text(SCREEN_W2, 162, "EASY", (option.level == 0 && (frame & 8)) ? 1 : 0);
	display_text(SCREEN_W2, 182, "NORMAL", (option.level == 1 && (frame & 8)) ? 1 : 0);
	display_text(SCREEN_W2, 202, "HARD", (option.level == 2 && (frame & 8)) ? 1 : 0);

	draw_title(SCREEN_W2, 40, 0);

	update_fx();
	draw_background();

	if (pad[master_pad].pulse & PAD_UP && option.level > 0) {
		option.level--;
		start_sfx(SFX_ROTATE);
	}
	if (pad[master_pad].pulse & PAD_DOWN && option.level < 2) {
		option.level++;
		start_sfx(SFX_ROTATE);
	}
	if (pad[master_pad].pulse & PAD_TRIANGLE) {
		game.mode_func = mode_select_players;
		start_sfx(SFX_FIRE);
	}
	if (pad[master_pad].pulse & (PAD_START | PAD_X)) {
		init_game();
	}
}


void draw_title(short x, short y, unsigned short type) {
	unsigned short i;
	unsigned short c;

	display_text2(x, y, "SUPER", (frame & 4) ? 11 : 12);

	title_spt.x = x - 21 - 44;
	title_spt.y = y + (qsin(((frame << 7) + 2048 + 32) & 4095) >> 9) + 25;
	title_spt.u = 0;
	title_spt.v = 0;
	title_spt.palette = 1;
	for (i = 0; i < 46; i++) {
		c = ((frame + i) & 31) << 3;
		if (c >= 128)
			c = 256 - c;
		title_spt.r = title_spt.g = title_spt.b = c + 32;
		draw_sprite(&title_spt);
		title_spt.y++;
		title_spt.v++;
	}

	title_spt.x = x - 21;
	title_spt.y = y + (qsin(((frame << 7) + 1024 + 32) & 4095) >> 9) + 25;
	title_spt.u = 42;
	title_spt.v = 0;
	title_spt.palette = 2;
	for (i = 0; i < 46; i++) {
		c = ((frame - i) & 31) << 3;
		if (c >= 128)
			c = 256 - c;
		title_spt.r = title_spt.g = title_spt.b = c + 32;
		draw_sprite(&title_spt);
		title_spt.y++;
		title_spt.v++;
	}

	title_spt.x = x - 21 + 44;
	title_spt.y = y + (qsin(((frame << 7) + 32) & 4095) >> 9) + 25;
	title_spt.u = 0;
	title_spt.v = 0;
	title_spt.palette = 3;
	for (i = 0; i < 46; i++) {
		c = ((frame + i) & 31) << 3;
		if (c >= 128)
			c = 256 - c;
		title_spt.r = title_spt.g = title_spt.b = c + 32;
		draw_sprite(&title_spt);
		title_spt.y++;
		title_spt.v++;
	}

	display_text2(x, y + 72, title_name[type], -1);

	for (i = 0; i < 4; i++) {
		monster_spt[i].x = x + (qcos(((frame << 6) + (i << 10)) & 4095) >> 6) - 18;
		monster_spt[i].y = y + (qsin(((frame << 6) + (i << 10)) & 4095) >> 6) + 34;
		monster_spt[i].u = anim_tab[PLANIM_NORMAL][(frame >> 1) & 15] << 5;
		monster_spt[i].palette = 1;
		draw_sprite(&monster_spt[i]);
	}
}


void mode_select_monster(void) {
	unsigned short p;
	char txt[12];

	for (p = 0; p < 2; p++)
		(*player[p].mode_func)(p);
	if (player[0].monster.select_flag && player[1].monster.select_flag) {
		if (player[0].timer >= 100 && player[1].timer > 100)
			init_round();
	}
	else {
		if (game.timer1 > 0)
			game.timer1--;
	}
	if (game.timer1 <= 250) {
		sprintf(txt, "%d", (game.timer1 + 49) / 50);
		display_text2(SCREEN_W2, SCREEN_H - 40, txt, 0);
	}
	update_fx();
	draw_background();
}


void plmode_select1(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short sx = p * SCREEN_W2 + SCREEN_W4;
	short x, y;

	y = 184 + pp->timer * 10;
	display_text2(sx, y, monster_name[pp->monster.type], 12);
	x = (p == 0) ? sx - (pp->timer << 4) : sx + (pp->timer << 4);
	display_monsticon(x, 64, pp->monster.type, p + 1, 128);
	if (--pp->timer == 0) {
		if (pp->monster.select_flag) {
			pp->mode_func = plmode_select4;
			start_sfx(SFX_MONSTER + pp->monster.type);
		}
		else {
			pp->aim.move = PLMOVE_NULL;
			pp->mode_func = plmode_select2;
		}
	}
}


void plmode_select2(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short sx = p * SCREEN_W2 + SCREEN_W4;

	if (frame & 8)
		display_text(sx, 120, "< SELECT >", 1);
	display_text2(sx, 184, monster_name[pp->monster.type], 12);
	display_monsticon(sx, 64, pp->monster.type, p + 1, 128);
	if (pad[p].keys & PAD_LEFT) {
		pp->aim.move = PLMOVE_LEFT;
		pp->mode_func = plmode_select3;
		pp->timer = 0;
		start_sfx(SFX_FIRE + p);
	}
	if (pad[p].keys & PAD_RIGHT) {
		pp->aim.move = PLMOVE_RIGHT;
		pp->mode_func = plmode_select3;
		pp->timer = 0;
		start_sfx(SFX_FIRE + p);
	}
	if (pad[p].pulse & PAD_X || game.timer1 == 0) {
		pp->monster.select_flag = 1;
		pp->mode_func = plmode_select4;
		pp->timer = 0;
		start_sfx(SFX_MONSTER + pp->monster.type);
	}
}


void plmode_select3(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short sx = p * SCREEN_W2 + SCREEN_W4;
	short x, y;

	y = 184 + pp->timer * 10;
	display_text2(sx, y, monster_name[pp->monster.type], 12);
	x = (p == 0) ? sx - (pp->timer << 4) : sx + (pp->timer << 4);
	display_monsticon(x, 64, pp->monster.type, p + 1, 128);
	if (++pp->timer >= 16) {
		switch (pp->aim.move) {
		case PLMOVE_LEFT:
			pp->monster.type += 3;
			break;
		case PLMOVE_RIGHT:
			pp->monster.type++;
			break;
		}
		pp->monster.type %= 4;
		pp->mode_func = plmode_select1;
		pp->timer = 16;
	}
}


void plmode_select4(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short sx = p * SCREEN_W2 + SCREEN_W4;
	unsigned short bright;

	pp->timer++;
	display_text2(sx, 184, monster_name[pp->monster.type], -1);
	bright = (frame << 3) & 255;
	if (bright >= 128)
		bright = 256 - bright;
	display_monsticon(sx, 64, pp->monster.type, p + 1, bright + 64);
}


void mode_winner1(void) {
	PLAYER* pp = &player[game.winner];
	short x;
	unsigned short type;

	x = SCREEN_W2 + (game.timer1 << 4);
	type = (pp->type == PLTYPE_HUMAN) ? game.winner : 2;
	display_text2(x, 48, player_name[type], (frame & 4) ? 0 : 2);
	display_text2(x, 184, monster_name[pp->monster.type], 12);
	x = SCREEN_W2 - (game.timer1 << 4);
	display_monsticon(x, 64, pp->monster.type, game.winner + 1, 128);
	create_bubblefx((rand() % SCREEN_W) - 4, (rand() % SCREEN_H) - 4);
	create_magicfx((rand() % SCREEN_W) - 8, (rand() % SCREEN_H) - 8, 4);
	if (game.timer1 > 0) {
		if (--game.timer1 == 0) {
			init_anitext(0, 112, "WINNER!", -1, ATMODE_DOWN, ATMODE_SPREAD, 145);
			init_anitext(2, 112, "WINNER!", -1, ATMODE_UP, ATMODE_SPREAD, 150);
		}
	}
	else {
		if (ani_text[2].mode == ATMODE_SPREAD)
			game.mode_func = mode_winner2;
	}
	update_fx();
	draw_background();
}


void mode_winner2(void) {
	PLAYER* pp = &player[game.winner];
	short x;
	unsigned short type;

	game.timer1++;
	x = SCREEN_W2 - (game.timer1 << 4);
	type = (pp->type == PLTYPE_HUMAN) ? game.winner : 2;
	display_text2(x, 48, player_name[type], (frame & 4) ? 0 : 2);
	display_text2(x, 184, monster_name[pp->monster.type], 12);
	x = SCREEN_W2 + (game.timer1 << 4);
	display_monsticon(x, 64, pp->monster.type, game.winner + 1, 128);
	if (ani_text[2].mode == ATMODE_NULL) {
		if (game.players == 1) {
			if (pp->type == PLTYPE_HUMAN) {
				if (++player[game.loser].monster.type < 4)
					init_battle();
				else
					init_ranking();
			}
			else {
				init_ranking();
			}
		}
		else {
			init_rematch();
		}
	}
	update_fx();
	draw_background();
}


void init_ranking(void) {
	game.rank = 8 - player[master_pad].score + player[1 - master_pad].score;
	game.mode_func = mode_ranking1;
	game.timer1 = 16;
	game.timer2 = 200;
}


void mode_ranking1(void) {
	RANKING* rp = &rank_tab[game.rank];

	draw_title(SCREEN_W2 - (game.timer1 << 4), 48, (player[master_pad].score < 8) ? 1 : 2);
	display_text2(SCREEN_W2 + (game.timer1 << 4), 192, rp->name, (frame & 8) ? rp->color1 : rp->color2);

	if (game.timer1 > 0) {
		game.timer1--;
	}
	else {
		if (--game.timer2 == 0)
			game.mode_func = mode_ranking2;
	}

	update_fx();
	draw_background();
}


void mode_ranking2(void) {
	RANKING* rp = &rank_tab[game.rank];

	draw_title(SCREEN_W2 + (game.timer1 << 4), 48, (player[master_pad].score == 8) ? 1 : 2);
	display_text2(SCREEN_W2 - (game.timer1 << 4), 192, rp->name, (frame & 8) ? rp->color1 : rp->color2);

	if (++game.timer1 == 16)
		init_gameover();

	update_fx();
	draw_background();
}


void init_rematch(void) {
	init_anitext(1, 96, "RE-MATCH?", 12, ATMODE_UP, ATMODE_SPREAD, 0);
	game.mode_func = mode_rematch;
	game.timer1 = 500;
}


void mode_rematch(void) {
	char txt[12];

	sprintf(txt, "%d", (game.timer1 + 49) / 50);
	display_text2(SCREEN_W2, SCREEN_H - 40, txt, 0);
	switch (ani_text[1].mode) {
	case ATMODE_HOLD:
		if (game.timer1 > 0) {
			game.timer1--;
			if ((pad[0].pulse | pad[1].pulse) & (PAD_START | PAD_X)) {
				ani_text[1].mode = ani_text[1].mode2;
			}
			if ((pad[0].pulse | pad[1].pulse) & PAD_TRIANGLE)
				game.timer1 -= game.timer1 % 50;
		}
		else {
			ani_text[1].mode = ani_text[1].mode2;
		}
		break;
	case ATMODE_NULL:
		if (game.timer1 > 0)
			init_game();
		else
			init_gameover();
		break;
	}
	update_fx();
	draw_background();
}


void init_gameover(void) {
	music.next = music.step + 1;
	init_anitext(1, 96, "GAME OVER", 1, ATMODE_UP, ATMODE_SPREAD, 150);
	game.mode_func = mode_gameover;
}


void mode_gameover(void) {
	if (ani_text[1].mode == ATMODE_NULL)
		init_intro();
	update_fx();
	draw_background();
}


void display_monsticon(short x, short y, unsigned short type, unsigned short color, unsigned short bright) {
	monsticon_spt.x = x - 64;
	monsticon_spt.y = y;
	monsticon_spt.u = (type & 1) << 7;
	monsticon_spt.v = (type & 2) << 6;
	monsticon_spt.palette = color;
	monsticon_spt.r = monsticon_spt.g = monsticon_spt.b = bright;
	draw_sprite(&monsticon_spt);
}


void mode_play(void) {
	unsigned short p;

	music.next = (MAP_H << 1) - ((player[0].empty + player[1].empty) / MAP_W) + 1;

	bubble_spt[1].u = ((frame >> 2 & 3) << 4);
	eater_spt.u = ((frame >> 1 & 3) << 5);

	for (p = 0; p < 2; p++) {
		update_player(p);
		display_stats(p);
		draw_timebar(p);
	}

	if (game.players == 0) {
		if (frame & 16)
			display_text2(SCREEN_W2, 124, "DEMO MODE", 11);
		if (++game.timer1 > DEMO_TIME) {
			init_intro();
		}
		else {
			if ((pad[0].pulse | pad[1].pulse) & (PAD_START | PAD_X))
				init_title();
		}
	}

	update_fx();
	draw_foreground();
	for (p = 0; p < 2; p++) {
		draw_map(p);
		draw_sequence(p);
	}
	draw_background();
}


void update_player(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short sx = p * SCREEN_W2 + 32;

	if (pp->pause) {
		pp->pause--;
	}
	else {
		(*pp->mode_func)(p);
	}

	if (pp->monster.oy < 0 || pp->monster.vy < 0) {
		pp->monster.oy += pp->monster.vy++;
		if (pp->monster.oy >= 0)
			pp->monster.oy = pp->monster.vy = 0;
	}
	monster_spt[pp->monster.type].x = (pp->aim.x << 4) + pp->aim.ox + sx - 24;
	monster_spt[pp->monster.type].y = ((MAP_H2 + 1) << 4) + (pp->monster.oy) + 16;
	monster_spt[pp->monster.type].u = anim_tab[pp->monster.anim][(frame >> 1) & 15] << 5;
	monster_spt[pp->monster.type].palette = p + 1;
	draw_sprite(&monster_spt[pp->monster.type]);
	pp->aim.ox /= 2;
}


void plmode_ready(unsigned short p) {
	PLAYER* pp = &player[p];

	if (ani_text[1].mode == ATMODE_SPREAD) {
		init_anitext(p << 1, 96, "GO!", -1, ATMODE_DOWN, ATMODE_SPREAD, 25);
		pp->mode_func = plmode_load;
		pp->pause = 25;
	}
}


void plmode_load(unsigned short p) {
	PLAYER* pp = &player[p];

	get_map_info(p);
	if (pp->max_depth <= MAP_H2) {
		if (pp->empty <= 50)
			pp->monster.anim = PLANIM_NEARLY;
		else
			pp->monster.anim = PLANIM_NORMAL;
		pp->chains = 0;
		pp->attack = 0;
		pp->aim.time = 512;
		pp->aim.pause = 0;
		pp->aim.id2 = sequence[pp->seq.pos++];
		pp->aim.id1 = sequence[pp->seq.pos++];
		pp->seq.pos %= SEQUENCE_LENGTH;
		pp->seq.oy = 16;
		pp->mode_func = plmode_aim_v;
		if (pp->type == PLTYPE_CPU)
			cpu_player(p);
	}
	else {
		lose_round(p);
	}
}


void cpu_player(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short brains = level_tab[game.level].brains;
	unsigned short x;
	unsigned short ox;
	unsigned short x1, y1, x2, y2;
	unsigned short score;
	unsigned short top_score = 0;

	pp->ai.dest_x = rand() % (MAP_W - 1);
	pp->ai.rotate = rand() & 3;
	if (pp->aim.id1 < MAGIC_ID) {
		ox = rand() % (MAP_W - 1);
		for (x = 0; x < MAP_W - 1; x++) {
			x1 = (ox + x) % (MAP_W - 1);
			x2 = x1 + 1;
			y1 = pp->depth[x1];
			y2 = pp->depth[x2];
			if (rand() % 100 < brains) {
				pp->map[y1][x1].id = pp->aim.id1;
				pp->map[y2][x2].id = pp->aim.id2;
				score = get_score(p, x1, y1, pp->aim.id1) + get_score(p, x2, y2, pp->aim.id2);
				if (score > top_score) {
					top_score = score;
					pp->ai.dest_x = x1;
					pp->ai.rotate = 1;
				}
			}
			if (rand() % 100 < brains) {
				pp->map[y1][x1].id = pp->aim.id2;
				pp->map[y2][x2].id = pp->aim.id1;
				score = get_score(p, x1, y1, pp->aim.id2) + get_score(p, x2, y2, pp->aim.id1);
				if (score > top_score) {
					top_score = score;
					pp->ai.dest_x = x1;
					pp->ai.rotate = 3;
				}
			}
			pp->map[y1][x1].id = 0;
			pp->map[y2][x2].id = 0;
		}
		ox = rand() % MAP_W;
		for (x = 0; x < MAP_W; x++) {
			x1 = (ox + x) % MAP_W;
			y1 = pp->depth[x1];
			y2 = y1 + 1;
			if (rand() % 100 < brains) {
				pp->map[y1][x1].id = pp->aim.id1;
				pp->map[y2][x1].id = pp->aim.id2;
				score = get_score(p, x1, y1, pp->aim.id1) + get_score(p, x1, y2, pp->aim.id2);
				if (score > top_score) {
					top_score = score;
					pp->ai.dest_x = x1;
					pp->ai.rotate = 0;
				}
			}
			if (rand() % 100 < brains) {
				pp->map[y1][x1].id = pp->aim.id2;
				pp->map[y2][x1].id = pp->aim.id1;
				score = get_score(p, x1, y1, pp->aim.id2) + get_score(p, x1, y2, pp->aim.id1);
				if (score > top_score) {
					top_score = score;
					pp->ai.dest_x = x1;
					pp->ai.rotate = 2;
				}
			}
			pp->map[y1][x1].id = 0;
			pp->map[y2][x1].id = 0;
		}
	}
	else {
		ox = rand() % (MAP_W - 1);
		for (x = 0; x < MAP_W - 1; x++) {
			x1 = (ox + x) % (MAP_W - 1);
			x2 = x1 + 1;
			y1 = pp->depth[x1];
			y2 = pp->depth[x2];
			if (y1 > 0 && y2 > 0) {
				score = score_tab[pp->map[y1 - 1][x1].id][pp->map[y2 - 1][x2].id] + 1;
				if (score > top_score) {
					top_score = score;
					pp->ai.dest_x = x1;
					pp->ai.rotate = 1;
				}
			}
		}
	}
	pp->ai.timer1 = pp->ai.timer2 = 0;
}


void plmode_aim_v(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short sx = p * SCREEN_W2 + 32;
	unsigned short x, y;
	unsigned short temp;

	x = pp->aim.x;
	draw_bubble((x << 4) + pp->aim.ox + sx, ((MAP_H2) << 4) + 24, pp->aim.id1);
	draw_bubble((x << 4) + pp->aim.ox + sx, ((MAP_H2 + 1) << 4) + 24, pp->aim.id2);

	if (--pp->aim.time == 50)
		pp->monster.anim = PLANIM_TIME;

	poll_player(p);
	if (pp->aim.pause == 0) {
		switch (pp->aim.move) {
		case PLMOVE_LEFT:
			if (x > 0) {
				pp->aim.x = x - 1;
				pp->aim.ox = 12;
				pp->aim.pause = 8;
			}
			break;
		case PLMOVE_RIGHT:
			if (pp->aim.x < MAP_W - 1) {
				pp->aim.x = x + 1;
				pp->aim.ox = -12;
				pp->aim.pause = 8;
			}
			break;
		}
	}
	else {
		pp->aim.pause--;
	}
	switch (pp->aim.rotate) {
	case PLMOVE_LEFT:
		pp->mode_func = plmode_aim_h;
		start_sfx(SFX_ROTATE + p);
		break;
	case PLMOVE_RIGHT:
		temp = pp->aim.id1;
		pp->aim.id1 = pp->aim.id2;
		pp->aim.id2 = temp;
		pp->mode_func = plmode_aim_h;
		start_sfx(SFX_ROTATE + p);
		break;
	}
	if (pp->aim.time == 0) {
		if (pp->aim.id1 == MAGIC_ID) {
			pp->magic[0].x = pp->magic[1].x = x;
			y = pp->depth[x];
			pp->magic[0].y = y;
			pp->magic[1].y = y + 1;
			pp->magic[0].oy = pp->magic[1].oy = ((MAP_H2 - y) << 1) + 1;
			pp->mode_func = plmode_firemagic;
		}
		else {
			y = pp->depth[x];
			pp->map[y][x].id = pp->aim.id1;
			pp->map[y][x].oy = ((MAP_H2 - y) << 2) + 2;
			y++;
			pp->map[y][x].id = pp->aim.id2;
			pp->map[y][x].oy = ((MAP_H2 + 1 - y) << 2) + 2;
			pp->mode_func = plmode_fire;
		}
		pp->monster.anim = PLANIM_NORMAL;
		pp->monster.oy = 0;
		pp->monster.vy = -6;
		start_sfx(SFX_FIRE + p);
	}
}


void plmode_aim_h(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short sx = p * SCREEN_W2 + 32;
	unsigned short x, y;
	unsigned short temp;

	x = pp->aim.x;
	if (x > MAP_W - 2)
		x--;
	draw_bubble((x << 4) + pp->aim.ox + sx, ((MAP_H2 + 1) << 4) + 16, pp->aim.id1);
	draw_bubble(((x + 1) << 4) + pp->aim.ox + sx, ((MAP_H2 + 1) << 4) + 16, pp->aim.id2);

	if (--pp->aim.time == 50)
		pp->monster.anim = PLANIM_TIME;

	poll_player(p);
	if (pp->aim.pause == 0) {
		switch (pp->aim.move) {
		case PLMOVE_LEFT:
			if (x > 0) {
				pp->aim.x = x - 1;
				pp->aim.ox = 12;
				pp->aim.pause = 8;
			}
			break;
		case PLMOVE_RIGHT:
			if (pp->aim.x < MAP_W - 1) {
				pp->aim.x++;
				if (pp->aim.x < MAP_W - 1)
					pp->aim.ox = -12;
				pp->aim.pause = 8;
			}
			break;
		}
	}
	else {
		pp->aim.pause--;
	}
	switch (pp->aim.rotate) {
	case PLMOVE_LEFT:
		temp = pp->aim.id1;
		pp->aim.id1 = pp->aim.id2;
		pp->aim.id2 = temp;
		pp->mode_func = plmode_aim_v;
		start_sfx(SFX_ROTATE + p);
		break;
	case PLMOVE_RIGHT:
		pp->mode_func = plmode_aim_v;
		start_sfx(SFX_ROTATE + p);
		break;
	}
	if (pp->aim.time == 0) {
		if (pp->aim.id1 == MAGIC_ID) {
			pp->magic[0].x = x;
			y = pp->depth[x];
			pp->magic[0].y = y;
			pp->magic[0].oy = (MAP_H2 + 1 - y) << 1;
			x++;
			pp->magic[1].x = x;
			y = pp->depth[x];
			pp->magic[1].y = y;
			pp->magic[1].oy = (MAP_H2 + 1 - y) << 1;
			pp->mode_func = plmode_firemagic;
		}
		else {
			y = pp->depth[x];
			pp->map[y][x].id = pp->aim.id1;
			pp->map[y][x].oy = (MAP_H2 + 1 - y) << 2;
			x++;
			y = pp->depth[x];
			pp->map[y][x].id = pp->aim.id2;
			pp->map[y][x].oy = (MAP_H2 + 1 - y) << 2;
			pp->mode_func = plmode_fire;
		}
		pp->monster.anim = PLANIM_NORMAL;
		pp->monster.oy = 0;
		pp->monster.vy = -6;
		start_sfx(SFX_FIRE + p);
	}
}


void poll_player(unsigned short p) {
	PLAYER* pp = &player[p];
	LEVEL* lp = &level_tab[game.level];

	pp->aim.move = PLMOVE_NULL;
	pp->aim.rotate = PLMOVE_NULL;
	switch (pp->type) {
	case PLTYPE_HUMAN:
		if (pad[p].keys & PAD_LEFT)
			pp->aim.move = PLMOVE_LEFT;
		if (pad[p].keys & PAD_RIGHT)
			pp->aim.move = PLMOVE_RIGHT;
		if (pad[p].pulse & PAD_SQUARE)
			pp->aim.rotate = PLMOVE_LEFT;
		if (pad[p].pulse & PAD_CIRCLE)
			pp->aim.rotate = PLMOVE_RIGHT;
		if (pad[p].pulse & PAD_X)
			pp->aim.time = 0;
		break;
	case PLTYPE_CPU:
		if (pp->ai.dest_x < pp->aim.x) {
			if (++pp->ai.timer1 >= lp->think_time)
				pp->aim.move = PLMOVE_LEFT;
		}
		else {
			if (pp->ai.dest_x > pp->aim.x) {
				if (++pp->ai.timer1 >= lp->think_time)
					pp->aim.move = PLMOVE_RIGHT;
			}
			else {
				if (pp->ai.rotate) {
					if (++pp->ai.timer2 >= lp->rotate_time) {
						pp->aim.rotate = PLMOVE_LEFT;
						pp->ai.rotate--;
						pp->ai.timer2 = 0;
					}
				}
				else {
					if (++pp->ai.timer2 >= lp->fire_time)
						pp->aim.time = 0;
				}
			}
		}
		break;
	}
}


void plmode_fire(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short x, y;
	unsigned short updated = 0;

	for (y = 0; y < MAP_H; y++) {
		for (x = 0; x < MAP_W; x++) {
			if (pp->map[y][x].oy) {
				pp->map[y][x].oy -= 2;
				updated = 1;
			}
		}
	}
	if (updated == 0) {
		pp->mode_func = plmode_ceat;
	}
}


void plmode_firemagic(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short sx = p * SCREEN_W2 + 32;
	unsigned short m;
	unsigned short updated = 0;

	for (m = 0; m < 2; m++) {
		if (pp->magic[m].oy > 0) {
			pp->magic[m].oy--;
			updated = 1;
		}
		create_magicfx((pp->magic[m].x << 4) + sx, (pp->magic[m].y << 4) + (pp->magic[m].oy << 3) + 16, 8);
	}
	if (updated == 0) {
		pp->mode_func = plmode_magic;
		start_sfx(SFX_MAGIC + p);
	}
}


void plmode_magic(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short sx = p * SCREEN_W2 + 32;
	unsigned short x, y;
	unsigned short id1, id2;

	id1 = MAGIC_ID;
	y = pp->magic[0].y;
	if (y > 0) {
		x = pp->magic[0].x;
		if (pp->map[y - 1][x].id)
			id1 = pp->map[y - 1][x].id;
	}
	id2 = MAGIC_ID;
	y = pp->magic[1].y;
	if (y > 0) {
		x = pp->magic[1].x;
		if (pp->map[y - 1][x].id)
			id2 = pp->map[y - 1][x].id;
	}

	for (y = 0; y < MAP_H; y++) {
		for (x = 0; x < MAP_W; x++) {
			if (pp->map[y][x].id == id1 || pp->map[y][x].id == id2) {
				pp->map[y][x].id = 0;
				pp->map[y][x].ecolor = 0;
				pp->eaten++;
				create_magicfx((x << 4) + sx, (y << 4) + 16, 4);
			}
		}
	}
	pp->chains++;
	pp->mode_func = plmode_cfloat;
	pp->pause = 16;
}


void plmode_cfloat(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short x, y;
	unsigned short id;
	unsigned short updated = 0;

	for (y = 1; y < MAP_H; y++) {
		for (x = 0; x < MAP_W; x++) {
			id = pp->map[y][x].id;
			if (id && pp->map[y - 1][x].id == 0) {
				pp->map[y - 1][x].id = id;
				pp->map[y - 1][x].oy = 4;
				pp->map[y][x].id = 0;
				updated = 1;
			}
		}
	}
	if (updated) {
		pp->mode_func = plmode_float;
	}
	else {
		pp->mode_func = plmode_cdump;
	}
}


void plmode_float(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short x, y;
	unsigned short updated = 0;

	for (y = 0; y < MAP_H; y++) {
		for (x = 0; x < MAP_W; x++) {
			if (pp->map[y][x].oy) {
				if (--(pp->map[y][x].oy) == 0 && y > 0 && pp->map[y - 1][x].id == 0) {
					pp->map[y - 1][x].id = pp->map[y][x].id;
					pp->map[y - 1][x].oy = 4;
					pp->map[y][x].id = 0;
				}
				updated = 1;
			}
		}
	}
	if (updated == 0) {
		pp->mode_func = plmode_ceat;
	}
}


void plmode_ceat(unsigned short p) {
	PLAYER* pp = &player[p];
	EATER* ep;
	unsigned short x, y;
	unsigned short id;
	unsigned short color;
	unsigned short eat_flag;

	pp->eaters = 0;
	pp->eaten = 0;
	for (y = 0; y < MAP_H; y++) {
		for (x = 0; x < MAP_W; x++) {
			id = pp->map[y][x].id;
			color = id_tab[id].color;
			if (id_tab[id].type == FLAME_TYPE) {
				eat_flag = 0;
				id = pp->map[y - 1][x].id;
				if (y > 0 && id_tab[id].type == BUBBLE_TYPE && id_tab[id].color == color) {
					eat_flag = 1;
				}
				else {
					id = pp->map[y][x + 1].id;
					if (x < MAP_W - 1 && id_tab[id].type == BUBBLE_TYPE && id_tab[id].color == color) {
						eat_flag = 1;
					}
					else {
						id = pp->map[y + 1][x].id;
						if (y < MAP_H - 1 && id_tab[id].type == BUBBLE_TYPE && id_tab[id].color == color) {
							eat_flag = 1;
						}
						else {
							id = pp->map[y][x - 1].id;
							if (x > 0 && id_tab[id].type == BUBBLE_TYPE && id_tab[id].color == color) {
								eat_flag = 1;
							}
						}
					}
				}

				if (eat_flag && pp->eaters < MAX_EATERS) {
					ep = &pp->eater[pp->eaters++];
					ep->mode = EMODE_EAT;
					ep->elist_pos = 0;
					ep->elist_end = 0;
					ep->color = color;
					ep->bright = 16;
					ep->x = x;
					ep->y = y;
				}
				color = 0xffff; // for flames
			}
			pp->map[y][x].ecolor = color;
		}
	}

	if (pp->eaters) {
		pp->mode_func = plmode_eat1;
		pp->chains++;
	}
	else {
		pp->mode_func = plmode_cdump;
	}
}


void plmode_eat1(unsigned short p) {
	PLAYER* pp = &player[p];
	EATER* ep;
	unsigned short sx = p * SCREEN_W2 + 32;
	unsigned short e;
	unsigned short x, y;
	unsigned short color;
	unsigned short elp, ele;
	unsigned short eat_flag = 0;

	for (e = 0; e < pp->eaters; e++) {
		ep = &pp->eater[e];

		color = ep->color;
		x = ep->x;
		y = ep->y;
		switch (ep->mode) {
		case EMODE_EAT:
			pp->map[y][x].id = 0;

			ele = ep->elist_end;
			if (y > 0 && pp->map[y - 1][x].ecolor == color) {
				pp->map[y - 1][x].ecolor = 0;
				ep->elist[ele].x = x;
				ep->elist[ele].y = y - 1;
				ele++;
			}
			if (x < MAP_W - 1 && pp->map[y][x + 1].ecolor == color) {
				pp->map[y][x + 1].ecolor = 0;
				ep->elist[ele].x = x + 1;
				ep->elist[ele].y = y;
				ele++;
			}
			if (y < MAP_H - 1 && pp->map[y + 1][x].ecolor == color) {
				pp->map[y + 1][x].ecolor = 0;
				ep->elist[ele].x = x;
				ep->elist[ele].y = y + 1;
				ele++;
			}
			if (x > 0 && pp->map[y][x - 1].ecolor == color) {
				pp->map[y][x - 1].ecolor = 0;
				ep->elist[ele].x = x - 1;
				ep->elist[ele].y = y;
				ele++;
			}
			ep->elist_end = ele;

			elp = ep->elist_pos;
			if (elp < ele) {
				ep->x = ep->elist[elp].x;
				ep->y = ep->elist[elp].y;
				ep->vx = x - ep->x;
				ep->vy = y - ep->y;
				elp++;
				start_sfx(SFX_EAT + p);
			}
			else {
				pp->eaten += elp;
				ep->mode = EMODE_FADE;
				ep->vx = 0;
				ep->vy = 0;
			}
			ep->elist_pos = elp;
			break;
		case EMODE_FADE:
			if (--ep->bright == 0)
				ep->mode = EMODE_NULL;
			break;
		}

		if (ep->mode) {
			eater_spt.x = (x << 4) + sx - 8;
			eater_spt.y = (y << 4) + 16 - 8;
			eater_spt.palette = color;
			eater_spt.r = eater_spt.g = eater_spt.b = ep->bright << 3;
			draw_sprite(&eater_spt);
			eat_flag = 1;
		}
	}

	if (eat_flag) {
		pp->mode_func = plmode_eat2;
		pp->timer = 7;
	}
	else {
		pp->mode_func = plmode_cfloat;
		pp->attack += pp->chains * (pp->eaten + 1);
	}
}


void plmode_eat2(unsigned short p) {
	PLAYER* pp = &player[p];
	EATER* ep;
	unsigned short sx = p * SCREEN_W2 + 32;
	unsigned short x, y;
	unsigned short e;

	for (e = 0; e < pp->eaters; e++) {
		ep = &pp->eater[e];

		switch (ep->mode) {
		case EMODE_FADE:
			if (--ep->bright == 0)
				ep->mode = EMODE_NULL;
		case EMODE_EAT:
			x = (ep->x << 4) + ((ep->vx * pp->timer) << 1) + sx;
			y = (ep->y << 4) + ((ep->vy * pp->timer) << 1) + 16;
			eater_spt.x = x - 8;
			eater_spt.y = y - 8;
			eater_spt.palette = ep->color;
			eater_spt.r = eater_spt.g = eater_spt.b = ep->bright << 3;
			draw_sprite(&eater_spt);
			if (ep->mode == EMODE_EAT)		// silly, but works
				create_bubblefx(x + 4, y + 4);
			break;
		}
	}

	if (--pp->timer == 0) {
		pp->mode_func = plmode_eat1;
	}
}


void plmode_cdump(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short x, y;
	unsigned short id;
	char txt[12];

	if (pp->attack > 2) {
		player[1 - p].dump += pp->attack - 2;	// attack other player
		if (pp->chains >= 2) {
			sprintf(txt, "%d CHAINS", pp->chains);
			init_anitext(p << 1, 96, txt, -1, ATMODE_UP, ATMODE_SPREAD, 25);
		}
	}
	if (pp->dump) {
		get_map_info(p);
		x = pp->dump_x;
		while (pp->dump && pp->empty > (MAP_H - MAP_H2) * MAP_W) {
			x = (x + 1) % MAP_W;
			y = pp->depth[x];
			if (y < MAP_H2) {
				do {	// may need to restrict number of loops
					id = random_bubble();
				} while (get_match(p, x, y, id));
				pp->map[y][x].id = id;
				pp->map[y][x].oy = ((MAP_H - pp->min_depth) << 2) + (x << 1);
				pp->depth[x]++;
				pp->empty--;
				pp->dump--;
				pp->dump_x = x;
			}
		}
		if (pp->dump) {
			while (pp->dump && pp->empty) {
				x = (x + 1) % MAP_W;
				y = pp->depth[x];
				if (y < MAP_H) {
					pp->map[y][x].id = random_bubble();
					pp->map[y][x].oy = ((MAP_H - pp->min_depth) << 2) + (x << 1);
					pp->depth[x]++;
					pp->empty--;
					pp->dump--;
					pp->dump_x = x;
				}
			}
			start_sfx(SFX_MONSTER + pp->monster.type);
		}
		pp->mode_func = plmode_dump;
		pp->monster.anim = PLANIM_PANIC;
		start_sfx(SFX_PANIC + p);
	}
	else {
		pp->mode_func = plmode_load;
	}
}


void plmode_dump(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short x, y;
	unsigned short updated = 0;

	for (y = 0; y < MAP_H; y++) {
		for (x = 0; x < MAP_W; x++) {
			if (pp->map[y][x].oy) {
				pp->map[y][x].oy -= 2;
				updated = 1;
			}
		}
	}
	if (updated == 0) {
		pp->disp_dump = pp->dump;
		pp->mode_func = plmode_load;
		start_sfx(SFX_DUMP + p);
	}
}


void lose_round(unsigned short p) {
	game.winner = 1 - p;
	game.loser = p;

	if (player[p].type == PLTYPE_CPU)
		game.level++;

	init_anitext(p << 1, 96, "LOSER!", 1, ATMODE_DOWN, ATMODE_SPREAD, 100);
	player[p].mode_func = plmode_lose;
	player[p].monster.anim = PLANIM_PANIC;
	player[p].monster.vy = -12;
	start_sfx(SFX_LOSE);

	init_anitext(game.winner << 1, 96, "WINNER!", -1, ATMODE_UP, ATMODE_SPREAD, 100);
	player[game.winner].mode_func = plmode_win;
	player[game.winner].wins++;
	player[game.winner].score++;
	player[game.winner].monster.anim = PLANIM_WIN;
}


void plmode_win(unsigned short p) {
	if (ani_text[p << 1].mode == ATMODE_NULL) {
		if (game.players) {
			if (player[p].wins >= 2) {
				music.next = music.step + 1;
				game.mode_func = mode_winner1;
				game.timer1 = 16;
			}
			else {
				init_round();
			}
		}
		else {
			init_intro();
		}
	}
}


void plmode_lose(unsigned short p) {
	PLAYER* pp = &player[p];

	if (pp->monster.oy == 0 && pp->monster.anim != PLANIM_DEAD) {
		pp->monster.anim = PLANIM_DEAD;
		start_sfx(SFX_DEAD);
	}
}


unsigned short get_match(unsigned short p, unsigned short x, unsigned short y, unsigned short id) {
	PLAYER* pp = &player[p];
	unsigned short match = 0;

	if (x > 0 && score_tab[id][pp->map[y][x - 1].id] > 1)
		return(1);
	if (y > 0 && score_tab[id][pp->map[y - 1][x].id] > 1)
		return(1);
	if (x < MAP_W - 1 && score_tab[id][pp->map[y][x + 1].id]>1)
		return(1);
	return(0);
}


unsigned short get_score(unsigned short p, unsigned short x, unsigned short y, unsigned short id) {
	PLAYER* pp = &player[p];
	unsigned short score;
	unsigned short shift = p + 1;

	score = MAP_H - y;
	if (x > 0)
		score += score_tab[id][pp->map[y][x - 1].id] << 2;
	if (y > 0)
		score += score_tab[id][pp->map[y - 1][x].id] << 2;
	if (x < MAP_W - 1)
		score += score_tab[id][pp->map[y][x + 1].id] << 2;
	if (y < MAP_H - 1)
		score += score_tab[id][pp->map[y + 1][x].id] << 2;
	if (y > 1)
		score += score_tab[id][pp->map[y - 2][x].id];
	if (y >= MAP_H2)
		score >>= 2;
	return(score);
}


void get_map_info(unsigned short p) {
	PLAYER* pp = &player[p];

	unsigned short x, y;

	pp->min_depth = MAP_H;
	pp->max_depth = 0;
	pp->empty = 0;
	for (x = 0; x < MAP_W; x++) {
		for (y = 0; y < MAP_H && pp->map[y][x].id; y++) { /* do nothing */ }
		pp->depth[x] = y;
		if (y < pp->min_depth)
			pp->min_depth = y;
		if (y > pp->max_depth)
			pp->max_depth = y;
		pp->empty += MAP_H - y;
	}
}


void draw_map(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short sx = p * SCREEN_W2 + 32;
	unsigned short x, y;

	for (y = 0; y < MAP_H; y++) {
		for (x = 0; x < MAP_W; x++)
			draw_bubble((x << 4) + sx, (y << 4) + (pp->map[y][x].oy << 2) + 16, pp->map[y][x].id);
	}
}


void draw_sequence(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short sx = (p == 0) ? SCREEN_W2 - 24 : SCREEN_W2 + 8;
	unsigned short s;

	if (pp->seq.oy)
		pp->seq.oy--;
	for (s = 0; s < 5; s++)
		draw_bubble(sx, 216 - (s << 4) - (pp->seq.oy << 1), sequence[(pp->seq.pos + s) % SEQUENCE_LENGTH]);
}


void draw_bubble(short x, short y, unsigned short id) {
	unsigned short type, color;

	if (id) {
		type = id_tab[id].type;
		if (type == BUBBLE_TYPE)
			bubble_spt[0].u = ((((frame >> 3) + (x >> 4) + (y >> 4)) & 3) << 4);
		color = id_tab[id].color;
		bubble_spt[type].x = x;
		bubble_spt[type].y = y;
		bubble_spt[type].palette = color;
		bubble_spt[type].r = bubble_spt[type].g = bubble_spt[type].b = 128;
		draw_sprite(&bubble_spt[type]);
	}
}


void display_stats(unsigned short p) {
	PLAYER* pp = &player[p];
	unsigned short sx = (p == 0) ? SCREEN_W2 - 24 : SCREEN_W2 + 8;
	char txt[4];

	if (pp->dump > pp->disp_dump)
		pp->disp_dump = pp->dump;
	if (pp->disp_dump > 99)
		pp->disp_dump = 99;

	sprintf(txt, "%2d", pp->disp_dump);
	display_text(sx + 8, 24, txt, 2);

	sprintf(txt, "%2d", pp->score);
	display_text(sx + 8, 80, txt, 3);
	switch (pp->wins) {
	case 1:
		wincup_spt.x = sx;
		wincup_spt.y = 104;
		draw_sprite(&wincup_spt);
		break;
	case 2:
		wincup_spt.x = sx - 4;
		wincup_spt.y = 104;
		draw_sprite(&wincup_spt);
		wincup_spt.x += 8;
		draw_sprite(&wincup_spt);
		break;
	}
}


void draw_timebar(unsigned short p) {
	unsigned short sx = p * SCREEN_W2 + 32;
	unsigned short ti;

	ti = player[p].aim.time >> 3;
	if (ti) {
		timebar_spt.x = 64 - ti + sx;
		timebar_spt.w = ti << 1;
		timebar_spt.u = 64 - ti;
		timebar_spt.v = frame >> 1 & 63;
		draw_sprite(&timebar_spt);
	}
}


void draw_foreground(void) {
	unsigned short s;

	for (s = 0; s < 7; s++)
		draw_sprite(&screen_spt[s]);
}


void draw_background(void) {
	short ox, oy;
	unsigned short x, y;

	if (music.step >= 16 && (frame % 50) == 25)
		bg_bright = 12;

	if (bg_bright > 0)
		bg_bright--;
	bg_spt.r = (bg_bright << 3) + 48;
	bg_spt.g = (bg_bright << 3);
	bg_spt.b = (bg_bright << 3) + 64;

	ox = (qcos((frame << 3) & 4095) >> 3) & 63;
	oy = (qsin((frame << 3) & 4095) >> 5) & 63;

	for (y = 0; y <= 4; y++) {
		bg_spt.y = (y << 6) - oy;
		for (x = 0; x <= 6; x++) {
			bg_spt.x = (x << 6) - ox;
			draw_sprite(&bg_spt);
		}
	}
}


void init_game(void) {
	unsigned short p;

	switch (game.players) {
	case 0:
		game.level = MAX_LEVEL - 1;
		game.colors = 4;
		for (p = 0; p < 2; p++) {
			player[p].type = PLTYPE_CPU;
			player[p].score = 0;
			player[p].wins = 0;
			player[p].monster.type = rand() % 4;
		}
		game.round = 0;
		init_round();
		break;
	case 1:
		switch (option.level) {
		case 0:
			game.level = 0;
			game.colors = 4;
			break;
		case 1:
			game.level = 8;
			game.colors = 4;
			break;
		case 2:
			game.level = 8;
			game.colors = 5;
			break;
		}
		p = master_pad;
		player[p].type = PLTYPE_HUMAN;
		player[p].score = 0;
		player[p].monster.select_flag = 0;
		p = 1 - p;
		player[p].type = PLTYPE_CPU;
		player[p].score = 0;
		player[p].monster.type = 0;
		player[p].monster.select_flag = 1;
		init_battle();
		break;
	case 2:
		game.colors = 4;
		for (p = 0; p < 2; p++) {
			player[p].type = PLTYPE_HUMAN;
			player[p].score = 0;
			player[p].monster.select_flag = 0;
		}
		init_battle();
		break;
	}
}


void init_battle(void) {
	unsigned short p;
	unsigned short type;

	music.next = 1;
	for (p = 0; p < 2; p++) {
		type = (player[p].type == PLTYPE_HUMAN) ? p : 2;
		init_anitext(p << 1, 48, player_name[type], 2, ATMODE_DOWN, ATMODE_HOLD, 0);
		player[p].mode_func = plmode_select1;
		player[p].timer = 16;
		player[p].wins = 0;
	}
	init_anitext(1, 112, "VS", 1, ATMODE_DOWN, ATMODE_HOLD, 0);
	game.mode_func = mode_select_monster;
	game.timer1 = 750;
	game.round = 0;
	start_sfx(SFX_DUMP);
}


void init_round(void) {
	PLAYER* pp;
	unsigned short s, p, x, y;
	char txt[12];

	kill_anitext();
	sprintf(txt, "ROUND %d", ++game.round);
	init_anitext(1, 96, txt, 2, ATMODE_UP, ATMODE_SPREAD, 50);

	for (s = 0; s < SEQUENCE_LENGTH; s++)
		sequence[s] = random_bubble();
	for (s = 98; s < SEQUENCE_LENGTH; s += 100)
		sequence[s] = sequence[s + 1] = MAGIC_ID;

	for (p = 0; p < 2; p++) {
		pp = &player[p];
		for (y = 0; y < MAP_H; y++) {
			for (x = 0; x < MAP_W; x++) {
				pp->map[y][x].id = 0;
				pp->map[y][x].oy = 0;
				pp->map[y][x].ecolor = 0;
			}
		}
		pp->mode_func = plmode_ready;
		pp->pause = 0;
		pp->monster.anim = PLANIM_NORMAL;
		pp->monster.oy = 0;
		pp->monster.vy = 0;
		pp->empty = MAP_W * MAP_H;
		pp->chains = 0;
		pp->attack = 0;
		pp->dump = 0;
		pp->disp_dump = 0;
		pp->seq.pos = 0;
		pp->seq.oy = 0;
		pp->aim.time = 0;
		pp->aim.x = p + 3;
	}

	game.mode_func = mode_play;
	game.timer1 = 0;
}


unsigned short random_bubble(void) {
	return(rand() % game.colors + (rand() % 4 ? BUBBLE_ID : FLAME_ID));
}


void kill_fx(void) {
	unsigned short i;

	for (i = 0; i < MAX_MAGICFX; i++)
		magicfx[i].bright = 0;
	for (i = 0; i < MAX_BUBBLEFX; i++)
		bubblefx[i].bright = 0;
}


void create_magicfx(short x, short y, unsigned short speed) {
	magicfx[next_magicfx].bright = 128;
	magicfx[next_magicfx].speed = speed;
	magicfx[next_magicfx].x = x;
	magicfx[next_magicfx++].y = y;
	next_magicfx %= MAX_MAGICFX;
}


void create_bubblefx(short x, short y) {
	bubblefx[next_bubblefx].bright = 128;
	bubblefx[next_bubblefx].x = x << 2;
	bubblefx[next_bubblefx].y = y << 2;
	bubblefx[next_bubblefx].vx = (rand() % 15) - 7;
	bubblefx[next_bubblefx++].vy = -12;
	next_bubblefx %= MAX_BUBBLEFX;
}


void update_fx(void) {
	unsigned short i;

	bubble_spt[2].u = ((frame & 15) << 4);
	for (i = 0; i < MAX_MAGICFX; i++) {
		if (magicfx[i].bright > 0) {
			bubble_spt[2].x = magicfx[i].x;
			bubble_spt[2].y = magicfx[i].y;
			bubble_spt[2].r = bubble_spt[2].g = bubble_spt[2].b = magicfx[i].bright;
			draw_sprite(&bubble_spt[2]);

			magicfx[i].bright -= magicfx[i].speed;
		}
	}

	for (i = 0; i < MAX_BUBBLEFX; i++) {
		if (bubblefx[i].bright > 0) {
			bubblefx_spt.x = bubblefx[i].x / 4;
			bubblefx_spt.y = bubblefx[i].y / 4;
			bubblefx_spt.r = bubblefx_spt.g = bubblefx_spt.b = bubblefx[i].bright;
			draw_sprite(&bubblefx_spt);

			bubblefx[i].bright -= 2;
			bubblefx[i].x += bubblefx[i].vx;
			bubblefx[i].y += bubblefx[i].vy++;
		}
	}
}


void kill_anitext(void) {
	unsigned short id;

	for (id = 0; id < 3; id++)
		ani_text[id].mode = ATMODE_NULL;
}


void init_anitext(unsigned short id, short y, char* txt, short color,
	unsigned short on_mode, unsigned short off_mode, unsigned short time) {
	ANI_TEXT* ap = &ani_text[id];
	unsigned short i, x;
	char c;

	ap->mode = on_mode;
	ap->mode2 = off_mode;
	ap->timer = time;
	ap->color = color;
	ap->dest_y = y;
	ap->len = strlen(txt);
	x = (id * SCREEN_W4) + ((12 - ap->len) * 8);
	for (i = 0; i < ap->len; i++) {
		ap->chr[i].x = x + (i << 4);
		ap->chr[i].y = y;
		ap->chr[i].vx = 0;
		ap->chr[i].vy = 0;
		c = (*txt++) - ' ';
		ap->chr[i].u = (c & 0x0f) << 4;
		ap->chr[i].v = (c >> 4) * 24;
		switch (on_mode) {
		case ATMODE_UP:
			ap->chr[i].y = SCREEN_H + (i << 4);
			break;
		case ATMODE_DOWN:
			ap->chr[i].y = -24 - (i << 4);
			break;
		}
		switch (off_mode) {
		case ATMODE_SPREAD:
			ap->chr[i].vx = (i << 1) - ap->len + 1;
			break;
		}
	}
}


void update_anitext(void) {
	ANI_TEXT* ap;
	unsigned short a, i;

	for (a = 0; a < 3; a++) {
		ap = &ani_text[a];
		if (ap->color >= 0)
			bigfont_spt.palette = ap->color;
		switch (ap->mode) {
		case ATMODE_UP:
			ap->mode = ATMODE_HOLD;
			for (i = 0; i < ap->len; i++) {
				if (ap->chr[i].y > ap->dest_y) {
					ap->chr[i].y -= 12;
					if (ap->chr[i].y < ap->dest_y)
						ap->chr[i].y = ap->dest_y;
					ap->mode = ATMODE_UP;
				}
			}
			break;
		case ATMODE_DOWN:
			ap->mode = ATMODE_HOLD;
			for (i = 0; i < ap->len; i++) {
				if (ap->chr[i].y < ap->dest_y) {
					ap->chr[i].y += 12;
					if (ap->chr[i].y > ap->dest_y)
						ap->chr[i].y = ap->dest_y;
					ap->mode = ATMODE_DOWN;
				}
			}
			break;
		case ATMODE_HOLD:
			if (--ap->timer == 0)
				ap->mode = ap->mode2;
			break;
		case ATMODE_SPREAD:
			ap->mode = ATMODE_NULL;
			for (i = 0; i < ap->len; i++) {
				if (ap->chr[i].y < SCREEN_H) {
					ap->chr[i].x += ap->chr[i].vx;
					ap->chr[i].vy++;
					ap->chr[i].y += ap->chr[i].vy;
					ap->mode = ATMODE_SPREAD;
				}
			}
			break;
		}
		if (ap->mode != ATMODE_NULL) {
			for (i = 0; i < ap->len; i++) {
				bigfont_spt.x = ap->chr[i].x;
				bigfont_spt.y = ap->chr[i].y;
				bigfont_spt.u = ap->chr[i].u;
				bigfont_spt.v = ap->chr[i].v;
				if (ap->color < 0)
					bigfont_spt.palette = (((frame >> 1) + i) % 12);
				draw_sprite(&bigfont_spt);
			}
		}
	}
}


void display_text(short ox, short oy, char* txt, unsigned short color) {
	char c;

	font_spt.x = ox - (strlen(txt) << 2);
	font_spt.y = oy;
	font_spt.palette = color;
	while (*txt != '\0' && font_spt.x < SCREEN_W) {
		c = (*txt++) - ' ';
		if (c >= 1) {
			font_spt.u = (c & 0x1f) << 3;
			font_spt.v = (c & 0x20) >> 1;
			draw_sprite(&font_spt);
		}
		font_spt.x += 8;
	}
}


void display_text2(short ox, short oy, char* txt, short color) {
	char c;
	unsigned short cy;

	bigfont_spt.x = ox - (strlen(txt) << 3);
	bigfont_spt.y = oy;
	if (color >= 0)
		bigfont_spt.palette = color;
	else
		cy = (frame >> 1) + (oy / 24);
	while (*txt != '\0' && bigfont_spt.x < SCREEN_W) {
		if (color < 0) {
			cy %= 12;
			bigfont_spt.palette = cy++;
		}
		c = (*txt++) - ' ';
		if (c >= 1) {
			bigfont_spt.u = (c & 0x0f) << 4;
			bigfont_spt.v = (c >> 4) * 24;
			draw_sprite(&bigfont_spt);
		}
		bigfont_spt.x += 16;
	}
}


void read_pads(void) {
	unsigned short p;

	for (p = 0; p < 2; p++) {
		pad[p].connected = ~(*pad[p].buffer);
		if (pad[p].connected) {
			pad[p].type = *(pad[p].buffer + 1) >> 4;
			switch (pad[p].type) {
				//	case 0x1:	// Mouse
				//		break;
				//	case 0x2:	// neGcon
				//		break;
			case 0x4:	// Standard Pad
				pad[p].keys = ~(*(pad[p].buffer + 2) << 8 | *(pad[p].buffer + 3));
				break;
				//	case 0x5:	// Analog Joystick
				//		break;
				//	case 0x6:	// G-con45
				//		break;
			case 0x7:	// Analog Pad
				// (Std Pad button compatibility only - no analog)
				pad[p].keys = ~(*(pad[p].buffer + 2) << 8 | *(pad[p].buffer + 3));
				break;
			default:	// Unknown contoller
				pad[p].connected = 0;
				pad[p].keys = 0;
			}
		}
		else {
			pad[p].type = 0;
			pad[p].keys = 0;
		}
	}
}


void process_pads(void) {
	unsigned short p, m;

	for (p = 0; p < 2; p++) {
		pad[p].pulse = 0;
		if (pad[p].keys != pad[p].previous) {
			for (m = 0x8000; m; m >>= 1) {
				if ((pad[p].keys & m) && (pad[p].previous & m) == 0)
					pad[p].pulse |= m;
			}
			pad[p].previous = pad[p].keys;
			pad[p].timer = 20;
			pad[p].repeat = pad[p].keys;
		}
		else {
			if (pad[p].keys) {
				if (--pad[p].timer == 0) {
					pad[p].timer = 5;
					pad[p].repeat = pad[p].keys;
				}
				else {
					pad[p].repeat = 0;
				}
			}
		}
	}
}


void start_sfx(unsigned short type) {
	short c;
	unsigned short noteon_flag = 0;
	short prog, note;

	for (c = 0; c < 24; c++) {
		if (sfx_chan[c].timer && sfx_chan[c].type == type) {
			switch (sfx_tab[type].mode) {
			case SFXMODE_MONO:
				SsUtKeyOff(c, vab[0].id, sfx_chan[c].prog, 0, sfx_chan[c].note);
			case SFXMODE_POLY:
				noteon_flag = 1;
				break;
			case SFXMODE_SUSTAIN:
				sfx_chan[c].timer = sfx_tab[type].time;
				break;
			}
			break;	// out of loop - ugh!
		}
	}
	if (noteon_flag || c == 24) {
		prog = sfx_tab[type].prog;
		note = sfx_tab[type].note;
		c = SsUtKeyOn(vab[0].id, prog, 0, note, 0, sfx_tab[type].voll, sfx_tab[type].voll);
		if (c >= 0) {
			sfx_chan[c].type = type;
			sfx_chan[c].prog = prog;
			sfx_chan[c].note = note;
			sfx_chan[c].timer = sfx_tab[type].time;
		}
	}
}


void update_sfx(void) {
	unsigned short c;

	for (c = 0; c < 24; c++) {
		if (sfx_chan[c].timer) {
			if (--sfx_chan[c].timer == 0) {
				SsUtKeyOff(c, vab[0].id, sfx_chan[c].prog, 0, sfx_chan[c].note);
			}
		}
	}
}


void update_music(void) {
	unsigned short c, p, v;

	if ((frame % 100) == 0) {
		for (c = 0; c < 3; c++) {
			p = music_tab[music.step][c].prog;
			if (p >= 0) {
				SsUtKeyOff(music.chan[c], vab[1].id, p, 0, 60);
			}
		}

		if (music.next < MAX_MUSIC)
			music.step = music.next;
		else
			music.step = MAX_MUSIC - 1;

		for (c = 0; c < 3; c++) {
			p = music_tab[music.step][c].prog;
			if (p >= 0) {
				v = music_tab[music.step][c].vol;
				music.chan[c] = SsUtKeyOn(vab[1].id, p, 0, 60, 0, v, v);
			}
		}
	}
}

static void traceLogCallback(int logLevel, const char* text, va_list args) {
	static std::array<char, 4096> buffer;
	vsnprintf(buffer.data(), buffer.size(), text, args);

	const std::array<spdlog::level::level_enum, 8> levels{ spdlog::level::trace, spdlog::level::trace, spdlog::level::debug, spdlog::level::info, spdlog::level::warn, spdlog::level::err, spdlog::level::critical, spdlog::level::off };
	const spdlog::level::level_enum level = levels.at(logLevel);
	raylibLog->log(level, buffer.data());
}

void initialize(void) {
	consoleSink.reset(new spdlog::sinks::stdout_color_sink_mt);
	raylibLog.reset(new spdlog::logger("raylib", consoleSink));
	timLog.reset(new spdlog::logger("tim", consoleSink));
	gameLog.reset(new spdlog::logger("game", consoleSink));

	raylibLog->set_level(FLAGS_debug ? spdlog::level::debug : spdlog::level::warn);
	timLog->set_level(FLAGS_debug ? spdlog::level::debug : spdlog::level::warn);
	gameLog->set_level(FLAGS_debug ? spdlog::level::debug : spdlog::level::warn);

	SetTraceLogCallback(&traceLogCallback);
	Tim::setLogger(timLog);
	if (!FLAGS_export_tims.empty()) {
		Tim::enableTimExport(FLAGS_export_tims);
	}
	if (!FLAGS_import_tims.empty()) {
		Tim::enableTimImport(FLAGS_import_tims);
	}

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(SCREEN_W, SCREEN_H, "Superbub");
	SetTargetFPS(60);

	get_pad_buffers(&pad[0].buffer, &pad[1].buffer);

	//	printf("Redirecting CD I/O...\n");
	//	MWRedirectIO(__MWIO_CDROM);

	init_tims();
	init_sound();
}

void init_tims(void) {
	unsigned short i;

	font_tim = Tim::loadWithMorphedCluts("FONT01.TIM", {
	ClutMorph{128, 0, 0, 128, 0, 0, 128, 0, 0},
	ClutMorph{128, 0, 0, 64, 0, 0, 96, 0, 0},
	ClutMorph{ 64, 0, 0, 128, 0, 0, 96, 0, 0},
	ClutMorph{ 128, 0, 0, 96, 0, 0, 64, 0, 0}, }, false);
	init_sprite(&font_tim, &font_spt);
	font_spt.attribute |= 0x40000000;
	font_spt.w = 8;
	font_spt.h = 16;

	bigfont_tim = Tim::loadWithMorphedCluts("FONT02.TIM", {
	ClutMorph{ 128, 0, 0, 64, 0, 0, 0, 0, 0 },
		ClutMorph{ 128, 0, 0, 128, 0, 0, 0, 0, 0 },
		ClutMorph{ 64, 0, 0, 128, 0, 0, 0, 0, 0 },
		ClutMorph{ 0, 0, 0, 128, 0, 0, 0, 0, 0 },
		ClutMorph{ 0, 0, 0, 128, 0, 0, 64, 0, 0 },
		ClutMorph{ 0, 0, 0, 128, 0, 0, 128, 0, 0 },
		ClutMorph{ 0, 0, 0, 64, 0, 0, 128, 0, 0 },
		ClutMorph{ 0, 0, 0, 0, 0, 0, 128, 0, 0 },
		ClutMorph{ 64, 0, 0, 0, 0, 0, 128, 0, 0 },
		ClutMorph{ 128, 0, 0, 0, 0, 0, 128, 0, 0 },
		ClutMorph{ 128, 0, 0, 0, 0, 0, 64, 0, 0 },
		ClutMorph{ 128, 0, 0, 128, 0, 0, 128, 0, 0 },
		}, false);
	init_sprite(&bigfont_tim, &bigfont_spt);
	bigfont_spt.attribute |= 0x40000000;
	bigfont_spt.w = 16;
	bigfont_spt.h = 24;

	title_tim = Tim::loadWithMorphedCluts("TITLE02.TIM", {
	ClutMorph{ 128, 0, 0, 112, 0, 0, 96, 0, 0},
	ClutMorph{ 96, 0, 0, 112, 0, 0, 128, 0, 0},
	ClutMorph{ 112, 0, 0, 96, 0, 0, 128, 0, 0}, }, false);
	init_sprite(&title_tim, &title_spt);
	title_spt.attribute |= 0x40000000;
	title_spt.w = 42;
	title_spt.h = 1;

	screen_tim[0] = Tim::load("SCREEN2A.TIM", false);
	for (i = 0; i < 3; i++)
		init_sprite(&screen_tim[0], &screen_spt[i]);
	screen_spt[0].x = 0;
	screen_spt[0].w = 32;
	screen_spt[0].u = 0;
	screen_spt[1].x = 160;
	screen_spt[1].w = 64;
	screen_spt[1].u = 32;
	screen_spt[2].x = 352;
	screen_spt[2].w = 32;
	screen_spt[2].u = 96;
	screen_tim[1] = Tim::load("SCREEN2B.TIM", false);
	for (i = 3; i < 7; i++)
		init_sprite(&screen_tim[1], &screen_spt[i]);
	screen_spt[5].attribute |= 0x60000000;
	screen_spt[6].attribute |= 0x60000000;
	screen_spt[3].x = screen_spt[5].x = 32;
	screen_spt[4].x = screen_spt[6].x = 224;
	screen_spt[3].y = screen_spt[4].y = 0;
	screen_spt[5].y = screen_spt[6].y = 192;
	screen_spt[3].h = screen_spt[4].h = 16;
	screen_spt[5].h = screen_spt[6].h = 64;
	screen_spt[3].v = 0;
	screen_spt[4].v = 16;
	screen_spt[5].v = 32;
	screen_spt[6].v = 96;

	bg_tim = Tim::load("BG04.TIM", false);
	init_sprite(&bg_tim, &bg_spt);

	const std::string choice_pic = FLAGS_psm_characters ? "CHOICE01.TIM" : "IMONST02.TIM";
	monsticon_tim = Tim::loadWithMorphedCluts(choice_pic, {
	ClutMorph{ 64, 64, 0, 0, 64, 64, 64, 0, 64},
	ClutMorph{ 0, 64, 64, 64, 0, 64, 64, 64, 0},
	ClutMorph{ 64, 0, 64, 64, 64, 0, 0, 64, 64}, }, false);
	init_sprite(&monsticon_tim, &monsticon_spt);
	monsticon_spt.w = 128;
	monsticon_spt.h = 128;

	std::string monster_pic0 = "MONST01.TIM";
	std::string monster_pic1 = "MONST02.TIM";
	std::string monster_pic2 = "MONST03.TIM";
	std::string monster_pic3 = "MONST04.TIM";
	if (FLAGS_psm_characters) {
		monster_pic0 = "PIG01.TIM";
		monster_pic1 = "LION01.TIM";
		monster_pic2 = "FROG01.TIM";
		monster_pic3 = "SPOOK01.TIM";
	}

	monster_tim[0] = Tim::loadWithMorphedCluts(monster_pic0, {
	ClutMorph{ 64, 64, 0, 0, 64, 64, 64, 0, 64},
	ClutMorph{ 0, 64, 64, 64, 0, 64, 64, 64, 0},
	ClutMorph{ 64, 0, 64, 64, 64, 0, 0, 64, 64}, }, false);

	monster_tim[1] = Tim::loadWithMorphedCluts(monster_pic1, {
	ClutMorph{ 64, 64, 0, 0, 64, 64, 64, 0, 64},
	ClutMorph{ 0, 64, 64, 64, 0, 64, 64, 64, 0},
	ClutMorph{ 64, 0, 64, 64, 64, 0, 0, 64, 64}, }, false);

	monster_tim[2] = Tim::loadWithMorphedCluts(monster_pic2, {
	ClutMorph{ 64, 64, 0, 0, 64, 64, 64, 0, 64},
	ClutMorph{ 0, 64, 64, 64, 0, 64, 64, 64, 0},
	ClutMorph{ 64, 0, 64, 64, 64, 0, 0, 64, 64}, }, false);

	monster_tim[3] = Tim::loadWithMorphedCluts(monster_pic3, {
	ClutMorph{ 64, 64, 0, 0, 64, 64, 64, 0, 64},
	ClutMorph{ 0, 64, 64, 64, 0, 64, 64, 64, 0},
	ClutMorph{ 64, 0, 64, 64, 64, 0, 0, 64, 64}, }, false);

	for (i = 0; i < 4; i++) {
		init_sprite(&monster_tim[i], &monster_spt[i]);
		monster_spt[i].w = 32;
	}

	bubble_tim[0] = Tim::loadWithMorphedCluts("BUBBLE08.TIM", {
	ClutMorph{ 120, 8, 0, 0, 128, 0, 0, 128, 0 }, // red
		ClutMorph{ 112, 16, 0, 96, 32, 0, 0, 128, 0 }, // yellow
		ClutMorph{ 0, 128, 0, 64, 64, 0, 128, 0, 0 }, // blue
		ClutMorph{ 0, 128, 0, 112, 16, 0, 0, 128, 0 }, // green
		ClutMorph{ 112, 16, 0, 0, 128, 0, 128, 0, 0 },
		}, false); // magenta

	bubble_tim[1] = Tim::loadWithMorphedCluts("FLAME04.TIM", {
	ClutMorph{ 120, 8, 0, 0, 128, 0, 0, 128, 0 }, // red
		ClutMorph{ 112, 16, 0, 96, 32, 0, 0, 128, 0 }, // yellow
		ClutMorph{ 0, 128, 0, 64, 64, 0, 128, 0, 0 }, // blue
		ClutMorph{ 0, 128, 0, 112, 16, 0, 0, 128, 0 }, // green
		ClutMorph{ 112, 16, 0, 0, 128, 0, 128, 0, 0 },
		}, false); // magenta

	bubble_tim[2] = Tim::load("MAGIC01.TIM", true);
	for (i = 0; i < 2; i++) {
		init_sprite(&bubble_tim[i], &bubble_spt[i]);
		bubble_spt[i].attribute |= 0x40000000;
		bubble_spt[i].w = 16;
	}
	init_sprite(&bubble_tim[2], &bubble_spt[2]);
	bubble_spt[2].attribute |= 0x50000000;
	bubble_spt[2].w = 16;

	eater_tim = Tim::loadWithMorphedCluts("BFLAME02.TIM", {
	ClutMorph{ 120, 8, 0, 0, 128, 0, 0, 128, 0 }, // red
	ClutMorph{ 112, 16, 0, 96, 32, 0, 0, 128, 0}, // yellow
	ClutMorph{ 0, 128, 0, 64, 64, 0, 128, 0, 0}, // blue
	ClutMorph{ 0, 128, 0, 112, 16, 0, 0, 128, 0}, // green
	ClutMorph{ 112, 16, 0, 0, 128, 0, 128, 0, 0}, }, true); // magenta
	init_sprite(&eater_tim, &eater_spt);
	eater_spt.attribute |= 0x50000000;
	eater_spt.w = 32;

	timebar_tim = Tim::load("TBAR03.TIM", true);
	init_sprite(&timebar_tim, &timebar_spt);
	timebar_spt.y = 235;
	timebar_spt.h = 4;

	wincup_tim = Tim::load("CUP01.TIM", false);
	init_sprite(&wincup_tim, &wincup_spt);
	wincup_spt.attribute |= 0x40000000;

	bubblefx_tim = Tim::load("SMLBUB02.TIM", true);
	init_sprite(&bubblefx_tim, &bubblefx_spt);
	bubblefx_spt.attribute |= 0x50000000;
}

void init_sound(void) {
	unsigned short i;

	for (i = 0; i < 16; i++)
		SsVabClose(i);

	vab[0].id = load_vab(vab[0].vh_addr, "FX01.VH", "FX01.VB");
	vab[1].id = load_vab(vab[1].vh_addr, "MUSIC01.VH", "MUSIC01.VB");

	SsSetMVol(127, 127);
}


void init_sprite(const Tim* tim, GsSPRITE* spt) {
	spt->tim = tim;
	spt->attribute = 0;
	spt->x = spt->y = 0;
	spt->w = tim->logicalWidth;
	spt->h = tim->logicalHeight;
	spt->u = 0;
	spt->v = 0;
	spt->palette = 0;
	spt->r = spt->g = spt->b = 128;
}

short load_vab(void* vh_addr, char* vh_file, char* vb_file) {
	short id = -1;

	gameLog->info("LOAD: {}", vh_file);
	gameLog->info("ADDR: {}", vh_addr);
	if (MWbload(vh_file, (void*)vh_addr) > 0) {
		gameLog->info("LOAD: {}", vb_file);
		gameLog->info("ADDR: {}", load_addr);
		if (MWbload(vb_file, (void*)load_addr) > 0) {
			id = SsVabTransfer((void*)vh_addr, (void*)load_addr, -1, 1);
			//			printf("VAB ID=%d\n",id);
		}
		else {
			gameLog->error("Failed to load: {}", vb_file);
		}
	}
	else {
		gameLog->error("Failed to load: {}", vh_file);
	}
	return(id);
}

std::vector<GsSPRITE> sprites;

void draw_sprite(const GsSPRITE* sprite) {
	sprites.emplace_back(*sprite);
}

void flush_sprites() {
	constexpr uint32_t brigthness_flag = 1u << 6;

	for (auto sprite = sprites.rbegin(); sprite != sprites.rend(); ++sprite) {
		Rectangle source;
		Rectangle dest;
		Color tint = WHITE;

		source.x = sprite->tim->palettedBounds.at(sprite->palette).x + sprite->u;
		source.y = sprite->tim->palettedBounds.at(sprite->palette).y + sprite->v;
		source.width = sprite->w;
		source.height = sprite->h;

		dest.x = sprite->x;
		dest.y = sprite->y;
		dest.width = sprite->w;
		dest.height = sprite->h;

		if ((sprite->attribute & brigthness_flag) == 0) {
			const float r_fact = float(sprite->r) / 128.0f;
			const float g_fact = float(sprite->g) / 128.0f;
			const float b_fact = float(sprite->b) / 128.0f;

			tint.r = unsigned char(std::clamp(int(r_fact * 255), 0, 255));
			tint.g = unsigned char(std::clamp(int(g_fact * 255), 0, 255));
			tint.b = unsigned char(std::clamp(int(b_fact * 255), 0, 255));
		}

		DrawTexturePro(sprite->tim->texture, source, dest, Vector2{ 0,0 }, 0, tint);
	}

	sprites.clear();
}

uint32_t pad_buffer_1, pad_buffer_2;

void get_pad_buffers(unsigned char** pad1, unsigned char** pad2) {
	*pad1 = reinterpret_cast<unsigned char*>(&pad_buffer_1);
	*pad2 = reinterpret_cast<unsigned char*>(&pad_buffer_2);
}

void fetch_input() {
	const std::map<int, uint16_t> player_1_keys{
		{KEY_R, PAD_L2 },
		{KEY_T, PAD_R2 },
		{KEY_F, PAD_L1 },
		{KEY_G, PAD_R1 },
		{KEY_W, PAD_TRIANGLE },
		{KEY_D, PAD_CIRCLE },
		{KEY_S, PAD_X },
		{KEY_A, PAD_SQUARE },
		{KEY_SPACE, PAD_SELECT },
		{KEY_ENTER, PAD_START },
		{KEY_UP, PAD_UP },
		{KEY_RIGHT, PAD_RIGHT },
		{KEY_DOWN, PAD_DOWN },
		{KEY_LEFT, PAD_LEFT },
	};

	constexpr uint32_t with_controller = 0x00;
	constexpr uint32_t without_controller = 0xff;
	constexpr uint32_t terminal_standard_pad = 0x4 << 12;
	constexpr uint32_t data_size_2bytes = 0x2 << 8;

	pad_buffer_1 = with_controller | terminal_standard_pad | data_size_2bytes;

	{
		uint16_t payload = 0;
		for (const auto& entry : player_1_keys) {
			if (IsKeyDown(entry.first)) {
				payload |= entry.second;
			}
		}

		payload = ((payload & 0x00ff) << 8) | ((payload & 0xff00) >> 8);
		pad_buffer_1 |= uint32_t(~payload) << 16;
	}

	pad_buffer_2 = without_controller;
}
