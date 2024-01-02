#include "engine.h"
#include <memory>
#include <stdarg.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/logger.h>
#include <raylib.h>

DISPENV GsDISPENV;
long MODE_NTSC = 0;
long MODE_PAL = 1;
uintptr_t _end = 0; // unused
uintptr_t _stack_addr = 0; // unused
size_t _stack_size = 0; // unused

static std::shared_ptr<spdlog::sinks::sink> consoleSink;
static std::shared_ptr<spdlog::logger> raylibLog;
static std::shared_ptr<spdlog::logger> engineLog;
static std::shared_ptr<spdlog::logger> gameLog;

static void traceLogCallback(int logLevel, const char* text, va_list args) {
	static std::array<char, 4096> buffer;
	vsnprintf(buffer.data(), buffer.size(), text, args);

	switch (logLevel) {
	case LOG_ALL:
	case LOG_TRACE:
		raylibLog->trace(buffer.data());
		break;

	case LOG_DEBUG:
		raylibLog->debug(buffer.data());
		break;

	case LOG_INFO:
		raylibLog->info(buffer.data());
		break;

	case LOG_WARNING:
		raylibLog->warn(buffer.data());
		break;

	case LOG_ERROR:
		raylibLog->error(buffer.data());
		break;

	case LOG_FATAL:
		raylibLog->critical(buffer.data());
		break;

	case LOG_NONE:
		break;
	}
}

static void initLogs() {
	consoleSink.reset(new spdlog::sinks::stdout_color_sink_mt);
	raylibLog.reset(new spdlog::logger("raylib", consoleSink));
	engineLog.reset(new spdlog::logger("engine", consoleSink));
	gameLog.reset(new spdlog::logger("game", consoleSink));

	SetTraceLogCallback(&traceLogCallback);
}

///////////////////////////
// Debug print functions //
///////////////////////////

void FntPrint(const char* format, ...) {
	va_list args;
	va_start(args, format);
	char buffer[4096];
	vsprintf_s(buffer, format, args);
	va_end(args);

	gameLog->log(spdlog::level::info, std::string(buffer));
}

unsigned long* FntFlush(int id) {
	gameLog->flush();
	return nullptr;
}

int FntOpen(int x, int y, int w, int h, int isbg, int n) {
	return 0;
}

void FntLoad(int tx, int ty) {
}

///////////////////////
// Display functions //
///////////////////////

void InitHeap(void* head, size_t size) {
	// We don't need to init the heap, but we can use this function call to do global initialization.
	initLogs();
}

long SetVideoMode(long mode) {
	return MODE_PAL;
}

////////////////////////
// Graphics functions //
////////////////////////

void GsInitGraph(int x_res, int y_res, int intl, int dither, int vram) {
	engineLog->debug("GsInitGraph x_res = {}, y_res = {}", x_res, y_res);
	InitWindow(x_res, y_res, "Game");
}

void GsDefDispBuff(int x0, int y0, int x1, int y1) {
}
