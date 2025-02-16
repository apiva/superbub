#pragma once
#include <raylib.h>
#include <filesystem>
#include <spdlog/logger.h>

class Vab {
public:
	static void setLogger(std::shared_ptr<spdlog::logger> _logger) {
		logger = _logger;
	}

	static void setExportPath(const std::filesystem::path& export_path) {
		exportPath = export_path;
	}

	static Vab load(const std::filesystem::path& path);

	~Vab();

	//Vab(const Vab&) = delete;
	//Vab(Vab&&) = delete;
	//Vab& operator=(const Vab&) = delete;
	//Vab& operator=(Vab&&) = delete;

	Sound& getSound(const int program, const int tone);

private:
	static std::shared_ptr<spdlog::logger> logger;
	static std::filesystem::path exportPath;

	struct Tone {
		int program;
		int tone;
		Sound sound;
	};

	std::vector<Tone> tones;

	Vab() {}
};
