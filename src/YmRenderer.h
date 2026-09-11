/*--------------------------------------------------------------------
	Atari Audio Library v1.20
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
	// Read the base AtariAudioRenderer.h header for more details about API
	static YmRenderer* Create(const void* sndhMemoryData, uint32_t sndhMemorySize, uint32_t hostReplayRate);
	const SongInfo&	GetSongInfo() const;
	uint32_t GetSubsongDurationSample(int subsongId) const;
	bool InitSubSong(int subSongId);
	void AudioRender(int16_t* buffer, uint32_t sampleCount);
	void AudioRenderWithVisualInfos(int16_t* buffer, uint32_t sampleCount, uint32_t* pVisualSamples);
	void MuteVoices(uint32_t muteVoiceMask);

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
	uint8_t ReadInterleaved(int reg) const;
	void YmWrite(int reg, uint8_t d);
	void ConvertTo4Bits(void);
	uint32_t ComputeCurrentVisualLevels();

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
		uint32_t mixStart;
		uint32_t len;
		uint16_t repeat;
		uint16_t replayRate;
	};

	uint32_t m_songDurationSample;
	const uint8_t* m_mixBank;
	uint32_t m_mixFrac;
	int m_mixPatternPos;
	int m_mixCurrentRepeat;
	uint32_t m_mixSamplePos;
	int m_sampleCount;
	int8_t m_mixLastSample;
	YmSample m_samples[kYmMaxSamples];


};

