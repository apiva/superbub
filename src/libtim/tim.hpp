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
	static std::filesystem::path exportPath;
	static std::filesystem::path importPath;

	static void setLogger(std::shared_ptr<spdlog::logger> _logger) {
		logger = _logger;
	}

	static void enableTimExport(const std::filesystem::path& path) {
		exportPath = path;
	}

	static void enableTimImport(const std::filesystem::path& path) {
		importPath = path;
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
	Texture texture;
	std::vector<Rectangle> palettedBounds;

	bool valid() const {
		return texture.id != 0;
	}
};
