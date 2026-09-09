/*--------------------------------------------------------------------
	Atari Audio Library v1.10
	Small & accurate ATARI-ST audio emulation
	Arnaud Carré aka Leonard/Oxygene
	@leonard_coder
--------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "YmRenderer.h"
#include "external/lzh.h"

YmRenderer*	YmRenderer::Create(const void* ymMemoryData, uint32_t ymMemorySize, uint32_t hostReplayRate)
{
	YmRenderer* yr = new YmRenderer();
	if ( yr->Load(ymMemoryData, ymMemorySize, hostReplayRate ))
		return yr;
	delete yr;
	return nullptr;
}

void YmRenderer::Destroy(YmRenderer* yr)
{
	delete yr;
}

YmRenderer::YmRenderer()
{
	static const char* sEmptyString = "";
	memset(&m_songInfo, 0, sizeof(m_songInfo));
	m_hostReplayRate = 0;
	m_songInfo.musicName = sEmptyString;
	m_songInfo.musicAuthor = sEmptyString;
	m_songInfo.ripper = sEmptyString;
	m_songInfo.converter = sEmptyString;
	m_songInfo.year = sEmptyString;
	m_dataStream = nullptr;
	m_dataStreamStride = 0;

}

YmRenderer::~YmRenderer()
{
	free((void*)m_songInfo.rawBinaryData);
}

extern uint16_t AURead16(const char* r);
extern uint32_t AURead32(const char* r);
extern const char* AUskipNTString(const char* r);

uint16_t YmRenderer::Read16(const char** r)
{
	const char* r8 = *r;
	uint16_t v = (r8[0] << 8) | (r8[1]);
	*r = r8+2;
	return v;
}

uint32_t YmRenderer::Read32(const char** r)
{
	uint32_t v = Read16(r);
	v = (v<<16) | Read16(r);
	return v;
}

bool YmRenderer::Load(const void* rawYmFile, uint32_t ymFileSize, uint32_t hostReplayRate)
{

	bool ret = false;
	m_hostReplayRate = hostReplayRate;
	SongInfo& si = m_songInfo;

	if (LzhDepacker::IsLzhPacked(rawYmFile, ymFileSize))
	{
		LzhDepacker lzd;
		m_songInfo.rawBinaryData = lzd.Unpack(rawYmFile, ymFileSize, m_songInfo.rawBinaryDataSize);
		if (nullptr == m_songInfo.rawBinaryData)
			return false;
	}
	else
	{
		si.rawBinaryDataSize = ymFileSize;
		si.rawBinaryData = malloc(si.rawBinaryDataSize);
		memcpy((void*)si.rawBinaryData, rawYmFile, ymFileSize);
	}

	uint32_t ymClock = 0;

	const char* r8 = (const char*)si.rawBinaryData;
	switch (AURead32(r8))
	{
		case e_YM5a://'YM5!':		// Extended YM2149 format, all machines.
		case e_YM6a://'YM6!':		// Extended YM2149 format, all machines.
		{
			if (0 == strncmp(r8 + 4, "LeOnArD!", 8))
			{
				r8 += 12;
				m_songLenInTick = Read32(&r8);
				m_flags = Read32(&r8);
				m_sampleCount = Read16(&r8);
				ymClock = Read32(&r8);
				si.playerTickRate = Read16(&r8);
				m_songLoopTick = Read32(&r8);
				int skip = Read16(&r8);
				r8 += skip;
				if (m_sampleCount > 0)
				{
					assert(false);
				}

				si.musicName = r8;
				r8 = AUskipNTString(r8);
				si.musicAuthor = r8;
				r8 = AUskipNTString(r8);
				si.converter = r8;
				r8 = AUskipNTString(r8);

				m_dataStream = (const uint8_t *)r8;
				m_dataStreamStride = 16;
				ret = true;
			}
		}
		break;
	}

	return ret;
}
