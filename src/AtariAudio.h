/*--------------------------------------------------------------------
	Atari Audio Library v1.10
	Small & accurate ATARI-ST audio emulation
	Arnaud Carré aka Leonard/Oxygene
	@leonard_coder
--------------------------------------------------------------------*/
#pragma once
#include <stdint.h>
#define	ATARI_AUDIO_VERSION		"1.10"


class AtariAudioRenderer
{
public:
	struct SongInfo
	{
		int subsongCount;
		int defaultSubsong;
		int playerTickRate;
		const char* musicName;
		const char* musicAuthor;
		const char* ripper;
		const char* converter;
		const char* year;
		const void* rawBinaryPlayer;
		uint32_t rawBinaryPlayerSize;
	};

	static AtariAudioRenderer* Create(const void* fileMemoryData, uint32_t fileMemorySize, uint32_t hostReplayRate);
	
	virtual ~AtariAudioRenderer() = default;
	virtual void AudioRender(int16_t* buffer, uint32_t count) = 0;

protected:
	// Private constructors prevent direct instantiation
	AtariAudioRenderer() = default;
    AtariAudioRenderer(const AtariAudioRenderer&) = delete;            // Prevent copy construction
    AtariAudioRenderer& operator=(const AtariAudioRenderer&) = delete; // Prevent copy assignment

};

#include "ym2149c.h"
#include "AtariMachine.h"
#include "SndhRenderer.h"
#include "YmRenderer.h"
