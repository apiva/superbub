#include <gflags/gflags.h>
#include <raylib.h>
#include <stdint.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/logger.h>
#include <memory>
#include <fstream>

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

struct Tim {
	TimHeader timHeader;
	DataHeader clutHeader;
	std::vector<uint16_t> clutData;
	DataHeader imageHeader;
	std::vector<uint16_t> imageData;
};

enum ColorMode {
	Indices4bits = 0b00,
	Indices8bits = 0b01,
	Colors16bits = 0b10,
	Colors24bits = 0b11,
};

DEFINE_string(input, "", "Input TIM file");
DEFINE_string(output, "", "Output TIM file");

// https://www.psxdev.net/forum/viewtopic.php?t=109

static ColorMode getColorMode(const TimHeader& header) {
	return ColorMode(header.flags & 0b11);
}

static bool hasClut(const TimHeader& header) {
	return bool((header.flags >> 3) & 0b1);
}

static Color getColorFromClut(const Tim& tim, const int index) {
	const uint16_t value = tim.clutData.at(index);
	const unsigned int r = (value >> 0) & 0b11111;
	const unsigned int g = (value >> 5) & 0b11111;
	const unsigned int b = (value >> 10) & 0b11111;
	const unsigned int t = (value >> 15) & 0b1;
	const bool is_black = (r == 0 && g == 0 && b == 0);

	Color color;
	color.r = r << 3;
	color.g = g << 3;
	color.b = b << 3;
	color.a = is_black ? (t ? 255 : 0) : (t ? 0 : 255);

	return color;
}

static int colorToInt(const Color color)
{
	return (((int)color.a << 24) | ((int)color.b << 16) | ((int)color.g << 8) | (int)color.r);
}

int main(int argc, char* argv[]) {
	gflags::ParseCommandLineFlags(&argc, &argv, false);

	std::shared_ptr<spdlog::sinks::sink> console_sink(new spdlog::sinks::stdout_color_sink_st);
	std::shared_ptr<spdlog::logger> log(new spdlog::logger("tim2png", console_sink));

	log->set_level(spdlog::level::debug);

	log->info("Input filename: {}", FLAGS_input);
	log->info("Output filename: {}", FLAGS_output);
	if (FLAGS_input.empty() || FLAGS_output.empty()) {
		log->error("Input or output path is empty, quitting");
		return 1;
	}

	Tim tim;

	{
		std::ifstream stream(FLAGS_input, std::ios::binary);

		if (stream.rdbuf()->sgetn(reinterpret_cast<char*>(&tim.timHeader), sizeof(TimHeader)) != sizeof(TimHeader)) {
			log->error("Failed to read TIM header");
			return 1;
		}

		if (stream.rdbuf()->sgetn(reinterpret_cast<char*>(&tim.clutHeader), sizeof(DataHeader)) != sizeof(DataHeader)) {
			log->error("Failed to read CLUT header");
			return 1;
		}

		tim.clutData.resize(tim.clutHeader.width * tim.clutHeader.height);
		if (stream.rdbuf()->sgetn(reinterpret_cast<char*>(tim.clutData.data()), sizeof(uint16_t) * tim.clutData.size()) != sizeof(uint16_t) * tim.clutData.size()) {
			log->error("Failed to read CLUT data");
			return 1;
		}

		if (stream.rdbuf()->sgetn(reinterpret_cast<char*>(&tim.imageHeader), sizeof(DataHeader)) != sizeof(DataHeader)) {
			log->error("Failed to read Image header");
			return 1;
		}

		tim.imageData.resize(tim.imageHeader.width * tim.imageHeader.height);
		if (stream.rdbuf()->sgetn(reinterpret_cast<char*>(tim.imageData.data()), sizeof(uint16_t) * tim.imageData.size()) != sizeof(uint16_t) * tim.imageData.size()) {
			log->error("Failed to read Image data");
			return 1;
		}

		log->info("File read successfully");
		log->debug("TIM tag = {}, version = {}, flags = {}, colorMode = {}, hasClut = {}", tim.timHeader.tag, tim.timHeader.version, tim.timHeader.flags, getColorMode(tim.timHeader), hasClut(tim.timHeader));
		log->debug("CLUT length = {}, x = {}, y = {}, width = {}, height = {}", tim.clutHeader.clutLength, tim.clutHeader.x, tim.clutHeader.y, tim.clutHeader.width, tim.clutHeader.height);
		log->debug("CLUT data size = {}", sizeof(uint16_t) * tim.clutData.size());
		log->debug("Image length = {}, x = {}, y = {}, width = {}, height = {}", tim.imageHeader.clutLength, tim.imageHeader.x, tim.imageHeader.y, tim.imageHeader.width, tim.imageHeader.height);
		log->debug("Image data size = {}", sizeof(uint16_t) * tim.imageData.size());
	}

	Image image;
	memset(&image, 0, sizeof(Image));
	image.mipmaps = 1;
	image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

	const ColorMode color_mode = getColorMode(tim.timHeader);

	if (color_mode == ColorMode::Indices4bits) {
		image.width = tim.imageHeader.width * 4;
		image.height = tim.imageHeader.height;
		image.data = RL_MALLOC(sizeof(uint32_t) * image.width * image.height);

		log->info("Color mode is 4-bit indices, resulting image width = {}, height = {}", image.width, image.height);

		int* pixel = reinterpret_cast<int*>(image.data);
		for (const uint16_t value : tim.imageData) {
			for (const unsigned int offset : {0, 4, 8, 12}) {
				const int index = (value >> offset) & 0b1111;
				const Color color = getColorFromClut(tim, index);
				*pixel = colorToInt(color);
				++pixel;
			}
		}
	}
	else if (color_mode == ColorMode::Indices8bits) {
		image.width = tim.imageHeader.width * 2;
		image.height = tim.imageHeader.height;
		image.data = RL_MALLOC(sizeof(uint32_t) * image.width * image.height);

		log->info("Color mode is 8-bit indices, resulting image width = {}, height = {}", image.width, image.height);

		int* pixel = reinterpret_cast<int*>(image.data);
		for (const uint16_t value : tim.imageData) {
			for (const unsigned int offset : {0, 8}) {
				const int index = (value >> offset) & 0b11111111;
				const Color color = getColorFromClut(tim, index);
				*pixel = colorToInt(color);
				++pixel;
			}
		}
	}
	else if (color_mode == ColorMode::Colors16bits) {
		log->error("Color mode is 16-bit colors, not supported at the moment");
		return 1;
	}
	else if (color_mode == ColorMode::Colors24bits) {
		log->error("Color mode is 24-bit colors, not supported at the moment");
		return 1;
	}

	if (!ExportImage(image, FLAGS_output.c_str())) {
		log->error("Failed to export Image");
		return 1;
	}

	UnloadImage(image);

	return 0;
}
