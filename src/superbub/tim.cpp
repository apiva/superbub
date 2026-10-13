#include <tim.hpp>
#include <fstream>

// https://www.psxdev.net/forum/viewtopic.php?t=109

struct TimHeader {
	uint8_t tag;
	uint8_t version;
	uint16_t padding0;
	uint8_t flags;
	uint8_t padding1;
	uint16_t padding2;
};

struct DataHeader {
	uint32_t clutLength;
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
};

typedef std::vector<uint16_t> Clut;

struct TimFile {
	TimHeader timHeader;
	DataHeader clutHeader;
	std::vector<Clut> clutTable;
	DataHeader imageHeader;
	std::vector<uint16_t> imageData;
};

enum ColorMode {
	Indices4bits = 0b00,
	Indices8bits = 0b01,
	Colors16bits = 0b10,
	Colors24bits = 0b11,
};

static ColorMode getColorMode(const TimHeader& header) {
	return ColorMode(header.flags & 0b11);
}

static bool hasClut(const TimHeader& header) {
	return bool((header.flags >> 3) & 0b1);
}

static SDL_Color getColorFromClut(const Clut& clut, const int index, const bool special_trasparency_bit) {
	const uint16_t value = clut.at(index);
	const unsigned int r = (value >> 0) & 0b11111;
	const unsigned int g = (value >> 5) & 0b11111;
	const unsigned int b = (value >> 10) & 0b11111;
	const unsigned int t = (value >> 15) & 0b1;
	const bool is_black = (r == 0 && g == 0 && b == 0);

	SDL_Color color;
	color.r = r << 3;
	color.g = g << 3;
	color.b = b << 3;
	if (special_trasparency_bit) {
		color.a = t ? 255 : 0;
	}
	else {
		color.a = is_black ? (t ? 255 : 0) : (t ? 0 : 255);
	}

	return color;
}

static int colorToInt(const SDL_Color color) {
	return (((int)color.a << 24) | ((int)color.b << 16) | ((int)color.g << 8) | (int)color.r);
}

static Clut morphClut(const Clut& clut, const ClutMorph& morph) {
	Clut result = clut;
	for (unsigned short c = 0; c < clut.size(); c++) {
		const unsigned short t = clut.at(c) & 0x8000;
		const unsigned short sr = clut.at(c) & 31;
		const unsigned short sg = (clut.at(c) >> 5) & 31;
		const unsigned short sb = (clut.at(c) >> 10) & 31;

		unsigned short r = ((sr * morph.rr) + (sg * morph.rg) + (sb * morph.rb)) / 128;
		unsigned short g = ((sr * morph.gr) + (sg * morph.gg) + (sb * morph.gb)) / 128;
		unsigned short b = ((sr * morph.br) + (sg * morph.bg) + (sb * morph.bb)) / 128;

		if (r < 0) r = 0;
		if (r > 31) r = 31;
		if (g < 0) g = 0;
		if (g > 31) g = 31;
		if (b < 0) b = 0;
		if (b > 31) b = 31;

		result.at(c) = t | r | (g << 5) | (b << 10);
	}

	return result;
}

TimLoader::TimLoader(SDL_Renderer* renderer, std::shared_ptr<spdlog::logger> logger, const std::filesystem::path& exportPath, const std::filesystem::path& importPath)
	: renderer(renderer), logger(logger), exportPath(exportPath), importPath(importPath) {
}

TimLoader::~TimLoader() {
	for (SDL_Texture* texture : textures) {
		SDL_DestroyTexture(texture);
	}
}

Tim TimLoader::load(const std::filesystem::path& path, const bool special_trasparency_bit) {
	return loadWithMorphedCluts(path, {}, special_trasparency_bit);
}

Tim TimLoader::loadWithMorphedCluts(const std::filesystem::path& path, const std::initializer_list<ClutMorph>& morphed_cluts, const bool special_trasparency_bit) {
	TimFile timfile;

	{
		std::ifstream stream(path, std::ios::binary);

		if (stream.rdbuf()->sgetn(reinterpret_cast<char*>(&timfile.timHeader), sizeof(TimHeader)) != sizeof(TimHeader)) {
			logger->error("Failed to read TIM header");
			return Tim{};
		}

		if (stream.rdbuf()->sgetn(reinterpret_cast<char*>(&timfile.clutHeader), sizeof(DataHeader)) != sizeof(DataHeader)) {
			logger->error("Failed to read CLUT header");
			return Tim{};
		}

		timfile.clutTable.resize(timfile.clutHeader.height);
		for (int clut_index = 0; clut_index < timfile.clutHeader.height; ++clut_index) {
			Clut& clut = timfile.clutTable.at(clut_index);
			clut.resize(timfile.clutHeader.width);

			if (stream.rdbuf()->sgetn(reinterpret_cast<char*>(clut.data()), sizeof(uint16_t) * clut.size()) != sizeof(uint16_t) * clut.size()) {
				logger->error("Failed to read CLUT data");
				return Tim{};
			}
		}


		if (stream.rdbuf()->sgetn(reinterpret_cast<char*>(&timfile.imageHeader), sizeof(DataHeader)) != sizeof(DataHeader)) {
			logger->error("Failed to read Image header");
			return Tim{};
		}

		timfile.imageData.resize(timfile.imageHeader.width * timfile.imageHeader.height);
		if (stream.rdbuf()->sgetn(reinterpret_cast<char*>(timfile.imageData.data()), sizeof(uint16_t) * timfile.imageData.size()) != sizeof(uint16_t) * timfile.imageData.size()) {
			logger->error("Failed to read Image data");
			return Tim{};
		}

		logger->info("File read successfully");
		logger->debug("TIM tag = {}, version = {}, flags = {}, colorMode = {}, hasClut = {}", timfile.timHeader.tag, timfile.timHeader.version, timfile.timHeader.flags, static_cast<unsigned int>(getColorMode(timfile.timHeader)), hasClut(timfile.timHeader));
		logger->debug("CLUT length = {}, x = {}, y = {}, width = {}, height = {}", timfile.clutHeader.clutLength, timfile.clutHeader.x, timfile.clutHeader.y, timfile.clutHeader.width, timfile.clutHeader.height);
		logger->debug("CLUT data size = {}", sizeof(uint16_t) * timfile.clutHeader.width * timfile.clutHeader.height);
		logger->debug("Image length = {}, x = {}, y = {}, width = {}, height = {}", timfile.imageHeader.clutLength, timfile.imageHeader.x, timfile.imageHeader.y, timfile.imageHeader.width, timfile.imageHeader.height);
		logger->debug("Image data size = {}", sizeof(uint16_t) * timfile.imageData.size());
	}

	const ColorMode color_mode = getColorMode(timfile.timHeader);
	if (color_mode == ColorMode::Indices4bits || color_mode == ColorMode::Indices8bits) {
		Tim tim;

		std::vector<Clut> all_cluts = timfile.clutTable;
		for (const auto& morphed_clut : morphed_cluts) {
			const Clut new_clut = morphClut(timfile.clutTable.front(), morphed_clut);
			all_cluts.emplace_back(std::move(new_clut));
		}

		const std::string color_mode_caption = (color_mode == ColorMode::Indices4bits ? "4-bits" : "8-bits");
		tim.logicalWidth = timfile.imageHeader.width * (color_mode == ColorMode::Indices4bits ? 4 : 2);
		tim.logicalHeight = timfile.imageHeader.height;
		logger->info("Color mode {}, image width = {}, image height = {}, palettes = {}", color_mode_caption, tim.logicalWidth, tim.logicalHeight, all_cluts.size());

		// One copy of the image per palette, stacked vertically. Transparent pixels are left fully zeroed.
		const int complete_width = tim.logicalWidth;
		const int complete_height = tim.logicalHeight * int(all_cluts.size());
		std::vector<int> complete_image(size_t(complete_width) * complete_height, 0);

		for (int clut_index = 0; clut_index < all_cluts.size(); ++clut_index) {
			const Clut& clut = all_cluts.at(clut_index);

			int* pixel = complete_image.data() + size_t(tim.logicalWidth) * tim.logicalHeight * clut_index;

			if (color_mode == ColorMode::Indices4bits) {
				for (const uint16_t value : timfile.imageData) {
					for (const unsigned int offset : {0, 4, 8, 12}) {
						const int index = (value >> offset) & 0b1111;
						const SDL_Color color = getColorFromClut(clut, index, special_trasparency_bit);
						*pixel = color.a ? colorToInt(color) : 0;
						++pixel;
					}
				}
			}
			else if (color_mode == ColorMode::Indices8bits) {
				for (const uint16_t value : timfile.imageData) {
					for (const unsigned int offset : {0, 8}) {
						const int index = (value >> offset) & 0b11111111;
						const SDL_Color color = getColorFromClut(clut, index, special_trasparency_bit);
						*pixel = color.a ? colorToInt(color) : 0;
						++pixel;
					}
				}
			}

			tim.palettedBounds.push_back(SDL_FRect{ 0, float(tim.logicalHeight * clut_index), float(tim.logicalWidth), float(tim.logicalHeight) });
		}

		// colorToInt() packs pixels as 0xAABBGGRR
		const std::unique_ptr<SDL_Surface, void(*)(SDL_Surface*)> surface(SDL_CreateSurfaceFrom(complete_width, complete_height, SDL_PIXELFORMAT_ABGR8888, complete_image.data(), complete_width * int(sizeof(int))), &SDL_DestroySurface);
		if (surface == nullptr) {
			logger->error("Failed to create surface for TIM {}: {}", path.string(), SDL_GetError());
			return Tim{};
		}

		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface.get());
		if (texture == nullptr) {
			logger->error("Failed to create texture for TIM {}: {}", path.string(), SDL_GetError());
		}
		else {
			SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
			SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

			textures.push_back(texture);
			tim.texture = texture;
		}

		if (!exportPath.empty()) {
			const std::filesystem::path export_filename = exportPath / std::filesystem::path(path).replace_extension(".png");
			logger->info("Exporting TIM to path {}", export_filename.string());

			std::filesystem::create_directories(export_filename.parent_path());
			if (!SDL_SavePNG(surface.get(), export_filename.string().c_str())) {
				logger->error("Failed to export TIM to path {}: {}", export_filename.string(), SDL_GetError());
			}
		}

		return tim;
	}
	else if (color_mode == ColorMode::Colors16bits) {
		logger->error("Color mode is 16-bit colors, not supported at the moment");
		return Tim{};
	}
	else if (color_mode == ColorMode::Colors24bits) {
		logger->error("Color mode is 24-bit colors, not supported at the moment");
		return Tim{};
	}

	return Tim{};
}
