/*--------------------------------------------------------------------
	Atari Audio Library v1.10
	Small & accurate ATARI-ST audio emulation
	Arnaud Carré aka Leonard/Oxygene
	@leonard_coder
--------------------------------------------------------------------*/
#pragma once
#include <stdint.h>

class AtariAudioRenderer
{
public:
	struct SongInfo
	{
		int subsongCount;
		int defaultSubsong;
		int playerTickRate;
		uint32_t 	hostReplayRate;
		const char* musicName;
		const char* musicAuthor;
		const char* ripper;
		const char* converter;
		const char* year;
		const void* rawBinaryData;
		uint32_t rawBinaryDataSize;
	};

	static AtariAudioRenderer* Create(const void* fileMemoryData, uint32_t fileMemorySize, uint32_t hostReplayRate);
	static void Destroy(AtariAudioRenderer* ar);
	
	// Get information about the SNDH (like song name, author, amount of subsong, etc.)
	const 	AtariAudioRenderer::SongInfo&	GetSongInfo() const { return m_songInfo; };

	// Get a subsong duration in samples. 0 means there is no information about duration for this subsong
	virtual uint32_t GetSubsongDurationSample(int subsongId) const = 0;

	// Initialize music driver to play a sub-song. By convention, subsongId starts at 1 (not 0)
	// You must call InitSubSong before any call to AudioRender
	virtual bool	InitSubSong(int subSongId) = 0;

	// Main audio rendering function.
	// Compute the next "count" samples into "buffer" (mono, signed, 16bits samples)
	// by default the song will loop. If you want to stop at the perfect end, you can
	// use GetSubsongDurationSample() upfront to get exact amount of samples.
	virtual void AudioRender(int16_t* buffer, uint32_t count) = 0;

protected:
	static	const	int		kSubsongCountMax = 128;

	// Private constructors prevent direct instantiation
	virtual ~AtariAudioRenderer();
	AtariAudioRenderer();
    AtariAudioRenderer(const AtariAudioRenderer&) = delete;            // Prevent copy construction
    AtariAudioRenderer& operator=(const AtariAudioRenderer&) = delete; // Prevent copy assignment

	SongInfo m_songInfo;
	uint32_t	m_subSongLenInTick[kSubsongCountMax];
	uint32_t	m_samplePerTick;
	uint32_t	m_innerSamplePos;
	bool 		m_subsongInit;
};
