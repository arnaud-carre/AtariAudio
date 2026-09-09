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
	const uint8_t* r8 = (const uint8_t*)*r;
	uint16_t v = (r8[0] << 8) | (r8[1]);
	*r = (const char*)(r8+2);
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
	m_innerSamplePos = 0;
	m_samplePerTick = 0;
	m_songLenInTick = 0;
	m_tick = 0;

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

	if (ret)
	{
		assert(ymClock > 0);
		assert(m_songInfo.playerTickRate > 0);

		m_samplePerTick = m_hostReplayRate / m_songInfo.playerTickRate;
		m_ym2149.Reset(m_hostReplayRate, ymClock);
	}
	else
	{
		free((void*)m_songInfo.rawBinaryData);
		m_songInfo.rawBinaryData = nullptr;
	}

	return ret;
}

uint32_t YmRenderer::GetSongDurationSample() const
{
	return m_songLenInTick * m_samplePerTick;
}

int16_t YmRenderer::ComputeNextSample()
{
	return m_ym2149.ComputeNextSample();
}

void YmRenderer::AudioRender(int16_t* buffer, uint32_t count)
{
	AudioRenderInternal(buffer, count, nullptr);
}

void YmRenderer::YmWrite(int reg, uint8_t d)
{
	m_ym2149.WritePort(0, reg);	// select reg
	m_ym2149.WritePort(2, d);	// write data
}

void YmRenderer::PlayerTick()
{
	for (int r = 0; r <= 12; r++)
		YmWrite(r, ReadInterleaved(r));

	uint8_t r13 = ReadInterleaved(13);
	if (r13 != 0xff)
		YmWrite(13, r13);

	m_tick++;
	if (m_tick >= m_songLenInTick)
		m_tick = 0;
}

void	YmRenderer::AudioRenderInternal(int16_t* buffer, uint32_t count, uint32_t* pSampleViewInfo)
{

	while (count > 0)
	{
		if (0 == m_innerSamplePos)
		{
			PlayerTick();
			m_innerSamplePos = m_samplePerTick;
		}

		uint32_t todo = (m_innerSamplePos <= count) ? m_innerSamplePos : count;
		assert(m_innerSamplePos >= todo);

		if (buffer)
		{
			if (nullptr == pSampleViewInfo)
			{
				for (uint32_t s = 0; s < todo; s++)
					*buffer++ = ComputeNextSample();
			}
			else
			{
				for (uint32_t s = 0; s < todo; s++)
				{
					*buffer++ = ComputeNextSample();
					*pSampleViewInfo++ = 0; //ComputeCurrentVisualLevels();
				}
			}
		}
		else
		{
			// fast forward
			for (uint32_t s = 0; s < todo; s++)
				ComputeNextSample();
		}

		count -= todo;
		m_innerSamplePos -= todo;
	}
}
