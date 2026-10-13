//
//	app.cpp
//
//	SDL main callbacks and input
//

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <spdlog/logger.h>
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <CLI/CLI.hpp>
#ifdef SDL_PLATFORM_EMSCRIPTEN
#include <emscripten.h>
#endif

// Defined in main.cpp: keep the types and constants in sync //

struct Options {
	bool debug = false;
	bool originalMonsters = false;
	std::string hd;
};

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

struct Pad {
	unsigned char* buffer;
	unsigned char connected;
	unsigned char type;
	unsigned short keys;
	unsigned short previous;
	unsigned short pulse;
	unsigned short repeat;
	unsigned short timer;
};

struct Game {
	void (*mode_func)(void);
	unsigned short timer1, timer2;
	unsigned short players, level, colors;
	unsigned short round;
	unsigned short winner, loser;
	unsigned short rank;
};

extern std::shared_ptr<spdlog::logger> gameLog;
extern Options options;
extern SDL_Renderer* renderer;
extern unsigned long frame;
extern Pad pad[2];
extern Game game;
extern uint32_t pad_buffer_1, pad_buffer_2;

void init_intro(void);
void update_anitext(void);
void read_pads(void);
void process_pads(void);
void update_sfx(void);
void update_music(void);
void initialize(void);
void shutdown(void);
void clear_sprites();
void flush_sprites();

void fetch_input();

// The game was made for PAL consoles: everything, music included, is timed on 50 frames per second
#define LOGIC_FPS 50
// How many logic frames can be run in a row to catch up, after the game was stalled
#define MAX_PENDING_FRAMES 4

// The logic runs at its own fixed rate, whatever the refresh rate of the display is
static const Uint64 frame_time = SDL_NS_PER_SECOND / LOGIC_FPS;
static Uint64 previous_time;
static Uint64 pending_time;

// Gamepad Stuff //

// How far a stick or a trigger must be pushed to count as pressed
#define STICK_THRESHOLD 16384
#define TRIGGER_THRESHOLD 16384

// The first gamepad plugged in goes to player 1, the second one to player 2
static SDL_Gamepad* gamepads[2] = { nullptr, nullptr };

static void add_gamepad(SDL_JoystickID id) {
	for (SDL_Gamepad* gamepad : gamepads) {
		if (gamepad && SDL_GetGamepadID(gamepad) == id) {
			return;
		}
	}

	for (int p = 0; p < 2; p++) {
		if (gamepads[p] == nullptr) {
			gamepads[p] = SDL_OpenGamepad(id);
			if (gamepads[p]) {
				gameLog->info("Player {} uses gamepad: {}", p + 1, SDL_GetGamepadName(gamepads[p]));
			}
			else {
				gameLog->warn("Failed to open gamepad: {}", SDL_GetError());
			}
			return;
		}
	}
}

static void remove_gamepad(SDL_JoystickID id) {
	for (int p = 0; p < 2; p++) {
		if (gamepads[p] && SDL_GetGamepadID(gamepads[p]) == id) {
			gameLog->info("Player {} lost the gamepad", p + 1);
			SDL_CloseGamepad(gamepads[p]);
			gamepads[p] = nullptr;
		}
	}
}

#ifdef SDL_PLATFORM_EMSCRIPTEN

// On-screen pad Stuff //

// The web page draws a pad over the game, for touch screens: here it's turned into a virtual joystick,
// so that the game handles it as any other gamepad

// The keys held down on the pad, in the bits set by the page, or -1 until the pad is touched for the first time
EM_JS(int, hud_key_mask, (), {
	return (typeof Module['hudKeyMask'] === 'number') ? Module['hudKeyMask'] : -1;
});

static SDL_JoystickID hud_joystick = 0;
static bool hud_joystick_failed = false;

// Called by SDL whenever it reads the state of the joysticks
static void SDLCALL update_hud_joystick(void* userdata) {
	// In the order of the bits set by the page
	const SDL_GamepadButton buttons[] = {
		SDL_GAMEPAD_BUTTON_DPAD_UP,
		SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
		SDL_GAMEPAD_BUTTON_DPAD_DOWN,
		SDL_GAMEPAD_BUTTON_DPAD_LEFT,
		SDL_GAMEPAD_BUTTON_SOUTH,
		SDL_GAMEPAD_BUTTON_EAST,
	};

	SDL_Joystick* joystick = SDL_GetJoystickFromID(hud_joystick);
	const int keys = hud_key_mask();
	if (joystick == nullptr || keys < 0) {
		return;
	}

	for (int b = 0; b < SDL_arraysize(buttons); b++) {
		SDL_SetJoystickVirtualButton(joystick, buttons[b], (keys & (1 << b)) != 0);
	}
}

// The pad becomes a gamepad only once it's used, so that it doesn't take the place of a real one for nothing
static void attach_hud_joystick() {
	if (hud_joystick != 0 || hud_joystick_failed || hud_key_mask() < 0) {
		return;
	}

	SDL_VirtualJoystickDesc desc;
	SDL_INIT_INTERFACE(&desc);
	desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
	// All the buttons up to the directional pad, so that their indices are the values of SDL_GamepadButton
	desc.nbuttons = SDL_GAMEPAD_BUTTON_DPAD_RIGHT + 1;
	desc.name = "On-screen pad";
	desc.Update = update_hud_joystick;

	hud_joystick = SDL_AttachVirtualJoystick(&desc);
	if (hud_joystick == 0) {
		gameLog->warn("Failed to attach the on-screen pad: {}", SDL_GetError());
		hud_joystick_failed = true;
	}
}

#endif

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	CLI::App app{ "Super Bub Contest" };
	app.add_flag("--debug", options.debug, "Enable debug");
	app.add_flag("--original-monsters", options.originalMonsters, "Use the original monster images instead of the PSM characters");
	app.add_option("--hd", options.hd, "Folder to export / import HD assets");
	try {
		app.parse(argc, argv);
	}
	catch (const CLI::ParseError& e) {
		return app.exit(e) == 0 ? SDL_APP_SUCCESS : SDL_APP_FAILURE;
	}

	initialize();

	// The gamepads already plugged in are reported with SDL_EVENT_GAMEPAD_ADDED, like the ones that come later
	if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
		gameLog->warn("Failed to initialize gamepads: {}", SDL_GetError());
	}

	frame = 0;

	init_intro();

	//	printf("Main Loop.\n");

	previous_time = SDL_GetTicksNS();
	pending_time = frame_time;

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	// The window is closed with its close button or the Escape key
	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}
	else if (event->type == SDL_EVENT_KEY_DOWN && event->key.scancode == SDL_SCANCODE_ESCAPE) {
		return SDL_APP_SUCCESS;
	}
	else if (event->type == SDL_EVENT_GAMEPAD_ADDED) {
		add_gamepad(event->gdevice.which);
	}
	else if (event->type == SDL_EVENT_GAMEPAD_REMOVED) {
		remove_gamepad(event->gdevice.which);
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
#ifdef SDL_PLATFORM_EMSCRIPTEN
	attach_hud_joystick();
#endif

	const Uint64 current_time = SDL_GetTicksNS();
	pending_time = std::min(pending_time + (current_time - previous_time), frame_time * MAX_PENDING_FRAMES);
	previous_time = current_time;

	while (pending_time >= frame_time) {
		pending_time -= frame_time;

		// Every logic frame draws the whole screen again
		clear_sprites();

		fetch_input();

		read_pads();
		if (pad[0].keys == (PAD_SELECT | PAD_START)) {
			return SDL_APP_SUCCESS;
		}
		process_pads();
		if (pad[0].pulse == (PAD_L1 | PAD_R1))
			options.debug = !options.debug;

		update_music();
		update_sfx();

		update_anitext();

		(*game.mode_func)();

		gameLog->trace("FRAME: {}", frame);

		frame++;
	}

	// Shows the last logic frame: again, if no new one was due yet
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);

	flush_sprites();

	SDL_RenderPresent(renderer);

	return SDL_APP_CONTINUE;
}

// Also called when SDL_AppInit() stops early, before initialize() has run
void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	for (SDL_Gamepad*& gamepad : gamepads) {
		if (gamepad) {
			SDL_CloseGamepad(gamepad);
			gamepad = nullptr;
		}
	}

	shutdown();
}

// The buttons are matched by their position, so that the layout is the one of the PSX pad
static uint16_t gamepad_keys(SDL_Gamepad* gamepad) {
	const std::map<SDL_GamepadButton, uint16_t> buttons{
		{SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, PAD_L1 },
		{SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, PAD_R1 },
		{SDL_GAMEPAD_BUTTON_NORTH, PAD_TRIANGLE },
		{SDL_GAMEPAD_BUTTON_EAST, PAD_CIRCLE },
		{SDL_GAMEPAD_BUTTON_SOUTH, PAD_X },
		{SDL_GAMEPAD_BUTTON_WEST, PAD_SQUARE },
		{SDL_GAMEPAD_BUTTON_BACK, PAD_SELECT },
		{SDL_GAMEPAD_BUTTON_START, PAD_START },
		{SDL_GAMEPAD_BUTTON_DPAD_UP, PAD_UP },
		{SDL_GAMEPAD_BUTTON_DPAD_RIGHT, PAD_RIGHT },
		{SDL_GAMEPAD_BUTTON_DPAD_DOWN, PAD_DOWN },
		{SDL_GAMEPAD_BUTTON_DPAD_LEFT, PAD_LEFT },
	};

	uint16_t keys = 0;
	if (gamepad == nullptr) {
		return keys;
	}

	for (const auto& entry : buttons) {
		if (SDL_GetGamepadButton(gamepad, entry.first)) {
			keys |= entry.second;
		}
	}

	if (SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > TRIGGER_THRESHOLD)
		keys |= PAD_L2;
	if (SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > TRIGGER_THRESHOLD)
		keys |= PAD_R2;

	// The left stick works as the directional pad
	const Sint16 stick_x = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
	const Sint16 stick_y = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);
	if (stick_x < -STICK_THRESHOLD)
		keys |= PAD_LEFT;
	if (stick_x > STICK_THRESHOLD)
		keys |= PAD_RIGHT;
	if (stick_y < -STICK_THRESHOLD)
		keys |= PAD_UP;
	if (stick_y > STICK_THRESHOLD)
		keys |= PAD_DOWN;

	return keys;
}

void fetch_input() {
	const std::map<SDL_Scancode, uint16_t> player_1_keys{
		{SDL_SCANCODE_R, PAD_L2 },
		{SDL_SCANCODE_T, PAD_R2 },
		{SDL_SCANCODE_F, PAD_L1 },
		{SDL_SCANCODE_G, PAD_R1 },
		{SDL_SCANCODE_W, PAD_TRIANGLE },
		{SDL_SCANCODE_D, PAD_CIRCLE },
		{SDL_SCANCODE_S, PAD_X },
		{SDL_SCANCODE_A, PAD_SQUARE },
		{SDL_SCANCODE_SPACE, PAD_SELECT },
		{SDL_SCANCODE_RETURN, PAD_START },
		{SDL_SCANCODE_UP, PAD_UP },
		{SDL_SCANCODE_RIGHT, PAD_RIGHT },
		{SDL_SCANCODE_DOWN, PAD_DOWN },
		{SDL_SCANCODE_LEFT, PAD_LEFT },
	};

	constexpr uint32_t with_controller = 0x00;
	constexpr uint32_t without_controller = 0xff;
	constexpr uint32_t terminal_standard_pad = 0x4 << 12;
	constexpr uint32_t data_size_2bytes = 0x2 << 8;

	// Player 1 always has a controller, as the keyboard works along with the gamepad
	pad_buffer_1 = with_controller | terminal_standard_pad | data_size_2bytes;

	{
		const bool* key_states = SDL_GetKeyboardState(nullptr);

		uint16_t payload = gamepad_keys(gamepads[0]);
		for (const auto& entry : player_1_keys) {
			if (key_states[entry.first]) {
				payload |= entry.second;
			}
		}

		payload = ((payload & 0x00ff) << 8) | ((payload & 0xff00) >> 8);
		pad_buffer_1 |= uint32_t(~payload) << 16;
	}

	if (gamepads[1]) {
		pad_buffer_2 = with_controller | terminal_standard_pad | data_size_2bytes;

		uint16_t payload = gamepad_keys(gamepads[1]);
		payload = ((payload & 0x00ff) << 8) | ((payload & 0xff00) >> 8);
		pad_buffer_2 |= uint32_t(~payload) << 16;
	}
	else {
		pad_buffer_2 = without_controller;
	}
}
