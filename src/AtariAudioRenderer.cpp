/*--------------------------------------------------------------------
	Atari Audio Library v1.10
	Small & accurate ATARI-ST audio emulation
	Arnaud Carré aka Leonard/Oxygene
	@leonard_coder
	--------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "AtariAudioRenderer.h"
#include "SndhRenderer.h"
#include "YmRenderer.h"

AtariAudioRenderer* AtariAudioRenderer::Create(const void* fileMemoryData, uint32_t fileMemorySize, uint32_t hostReplayRate)
{
	SndhRenderer* sr = SndhRenderer::Create(fileMemoryData, fileMemorySize, hostReplayRate);
	if (sr)
		return sr;

	YmRenderer* yr = YmRenderer::Create(fileMemoryData, fileMemorySize, hostReplayRate);
	if (yr)
		return yr;

	return nullptr;
}

void AtariAudioRenderer::Destroy(AtariAudioRenderer* ar)
{
	delete ar;
}

AtariAudioRenderer::AtariAudioRenderer()
{
	static const char* sEmptyString = "";
	memset(&m_songInfo, 0, sizeof(m_songInfo));
	m_songInfo.musicName = sEmptyString;
	m_songInfo.musicAuthor = sEmptyString;
	m_songInfo.ripper = sEmptyString;
	m_songInfo.converter = sEmptyString;
	m_songInfo.year = sEmptyString;
}

AtariAudioRenderer::~AtariAudioRenderer()
{
	free((void*)m_songInfo.rawBinaryData);
}
