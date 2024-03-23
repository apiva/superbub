#pragma once
#include <raylib.h>
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
	static std::shared_ptr<spdlog::logger> logger;

	static void setLogger(std::shared_ptr<spdlog::logger> _logger) {
		logger = _logger;
	}

	static Tim load(const std::filesystem::path& path);
	static Tim loadWithMorphedCluts(const std::filesystem::path& path, const std::initializer_list<ClutMorph>& morphed_cluts);

	Tim();
	~Tim();

	Tim(const Tim&) = delete;
	Tim(Tim&& other);
	Tim& operator=(const Tim&) = delete;
	Tim& operator=(Tim&& other);

	int logicalWidth = 0;
	int logicalHeight = 0;
	Image image;
	Texture texture;
	std::vector<Rectangle> palettedBounds;

	bool valid() const {
		return image.data != nullptr;
	}

	bool exportImage(const std::filesystem::path& path) const;
};
