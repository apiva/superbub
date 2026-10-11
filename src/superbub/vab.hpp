#pragma once
#include <SDL3/SDL.h>
#include <filesystem>
#include <array>
#include <vector>
#include <spdlog/logger.h>

class Vab {
public:
	struct Wave {
		SDL_AudioSpec spec{};
		std::vector<Uint8> data;

		bool valid() const {
			return !data.empty();
		}
	};

	const Wave& getAudio(const int program, const int tone) const;

private:
	friend class VabLoader;

	struct VabSound {
		int program;
		int tone;
		Wave wave;
	};

	std::vector<VabSound> sounds;

	Vab() {}
};

class VabLoader {
public:
	VabLoader(std::shared_ptr<spdlog::logger> logger, const std::filesystem::path& hdFolder);

	Vab load(const std::filesystem::path& path);

private:
	std::shared_ptr<spdlog::logger> logger;
	std::filesystem::path hdFolder;
};

class Voices {
public:
	using Voice = short;
	static constexpr Voice NullVoice = -1;
	static constexpr size_t MaxVoices = 24;

	// Needs the SDL audio subsystem, and has to be destroyed before SDL is shut down
	Voices(std::shared_ptr<spdlog::logger> logger);

	Voice voiceOn(const Vab& vab, const int program, const int tone, const int note, const short volume);
	void voiceOff(const Voice voice);
	void allVoicesOff();
	void setVolume(const short volume);

private:

	std::shared_ptr<spdlog::logger> logger;
	// Declared before the streams, so that it is closed after they are gone
	std::shared_ptr<SDL_AudioDeviceID> device;
	std::array<std::shared_ptr<SDL_AudioStream>, MaxVoices> streams;
};
