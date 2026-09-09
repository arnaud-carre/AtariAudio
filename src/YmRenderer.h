/*--------------------------------------------------------------------
	Atari Audio Library v1.10
	Small & accurate ATARI-ST audio emulation
	Arnaud Carré aka Leonard/Oxygene
	@leonard_coder
--------------------------------------------------------------------*/
#pragma once
#include <stdint.h>
#include "ym2149c.h"
#include "Mk68901.h"

class YmRenderer
{
public:
	struct SongInfo
	{
		int playerTickRate;
		const char* musicName;
		const char* musicAuthor;
		const char* ripper;
		const char* converter;
		const char* year;
		const void* rawBinaryData;
		uint32_t rawBinaryDataSize;
	};

	// Create a YmRenderer instance from a YM file data located in memory
	// The input YM data could be LZH packed
	// hostReplayRate is the rate you want to render audio stream ( ie 44100 for 44.1Khz )
	// After create you can free sndhMemoryData if needed (SndhRenderer keep an internal copy of the required data)
	static YmRenderer*	Create(const void* ymMemoryData, uint32_t ymMemorySize, uint32_t hostReplayRate);

	// Destroy SndhRenderer object
	static void Destroy(YmRenderer* yr);

	// Get information about the SNDH (like song name, author, amount of subsong, etc.)
//	const 	SongInfo&	GetSongInfo() const { return m_songInfo; };

	// Get song duration in samples. 0 means there is no information about duration for this song
	uint32_t GetSongDurationSample() const;

	// Same as GetSongDurationSample, but returned value is in millisec
	uint32_t GetSongDurationMs() const;

	// Main audio rendering function.
	// Compute the next "count" samples into "buffer" (mono, signed, 16bits samples)
	// by default the song will loop. If you want to stop at the perfect end, you can
	// use GetSongDurationSample() upfront to get exact amount of samples.
	void	AudioRender(int16_t* buffer, uint32_t sampleCount);

	//-------------------------------------------------------------------------
	// Additional functions for high level players
	//-------------------------------------------------------------------------

	// Same as AudioRender but also fills pVisualSamples buffer with 1 32bits per sample
	// the 32bits contains vu meter values for 3 ym voices and STE DAC in form of 8888
	// Use it if you want to draw some per voice vu meter in a player
	void	AudioRenderWithVisualInfos(int16_t* buffer, uint32_t sampleCount, uint32_t* pVisualSamples);

	// Set a mute mask to artifically mute some YM or STE dac voices
	// NOTE: InitSubSong always un-mute everything. So MuteVoices should be called after InitSubsong
	static const uint32_t kYMVoiceA = (1 << 0);
	static const uint32_t kYMVoiceB = (1 << 1);
	static const uint32_t kYMVoiceC = (1 << 2);
//	void MuteVoices(uint32_t muteVoiceMask) { m_atariMachine.MuteVoices(muteVoiceMask); }

private:
    // Private constructors prevent direct instantiation
    YmRenderer();
    ~YmRenderer();
    YmRenderer(const YmRenderer&) = delete;            // Prevent copy construction
    YmRenderer& operator=(const YmRenderer&) = delete; // Prevent copy assignment

	enum
	{
		e_YM2a = ('Y' << 24) | ('M' << 16) | ('2' << 8) | ('!'),	//'YM2!'
		e_YM3a = ('Y' << 24) | ('M' << 16) | ('3' << 8) | ('!'),	//'YM3!'
		e_YM3b = ('Y' << 24) | ('M' << 16) | ('3' << 8) | ('b'),	//'YM3b'
		e_YM4a = ('Y' << 24) | ('M' << 16) | ('4' << 8) | ('!'),	//'YM4!'
		e_YM5a = ('Y' << 24) | ('M' << 16) | ('5' << 8) | ('!'),	//'YM5!'
		e_YM6a = ('Y' << 24) | ('M' << 16) | ('6' << 8) | ('!'),	//'YM6!'
		e_MIX1 = ('M' << 24) | ('I' << 16) | ('X' << 8) | ('1'),	//'MIX1'
		e_YMT1 = ('Y' << 24) | ('M' << 16) | ('T' << 8) | ('1'),	//'YMT1'
		e_YMT2 = ('Y' << 24) | ('M' << 16) | ('T' << 8) | ('2'),	//'YMT2'
	};

	void PlayerTick();
	int16_t ComputeNextSample(void);
	bool	Load(const void* rawYmFile, uint32_t ymFileSize, uint32_t hostReplayRate);
	void		AudioRenderInternal(int16_t* buffer, uint32_t count, uint32_t* pSampleViewInfo);
	uint8_t ReadInterleaved(int reg) const { return m_dataStream[m_songLenInTick*reg + m_tick]; }
	void YmWrite(int reg, uint8_t d);

	uint16_t Read16(const char** r);
	uint32_t Read32(const char** r);

	SongInfo m_songInfo;
	Ym2149c m_ym2149;
	Mk68901 m_mfp;

	uint32_t m_tick;
	uint32_t	m_songLenInTick;
	uint32_t m_songLoopTick;
	uint32_t	m_samplePerTick;
	uint32_t	m_innerSamplePos;
	uint32_t 	m_hostReplayRate;
	uint32_t m_flags;
	const uint8_t* m_dataStream;
	int m_dataStreamStride;
	int m_sampleCount;
};

