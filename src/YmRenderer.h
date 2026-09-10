/*--------------------------------------------------------------------
	Atari Audio Library v1.10
	Small & accurate ATARI-ST audio emulation
	Arnaud Carré aka Leonard/Oxygene
	@leonard_coder
--------------------------------------------------------------------*/
#pragma once
#include <stdint.h>
#include "AtariAudioRenderer.h"
#include "ym2149c.h"
#include "Mk68901.h"

class YmRenderer : public AtariAudioRenderer
{
public:
	// Create a YmRenderer instance from a YM file data located in memory
	// The input YM data could be LZH packed
	// hostReplayRate is the rate you want to render audio stream ( ie 44100 for 44.1Khz )
	// After create you can free sndhMemoryData if needed (SndhRenderer keep an internal copy of the required data)
	static YmRenderer*	Create(const void* ymMemoryData, uint32_t ymMemorySize, uint32_t hostReplayRate);

	// Get information about the SNDH (like song name, author, amount of subsong, etc.)
	const 	AtariAudioRenderer::SongInfo&	GetSongInfo() const;

	// Get a subsong duration in samples. 0 means there is no information about duration for this subsong
	uint32_t GetSubsongDurationSample(int subsongId) const;

	// Initialize music driver to play a sub-song. By convention, subsongId starts at 1 (not 0)
	// You must call InitSubSong before any call to AudioRender
	bool	InitSubSong(int subSongId);

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
	enum class eYmType
	{
		eUnknown,
		eYM2a,
		eYM3a,
		eYM3b,
		eYM4a,
		eYM5a,
		eYM6a,
		eMIX1,
		eYMT1,
		eYMT2,
	};

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

	enum eYmFxType
	{
		eNone,
		eSid,
		eSinSid,
		eSyncBuzzer,
		eDigidrum,
	};

	struct YmFx
	{
		eYmFxType type = eNone;
		int ymVoice;
		uint32_t fxPhase;
		uint8_t sidVol;
		uint8_t syncBuzzShape;
		int drumId;
	};


	void PlayerTick();
	void SetTimer(int slot, int prediv, int count);
	uint32_t YmFxDecode(int fxSlot, int regCode, int regPrediv, int regCount);
	int16_t ComputeNextSample(void);
	bool	Load(const void* rawYmFile, uint32_t ymFileSize, uint32_t hostReplayRate);
	void		AudioRenderInternal(int16_t* buffer, uint32_t count, uint32_t* pSampleViewInfo);
	uint8_t ReadInterleaved(int reg) const { return m_dataStream[m_subSongLenInTick[0]*reg + m_tick]; }
	void YmWrite(int reg, uint8_t d);
	void ConvertTo4Bits(void);

	uint16_t StreamBE16(const char** r);
	uint32_t StreamBE32(const char** r);

	Ym2149c m_ym2149;
	Mk68901 m_mfp;

	YmFx m_ymFx[2];
	uint32_t m_tick;
	uint32_t m_songLoopTick;
	uint32_t m_flags;
	eYmType m_ymType;
	const uint8_t* m_dataStream;
	int m_dataStreamStride;

	static const int kYmMaxSamples = 128;
	struct YmSample
	{
		const uint8_t* data;
		uint32_t len;
		uint32_t replen;
	};

	int m_sampleCount;
	YmSample m_samples[kYmMaxSamples];
};

