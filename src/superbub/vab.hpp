#pragma once
#include <raylib.h>
#include <filesystem>
#include <array>
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

	Wave& getAudio(const int program, const int tone);

private:
	static std::shared_ptr<spdlog::logger> logger;
	static std::filesystem::path hdFolder;

	struct VabSound {
		int program;
		int tone;
		Wave wave;
	};

	std::vector<VabSound> sounds;

	Vab() {}
};

class Voices {
public:
	using Voice = short;
	static constexpr Voice NullVoice = -1;
	static constexpr size_t MaxVoices = 24;

	Voices();
	~Voices();

	Voice voiceOn(Vab& vab, const int program, const int tone, const int note, const short volume);
	void voiceOff(const Voice voice);
	void allVoicesOff();
	void setVolume(const short volume);

private:

	std::array<Sound, MaxVoices> sounds;
};
