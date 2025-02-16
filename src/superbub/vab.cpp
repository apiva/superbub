#include <vab.hpp>
#include <array>
#include <vector>
#include <fstream>
#include <filesystem>
#include <inttypes.h>
#include <sstream>
#include <spdlog/fmt/std.h>

#pragma optimize("",off)

// https://jsgroth.dev/blog/posts/ps1-spu-part-1/
// https://github.com/vgmstream/vgmstream/issues/200#issuecomment-394543004

struct VhHeader {
	std::array<char, 4> id;
	uint32_t version;
	uint32_t vabId;
	uint32_t waveformSize;
	uint16_t reserved0;
	uint16_t programCount;
	uint16_t toneCount;
	uint16_t vagCount;
	uint8_t masterVolume;
	uint8_t masterPan;
	uint8_t bankAttribute1;
	uint8_t bankAttribute2;
	uint32_t reserved1;
};

struct VhProgramAttributes {
	uint8_t tones;
	uint8_t volume;
	uint8_t priority;
	uint8_t mode;
	uint8_t pan;
	int8_t reserved0;
	int16_t attribute;
	uint32_t reserved1;
	uint32_t reserved2;
};

struct VhToneAttributes {
	uint8_t prior; /*tone priority (0 - 127); used for controlling allocation when more voices than can be keyed on are requested*/
	uint8_t mode; /*tone mode (0 = normal; 4 = reverb	applied */
	uint8_t vol; /*tone volume*/
	uint8_t pan; /*tone pan*/
	uint8_t center; /*center note (0~127)*/
	uint8_t shift; /*pitch correction (0~127,cent units)*/
	uint8_t min; /*minimum note limit (0~127)*/
	uint8_t max; /*maximum note limit (0~127,	provided min < max)*/
	uint8_t vibW; /*vibrato width (1/128 rate,0~127)*/
	uint8_t vibT; /*1 cycle time of vibrato (tick units)*/
	uint8_t porW; /*portamento width (1/128 rate, 0~127)*/
	uint8_t porT; /*portamento holding time (tick units)*/
	uint8_t pbmin; /*pitch bend (-0~127, 127 = 1 octave)*/
	uint8_t pbmax; /*pitch bend (+0~127, 127 = 1 octave)*/
	uint8_t reserved1; /*system reserved*/
	uint8_t reserved2; /*system reserved*/
	uint16_t adsr1; /*ADSR1*/
	uint16_t adsr2; /*ADSR2*/
	int16_t prog; /*parent program*/
	int16_t vag; /*waveform (VAG) used*/
	int16_t reserved[4]; /*system reserved*/
};

struct VhProgramTones {
	std::array<VhToneAttributes, 16> toneAttributes;
};

struct VhWaveOffsets {
	uint16_t unused0;
	std::array<uint16_t, 254> offsets;
	uint16_t unused1;
};

struct VhFile {
	VhHeader header;
	std::array<VhProgramAttributes, 128> programAttributes;
	std::vector<VhProgramTones> programTones;
	VhWaveOffsets waveOffsets;
};

static_assert(sizeof(VhHeader) == 32);
static_assert(sizeof(VhProgramAttributes) == 16);
static_assert(sizeof(VhToneAttributes) == 32);
static_assert(sizeof(VhWaveOffsets) == 512);

std::shared_ptr<spdlog::logger> Vab::logger;
std::filesystem::path Vab::hdFolder;

// https://github.com/ColdSauce/psxsdk/blob/master/tools/vag2wav.c
static void adpcmToWav(std::istream& adpcm, const size_t data_size, const unsigned int samp_freq, std::ostream& wav) {
	{
		// Write header chunk	
		wav.write("RIFF", 4);
		// Skip file size field for now
		wav.write("\0\0\0\0", 4);

		wav.write("WAVE", 4);
		// Write fmt chunk
		wav.write("fmt ", 4);
		// Write chunk 1 size in little endian format
		wav.put(0x10);
		wav.put(0);
		wav.put(0);
		wav.put(0);
		// Write audio format (1 = PCM)	
		wav.put(1);
		wav.put(0);
		// Number of channels (1)
		wav.put(1);
		wav.put(0);
		// Write sample rate
		wav.put(samp_freq & 0xf);
		wav.put((samp_freq >> 8) & 0xff);
		wav.put(0);
		wav.put(0);
		// Write byte rate (SampleRate * NumChannels * BitsPerSample/8)
		// That would be 44100*1*(16/8), thus 88200.
		wav.put((samp_freq * 2) & 0xff);
		wav.put(((samp_freq * 2) >> 8) & 0xff);
		wav.put(((samp_freq * 2) >> 16) & 0xff);
		wav.put(((samp_freq * 2) >> 24) & 0xff);
		// Write block align (NumChannels * BitsPerSample/8), thus 2
		wav.put(2);
		wav.put(0);
		// Write BitsPerSample
		wav.put(16);
		wav.put(0);

		// Write data chunk
		wav.write("data", 4);

		// Skip SubChunk2Size, we will return to it later
		wav.write("\0\0\0\0", 4);
	}

	{
		int predict_nr, shift_factor, flags;
		int i, d, s;
		double samples[28];
		double s_1 = 0, s_2 = 0;
		const double f[5][2] = { { 0.0, 0.0 },
						{   60.0 / 64.0,  0.0 },
						{  115.0 / 64.0, -52.0 / 64.0 },
						{   98.0 / 64.0, -55.0 / 64.0 },
						{  122.0 / 64.0, -60.0 / 64.0 } };

		size_t offset = 0;

		while (offset < (data_size + 48)) {
			predict_nr = adpcm.get();
			shift_factor = predict_nr & 0xf;
			predict_nr >>= 4;
			flags = adpcm.get();
			if (flags == 7) {
				break;
			}
			for (i = 0; i < 28; i += 2) {
				d = adpcm.get();
				s = (d & 0xf) << 12;
				if (s & 0x8000)
					s |= 0xffff0000;
				samples[i] = (double)(s >> shift_factor);
				s = (d & 0xf0) << 8;
				if (s & 0x8000)
					s |= 0xffff0000;
				samples[i + 1] = (double)(s >> shift_factor);
			}

			for (i = 0; i < 28; i++) {
				samples[i] = samples[i] + s_1 * f[predict_nr][0] + s_2 * f[predict_nr][1];
				s_2 = s_1;
				s_1 = samples[i];
				d = (int)(samples[i] + 0.5);
				wav.put(d & 0xff);
				wav.put(d >> 8);
			}

			offset += 16;
		}
	}

	{
		const uint32_t sz = wav.tellp();

		// Now write ChunkSize
		wav.seekp(4, std::ios::beg);

		wav.put((sz - 8) & 0xff);
		wav.put(((sz - 8) >> 8) & 0xff);
		wav.put(((sz - 8) >> 16) & 0xff);
		wav.put(((sz - 8) >> 24) & 0xff);

		// Now write Subchunk2Size
		wav.seekp(40, std::ios::beg);

		wav.put((sz - 44) & 0xff);
		wav.put(((sz - 44) >> 8) & 0xff);
		wav.put(((sz - 44) >> 16) & 0xff);
		wav.put(((sz - 44) >> 24) & 0xff);
	}
}

Vab Vab::load(const std::filesystem::path& path) {
	const std::filesystem::path vh_path = path;
	const std::filesystem::path vb_path = std::filesystem::path(vh_path).replace_extension(".vb");

	Vab vab;
	VhFile vh;

	{
		std::ifstream vh_stream(vh_path);

		if (vh_stream.rdbuf()->sgetn(reinterpret_cast<char*>(&vh.header), sizeof(VhHeader)) != sizeof(VhHeader)) {
			return Vab{};
		}

		if (vh_stream.rdbuf()->sgetn(reinterpret_cast<char*>(vh.programAttributes.data()), sizeof(VhProgramAttributes) * vh.programAttributes.size()) != sizeof(VhProgramAttributes) * vh.programAttributes.size()) {
			return Vab{};
		}

		vh.programTones.resize(vh.header.programCount);
		if (vh_stream.rdbuf()->sgetn(reinterpret_cast<char*>(vh.programTones.data()), sizeof(VhProgramTones) * vh.programTones.size()) != sizeof(VhProgramTones) * vh.programTones.size()) {
			return Vab{};
		}

		if (vh_stream.rdbuf()->sgetn(reinterpret_cast<char*>(&vh.waveOffsets), sizeof(VhWaveOffsets)) != sizeof(VhWaveOffsets)) {
			return Vab{};
		}
	}

	logger->info("Reading VAB file: path {}, programs {}, tones {}, vags {}", path, vh.header.programCount, vh.header.toneCount, vh.header.vagCount);

	struct VagInfo {
		int vagIndex;
		size_t offset;
		size_t size;
	};

	std::vector<VagInfo> vagInfos;

	size_t current_offset = 0;
	for (int vag = 0; vag < vh.header.vagCount; ++vag) {
		VagInfo info;
		info.vagIndex = vag;
		info.offset = current_offset;
		info.size = size_t(vh.waveOffsets.offsets.at(vag)) << 3;
		current_offset += info.size;
		vagInfos.push_back(info);
	}

	{
		std::ifstream vb_stream(vb_path, std::ios::binary);

		for (int program = 0; program < vh.header.programCount; ++program) {
			const VhProgramAttributes& program_attribs = vh.programAttributes.at(program);
			for (int tone = 0; tone < program_attribs.tones; ++tone) {
				VabSound vabsound;
				vabsound.program = program;
				vabsound.tone = tone;
				memset(&vabsound.sound, 0, sizeof(Sound));

				if (!hdFolder.empty()) {
					const std::filesystem::path hd_path = hdFolder / fmt::format("{}_{}_{}.wav", path.stem().string(), program, tone);
					if (std::filesystem::exists(hd_path)) {
						logger->info("Loading HD sound {}", hd_path);

						vabsound.sound = LoadSound(hd_path.string().c_str());
						if (!IsSoundValid(vabsound.sound)) {
							logger->error("Failed to load HD sound {}", hd_path);
						}

						vab.sounds.emplace_back(std::move(vabsound));
						continue;
					}
				}

				Wave wave;

				{
					logger->info("Creating WAV from VAB data path {}, program {}, tone {}", path, program, tone);

					const VhToneAttributes& tone_attribs = vh.programTones.at(program).toneAttributes.at(tone);
					const VagInfo& vag_info = vagInfos.at(tone_attribs.vag - 1);

					std::stringstream wav_stream;
					vb_stream.seekg(vag_info.offset, std::ios::beg);
					adpcmToWav(vb_stream, vag_info.size, 44100 / 2, wav_stream);
					const std::string wav_data = wav_stream.str();

					wave = LoadWaveFromMemory(".wav", reinterpret_cast<const unsigned char*>(wav_data.data()), wav_data.size());
					if (!IsWaveValid(wave)) {
						logger->error("Failed to create WAV for VAB path {}, program {}, tone {}", path, program, tone);
						vab.sounds.emplace_back(std::move(vabsound));
						continue;
					}
				}

				if (!hdFolder.empty()) {
					const std::filesystem::path hd_path = hdFolder / fmt::format("{}_{}_{}.wav", path.stem().string(), program, tone);
					logger->info("Exporting HD asset {}", hd_path);

					std::filesystem::create_directories(hd_path.parent_path());
					if (!ExportWave(wave, hd_path.string().c_str())) {
						logger->error("Failed to export HD asset {}", hd_path);
					}
				}

				logger->info("Creating sound for VAB file {}, program {}, tone {}", path, program, tone);
				vabsound.sound = LoadSoundFromWave(wave);
				if (!IsSoundValid(vabsound.sound)) {
					logger->error("Failed to create sound for VAB file {}, program {}, tone {}", path, program, tone);
				}

				vab.sounds.emplace_back(std::move(vabsound));
			}
		}
	}

	return vab;
}

Vab::~Vab() {
	for (auto& tone : sounds) {
		UnloadSound(tone.sound);
	}
}
