#include <vab.hpp>
#include <algorithm>
#include <array>
#include <cmath>
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

// Flags in the second byte of every 16 bytes ADPCM block
constexpr int AdpcmLoopEnd = 1 << 0; // Last block: either stop, or jump back to the loop start
constexpr int AdpcmRepeat = 1 << 1; // On the last block: jump back to the loop start instead of stopping
constexpr int AdpcmLoopStart = 1 << 2;
constexpr int AdpcmSamplesPerBlock = 28;

// The SPU plays a waveform at this rate when the requested note is the center note of the tone
constexpr unsigned int SpuSampleRate = 44100;

struct AdpcmLoop {
	bool looping = false;
	size_t startSample = 0;
	size_t sampleCount = 0;
};

// https://github.com/ColdSauce/psxsdk/blob/master/tools/vag2wav.c
static AdpcmLoop adpcmToWav(std::istream& adpcm, const size_t data_size, const unsigned int samp_freq, std::ostream& wav) {
	AdpcmLoop loop;

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
		wav.put(samp_freq & 0xff);
		wav.put((samp_freq >> 8) & 0xff);
		wav.put((samp_freq >> 16) & 0xff);
		wav.put((samp_freq >> 24) & 0xff);
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
		double samples[AdpcmSamplesPerBlock];
		double s_1 = 0, s_2 = 0;
		const double f[5][2] = { { 0.0, 0.0 },
						{   60.0 / 64.0,  0.0 },
						{  115.0 / 64.0, -52.0 / 64.0 },
						{   98.0 / 64.0, -55.0 / 64.0 },
						{  122.0 / 64.0, -60.0 / 64.0 } };

		size_t offset = 0;

		// Never read past the waveform: the next one starts right after it
		while (offset < data_size) {
			predict_nr = adpcm.get();
			shift_factor = predict_nr & 0xf;
			predict_nr >>= 4;
			flags = adpcm.get();
			// One-shot waveforms are closed by a block that only carries this value
			if (flags == 7 || !adpcm) {
				break;
			}
			if (predict_nr > 4) {
				predict_nr = 0;
			}
			if (flags & AdpcmLoopStart) {
				loop.startSample = loop.sampleCount;
			}
			for (i = 0; i < AdpcmSamplesPerBlock; i += 2) {
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

			for (i = 0; i < AdpcmSamplesPerBlock; i++) {
				samples[i] = samples[i] + s_1 * f[predict_nr][0] + s_2 * f[predict_nr][1];
				s_2 = s_1;
				s_1 = samples[i];
				d = std::clamp((int)(samples[i] + 0.5), -32768, 32767);
				wav.put(d & 0xff);
				wav.put(d >> 8);
			}

			offset += 16;
			loop.sampleCount += AdpcmSamplesPerBlock;

			if (flags & AdpcmLoopEnd) {
				loop.looping = (flags & AdpcmRepeat) != 0;
				break;
			}
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

	return loop;
}

static bool loadWave(SDL_IOStream* io, Vab::Wave& wave) {
	Uint8* raw_buffer = nullptr;
	Uint32 length = 0;

	if (io == nullptr || !SDL_LoadWAV_IO(io, true, &wave.spec, &raw_buffer, &length)) {
		return false;
	}

	const std::unique_ptr<Uint8, void(*)(void*)> buffer(raw_buffer, &SDL_free);
	wave.data.assign(buffer.get(), buffer.get() + length);
	return wave.valid();
}

VabLoader::VabLoader(std::shared_ptr<spdlog::logger> logger, const std::filesystem::path& hdFolder)
	: logger(logger), hdFolder(hdFolder) {
}

Vab VabLoader::load(const std::filesystem::path& path) {
	const std::filesystem::path vh_path = path;
	const std::filesystem::path vb_path = std::filesystem::path(vh_path).replace_extension(".VB");

	Vab vab;
	VhFile vh;

	{
		std::ifstream vh_stream(vh_path, std::ios::binary);

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
				const VhToneAttributes& tone_attribs = vh.programTones.at(program).toneAttributes.at(tone);
				const VagInfo& vag_info = vagInfos.at(tone_attribs.vag - 1);

				Vab::VabSound vabsound;
				vabsound.program = program;
				vabsound.tone = tone;
				vabsound.wave.centerNote = tone_attribs.center;

				// The waveform is always decoded, as it tells whether and where the sound loops
				std::string wav_data;
				AdpcmLoop loop;

				{
					logger->info("Creating WAV from VAB data path {}, program {}, tone {}", path, program, tone);

					std::stringstream wav_stream;
					vb_stream.clear();
					vb_stream.seekg(vag_info.offset, std::ios::beg);
					loop = adpcmToWav(vb_stream, vag_info.size, SpuSampleRate, wav_stream);
					wav_data = wav_stream.str();
				}

				const std::filesystem::path hd_path = hdFolder / fmt::format("{}_{}_{}.wav", path.stem().string(), program, tone);
				const bool load_hd = !hdFolder.empty() && std::filesystem::exists(hd_path);

				if (load_hd) {
					logger->info("Loading HD sound {}", hd_path);

					if (!loadWave(SDL_IOFromFile(hd_path.string().c_str(), "rb"), vabsound.wave)) {
						logger->error("Failed to load HD sound {}: {}", hd_path, SDL_GetError());
					}
				}
				else if (!loadWave(SDL_IOFromConstMem(wav_data.data(), wav_data.size()), vabsound.wave)) {
					logger->error("Failed to create WAV for VAB path {}, program {}, tone {}: {}", path, program, tone, SDL_GetError());
				}

				if (vabsound.wave.valid() && loop.looping && loop.sampleCount > 0) {
					// The HD sound can have any length and format: keep the loop start at the same relative position
					const size_t frame_size = SDL_AUDIO_FRAMESIZE(vabsound.wave.spec);
					const size_t frame_count = vabsound.wave.data.size() / frame_size;
					vabsound.wave.looping = true;
					vabsound.wave.loopStart = (loop.startSample * frame_count / loop.sampleCount) * frame_size;
				}

				if (!hdFolder.empty() && !load_hd && vabsound.wave.valid()) {
					logger->info("Exporting HD asset {}", hd_path);

					std::filesystem::create_directories(hd_path.parent_path());
					std::ofstream hd_stream(hd_path, std::ios::binary);
					hd_stream.write(wav_data.data(), wav_data.size());
					if (!hd_stream) {
						logger->error("Failed to export HD asset {}", hd_path);
					}
				}

				vab.sounds.emplace_back(std::move(vabsound));
			}
		}
	}

	return vab;
}

const Vab::Wave& Vab::getAudio(const int program, const int tone) const {
	for (const VabSound& sound : sounds) {
		if (sound.program == program && sound.tone == tone) {
			return sound.wave;
		}
	}

	static const Wave null_wave;
	return null_wave;
}

static float shortToFloatVolume(const short volume) {
	return float(std::clamp<short>(volume, 0, 127)) / 127;
}

// Called from the audio thread whenever the device wants more than the stream holds: queue the loop again
static void SDLCALL feedLoopingWave(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) {
	const Vab::Wave& wave = *static_cast<const Vab::Wave*>(userdata);
	const Uint8* loop_data = wave.data.data() + wave.loopStart;
	const int loop_size = int(wave.data.size() - wave.loopStart);

	if (loop_size <= 0) {
		return;
	}

	for (int available = SDL_GetAudioStreamAvailable(stream); available >= 0 && available < total_amount; available = SDL_GetAudioStreamAvailable(stream)) {
		if (!SDL_PutAudioStreamData(stream, loop_data, loop_size)) {
			break;
		}
	}
}

Voices::Voices(std::shared_ptr<spdlog::logger> logger) : logger(logger) {
	const SDL_AudioDeviceID device_id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
	if (device_id == 0) {
		logger->error("Failed to open audio device: {}", SDL_GetError());
		return;
	}

	device.reset(new SDL_AudioDeviceID(device_id), [](SDL_AudioDeviceID* device_id) {
		SDL_CloseAudioDevice(*device_id);
		delete device_id;
	});
}

Voices::Voice Voices::voiceOn(const Vab& vab, const int program, const int tone, const int note, const short volume) {
	for (Voice voice = 0; voice < MaxVoices; ++voice) {
		std::shared_ptr<SDL_AudioStream>& stream = streams.at(voice);
		if (stream == nullptr) {
			const Vab::Wave& wave = vab.getAudio(program, tone);
			if (device != nullptr && wave.valid()) {
				// The waveform plays at its own rate on the center note, and one octave higher every 12 notes above it
				const float pitch = std::pow(2.0f, float(note - wave.centerNote) / 12.0f);

				// The stream converts from the wave format to whatever the device wants once bound.
				// Destroying it also unbinds it from the device.
				stream.reset(SDL_CreateAudioStream(&wave.spec, nullptr), &SDL_DestroyAudioStream);
				bool playing = stream != nullptr
					&& SDL_SetAudioStreamGain(stream.get(), shortToFloatVolume(volume))
					&& SDL_SetAudioStreamFrequencyRatio(stream.get(), pitch)
					&& SDL_BindAudioStream(*device, stream.get())
					&& SDL_PutAudioStreamData(stream.get(), wave.data.data(), int(wave.data.size()));

				if (playing && wave.looping) {
					// Keeps playing the loop until the voice is turned off
					playing = SDL_SetAudioStreamGetCallback(stream.get(), &feedLoopingWave, const_cast<Vab::Wave*>(&wave));
				}
				else if (playing) {
					playing = SDL_FlushAudioStream(stream.get());
				}

				if (!playing) {
					logger->error("Failed to play program {}, tone {}: {}", program, tone, SDL_GetError());
					stream.reset();
				}
			}
			return voice;
		}
	}

	return NullVoice;
}

void Voices::voiceOff(const Voice voice) {
	if (voice == NullVoice) {
		return;
	}

	streams.at(voice).reset();
}

void Voices::allVoicesOff() {
	for (Voice voice = 0; voice < MaxVoices; ++voice) {
		voiceOff(voice);
	}
}

void Voices::setVolume(const short volume) {
	if (device != nullptr) {
		SDL_SetAudioDeviceGain(*device, shortToFloatVolume(volume));
	}
}
