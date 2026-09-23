//----------------------------------------------------------
//
//	AtariAudio 1.25
//	Small & accurate ATARI-ST audio emulation
//	by Arnaud Carré aka Leonard/Oxygene (@leonard_coder)
//
//----------------------------------------------------------
#pragma once
#include <stdint.h>

class AtariAudioRenderer
{
public:

	static const uint32_t kDefaultAtariYmClock = 2000000;

	enum class eFileType
	{
		eUnknown,
		eSndh,
		eYm,
	};

	struct SongInfo
	{
		int subsongCount;					// amount of sub-song (.sndh could have several, .ym has 1)
		int defaultSubsong;					// main subsong in case of multiple sub-songs
		int playerTickRate;					// original music driver tick rate (typically 50Hz)
		uint32_t 	hostReplayRate;			// emulation output rate (typically 48kHz)
		uint32_t ym2149Clock;				// ym2149 audio chip clock (typically 2MHz for Atari ST)
		eFileType fileType;					// file type (.sndh or .ym)
		const char* musicName;				// music name or empty string ""
		const char* musicAuthor;			// music author or empty string ""
		const char* ripper;					// music ripper or empty string ""
		const char* converter;				// music converter or empty string ""
		const char* year;					// music year or empty string ""
		const char* fileFormat;				// detail file format string ("SNDH", "YM 5", etc.)
		const void* rawBinaryData;			// raw un-packed music file data
		uint32_t rawBinaryDataSize;			// raw un-packed music file data size (in bytes)
	};

	// Create a AtariAudioRenderer instance from a .sndh or .ym file data located in memory
	// hostReplayRate is the rate you want to render audio stream ( ie 48000 for 48kHz )
	// defaultYm2149Clock is only used for some song file format that doesn't include ym clock (ym2 or ym3)
	// The input .sndh file could be ICE! packed and .ym could be LZH packed
	// After create you can free fileMemoryData if needed (AtariAudioRenderer keeps an internal copy of the required data)
	static AtariAudioRenderer* Create(const void* fileMemoryData, uint32_t fileMemorySize, uint32_t hostReplayRate, uint32_t defaultYm2149Clock = kDefaultAtariYmClock);
	static void Destroy(AtariAudioRenderer* ar);
	
	// Get information about the SNDH (like song name, author, amount of subsong, etc.)
	const 	AtariAudioRenderer::SongInfo&	GetSongInfo() const { return m_songInfo; };

	// Get a subsong duration in samples. 0 means there is no information in the file about duration for this subsong
	virtual uint32_t GetSubsongDurationSample(int subsongId) const = 0;

	// Initialize music driver to play a sub-song. By convention, subsongId starts at 1 (not 0)
	// You must call InitSubSong before any call to AudioRender, even for a single sub song file (like .ym)
	virtual bool	InitSubSong(int subSongId) = 0;

	// Main audio rendering function.
	// Compute the next "count" samples into "buffer" (mono, signed, 16bits samples)
	// By default the song will loop. If you want to stop at the perfect song duration, you can use GetSubsongDurationSample() upfront to get exact amount of samples to generate
	// Of course rendering can be broken into smaller blocks by invoking AudioRender iteratively with reduced buffer sizes
	virtual void AudioRender(int16_t* buffer, uint32_t count) = 0;

	// Fast forward into the music. Doesn't output data, but perform full emulation
	void FastForward(uint32_t count) { AudioRender(nullptr, count); }

	// Helper time unit convert functions
	uint32_t SampleToMs(uint32_t sample) const;
	uint32_t MsToSample(uint32_t ms) const;

	//-------------------------------------------------------------------------
	// Additional functions for high level players
	//-------------------------------------------------------------------------

	// Same as AudioRender but also fills pVisualSamples buffer with 1 32bits per sample
	// the 32bits contains vu meter values for 3 ym voices and STE DAC in form of 8888
	// Use it if you want to draw some per voice vu meter in a player
	virtual void AudioRenderWithVisualInfos(int16_t* buffer, uint32_t sampleCount, uint32_t* pVisualSamples) = 0;

	// Set a mute mask to artifically mute some YM or STE dac voices
	// NOTE: InitSubSong always un-mute everything. So MuteVoices should be called after InitSubsong
	static const uint32_t kYMVoiceA = (1 << 0);
	static const uint32_t kYMVoiceB = (1 << 1);
	static const uint32_t kYMVoiceC = (1 << 2);
	static const uint32_t kSTEDac = (1 << 3);
	virtual void MuteVoices(uint32_t muteVoiceMask) = 0;

protected:
	static	const	int		kSubsongCountMax = 128;

	// preventing direct instantiation
	virtual ~AtariAudioRenderer();
	AtariAudioRenderer();
    AtariAudioRenderer(const AtariAudioRenderer&) = delete;
    AtariAudioRenderer& operator=(const AtariAudioRenderer&) = delete;

	static eFileType QuickFileTypeCheck(const void* rawMemory, uint32_t rawSize);
	uint16_t ReadBE16(const char* r);
	uint32_t ReadBE32(const char* r);
	const char* SkipNTString(const char* r);

	SongInfo m_songInfo;
	uint32_t	m_subSongLenInTick[kSubsongCountMax];
	uint32_t	m_samplePerTick;
	uint32_t	m_innerSamplePos;
	bool 		m_subsongInit;
};
