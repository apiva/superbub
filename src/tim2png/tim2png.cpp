#include <gflags/gflags.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/logger.h>
#include <raylib.h>
#include <tim.hpp>

DEFINE_string(input, "", "Input TIM file");
DEFINE_string(output, "", "Output TIM file");
DEFINE_bool(debug, false, "Enable debug output");

std::shared_ptr<spdlog::sinks::sink> console_sink;
std::shared_ptr<spdlog::logger> raylibLogger;
std::shared_ptr<spdlog::logger> timLogger;
std::shared_ptr<spdlog::logger> toolLogger;

static void traceLogCallback(int logLevel, const char* text, va_list args) {
	static std::array<char, 4096> buffer;
	vsnprintf(buffer.data(), buffer.size(), text, args);

	const std::array<spdlog::level::level_enum, 8> levels{ spdlog::level::trace, spdlog::level::trace, spdlog::level::debug, spdlog::level::info, spdlog::level::warn, spdlog::level::err, spdlog::level::critical, spdlog::level::off };
	const spdlog::level::level_enum level = levels.at(logLevel);
	raylibLogger->log(level, buffer.data());
}

int main(int argc, char* argv[]) {
	gflags::ParseCommandLineFlags(&argc, &argv, false);

	console_sink.reset(new spdlog::sinks::stdout_color_sink_st);
	raylibLogger.reset(new spdlog::logger("raylib", console_sink));
	timLogger.reset(new spdlog::logger("tim", console_sink));
	toolLogger.reset(new spdlog::logger("tool", console_sink));
	if (FLAGS_debug) {
		raylibLogger->set_level(spdlog::level::debug);
		timLogger->set_level(spdlog::level::debug);
		toolLogger->set_level(spdlog::level::debug);
	}
	else {
		raylibLogger->set_level(spdlog::level::warn);
		timLogger->set_level(spdlog::level::warn);
		toolLogger->set_level(spdlog::level::warn);
	}

	SetTraceLogCallback(traceLogCallback);
	Tim::setLogger(timLogger);

	InitWindow(640, 480, "Tim Exporter");

	toolLogger->info("Input filename: {}", FLAGS_input);
	toolLogger->info("Output filename: {}", FLAGS_output);
	if (FLAGS_input.empty() || FLAGS_output.empty()) {
		toolLogger->error("Input or output path is empty, quitting");
		return 1;
	}

	{
		Tim tim = Tim::load(FLAGS_input);
		if (!tim.valid()) {
			toolLogger->error("Failed to load TIM");
			return 1;
		}

		if (!tim.exportImage(FLAGS_output)) {
			toolLogger->error("Failed to export TIM");
			return 1;
		}
	}

	CloseWindow();

	return 0;
}
