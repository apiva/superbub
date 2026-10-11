#pragma once
#include <SDL3/SDL.h>
#include <spdlog/logger.h>
#include <filesystem>

struct ClutMorph {
	short rr;
	short rg;
	short rb;
	short gr;
	short gg;
	short gb;
	short br;
	short bg;
	short bb;
};

class Tim {
public:
	int logicalWidth = 0;
	int logicalHeight = 0;
	// Owned by the TimLoader that loaded this TIM, and valid only as long as that loader exists
	SDL_Texture* texture = nullptr;
	std::vector<SDL_FRect> palettedBounds;

	bool valid() const {
		return texture != nullptr;
	}
};

// Loads TIMs as textures of the given renderer. The textures are destroyed together with the loader,
// so it has to be destroyed before the renderer.
class TimLoader {
public:
	TimLoader(SDL_Renderer* renderer, std::shared_ptr<spdlog::logger> logger, const std::filesystem::path& exportPath, const std::filesystem::path& importPath);
	~TimLoader();

	TimLoader(const TimLoader&) = delete;
	TimLoader& operator=(const TimLoader&) = delete;

	Tim load(const std::filesystem::path& path, const bool special_trasparency_bit);
	Tim loadWithMorphedCluts(const std::filesystem::path& path, const std::initializer_list<ClutMorph>& morphed_cluts, const bool special_trasparency_bit);

private:
	SDL_Renderer* renderer;
	std::shared_ptr<spdlog::logger> logger;
	std::filesystem::path exportPath;
	std::filesystem::path importPath;
	std::vector<SDL_Texture*> textures;
};
