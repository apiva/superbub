#pragma once
#include <raylib.h>
#include <filesystem>
#include <spdlog/logger.h>

class Vab {
public:
	static void setLogger(std::shared_ptr<spdlog::logger> _logger) {
		logger = _logger;
	}

	static void setHdFolder(const std::filesystem::path& hd_folder) {
		hdFolder = hd_folder;
	}

	static Vab load(const std::filesystem::path& path);

	~Vab();

	Vab(const Vab&) = delete;
	Vab(Vab&&) = default;
	Vab& operator=(const Vab&) = delete;
	Vab& operator=(Vab&&) = default;

	Sound& getSound(const int program, const int tone);

private:
	static std::shared_ptr<spdlog::logger> logger;
	static std::filesystem::path hdFolder;

	struct VabSound {
		int program;
		int tone;
		Sound sound;
	};

	std::vector<VabSound> sounds;

	Vab() {}
};
