/*--------------------------------------------------------------------
	Atari Audio Library v1.20
	Small & accurate ATARI-ST audio emulation
	Arnaud Carré aka Leonard/Oxygene
	@leonard_coder
--------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "YmRenderer.h"
#include "external/lzh.h"

#define	D_DBG_OUTPUT				0

#if D_DBG_OUTPUT
#include <stdio.h>
static FILE*	sDbgH;
#endif

YmRenderer*	YmRenderer::Create(const void* ymMemoryData, uint32_t ymMemorySize, uint32_t hostReplayRate)
{
	YmRenderer* yr = new YmRenderer();
	if ( yr->Load(ymMemoryData, ymMemorySize, hostReplayRate ))
		return yr;
	delete yr;
	return nullptr;
}

YmRenderer::YmRenderer()
{
	m_dataStream = nullptr;
	m_dataStreamStride = 0;
}

YmRenderer::~YmRenderer()
{
}

uint16_t YmRenderer::StreamBE16(const char** r)
{
	const uint8_t* r8 = (const uint8_t*)*r;
	uint16_t v = (r8[0] << 8) | (r8[1]);
	*r = (const char*)(r8+2);
	return v;
}

uint32_t YmRenderer::StreamBE32(const char** r)
{
	uint32_t v = StreamBE16(r);
	v = (v<<16) | StreamBE16(r);
	return v;
}

void YmRenderer::ConvertTo4Bits(void)
{
/*
	// MadMAx 4bits table ripped from Wings Of Death replayer :)
	$0002ea 0007 090a
	$0002ee 0b0c 0c0d
	$0002f2 0d0d 0e0e
	$0002f6 0e0f 0f0f
*/
	static const uint8_t sMadMax4BitsTable[16] = { 0x0, 0x7, 0x9, 0xa, 0xb, 0xc, 0xc, 0xd, 0xd, 0xd, 0xe, 0xe, 0xe, 0xf, 0xf, 0xf };
	for (int s = 0; s < m_sampleCount; s++)
	{
		YmSample& smp = m_samples[s];
		uint8_t* w8 = (uint8_t *)smp.data;	// we can overwrite, we're using our own copy of the data
		for (uint32_t i = 0; i < smp.len; i++)
		{
			uint8_t v = (smp.data[i])>>4;
			w8[i] = sMadMax4BitsTable[v&15];
		}
	}
}

bool YmRenderer::Load(const void* rawYmFile, uint32_t ymFileSize, uint32_t hostReplayRate)
{

	bool ret = false;
	m_innerSamplePos = 0;
	m_samplePerTick = 0;
	m_songDurationSample = 0;
	m_sampleCount = 0;
	m_tick = 0;

	SongInfo& si = m_songInfo;
	si.hostReplayRate = hostReplayRate;

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

	uint32_t ymClock = Ym2149c::kDefaultAtariYmClock;

	const char* r8 = (const char*)si.rawBinaryData;
	const uint32_t sign = ReadBE32(r8);
	switch (sign)
	{
		case e_YM3a:
		case e_YM3b:
		{
			m_dataStreamStride = 14;
			m_flags = 1;		// interleaved
			m_subSongLenInTick[0] = (si.rawBinaryDataSize-4) / m_dataStreamStride; // -4 for header
			si.playerTickRate = 50;
			m_songLoopTick = 0;
			m_ymType = (e_YM3a == sign) ? eYmType::eYM3a : eYmType::eYM3b;
			m_dataStream = (const uint8_t *)r8 + 4;
			if (eYmType::eYM3b == m_ymType)
			{
				const uint32_t* pr = (const uint32_t *)(r8 + si.rawBinaryDataSize - 4);
				m_songLoopTick = *pr;
			}
			m_samplePerTick = si.hostReplayRate / si.playerTickRate;
			ret = true;
		}
		break;
		case e_YM5a://'YM5!':		// Extended YM2149 format, all machines.
		case e_YM6a://'YM6!':		// Extended YM2149 format, all machines.
		{
			if (0 == strncmp(r8 + 4, "LeOnArD!", 8))
			{
				r8 += 12;
				m_subSongLenInTick[0] = StreamBE32(&r8);
				m_flags = StreamBE32(&r8);
				m_sampleCount = StreamBE16(&r8);
				ymClock = StreamBE32(&r8);
				si.playerTickRate = StreamBE16(&r8);
				assert(si.playerTickRate > 0);
				m_songLoopTick = StreamBE32(&r8);
				int skip = StreamBE16(&r8);
				r8 += skip;
				if (m_sampleCount <= kYmMaxSamples)
				{
					if (m_sampleCount > 0)
					{
						for (int s = 0; s < m_sampleCount; s++)
						{
							YmSample& smp = m_samples[s];
							smp.len = StreamBE32(&r8);
							smp.data = (const uint8_t*)r8;
							r8 += smp.len;
						}
						if (0 == (m_flags&4))
							ConvertTo4Bits();
					}

					si.musicName = r8;
					r8 = AUskipNTString(r8);
					si.musicAuthor = r8;
					r8 = AUskipNTString(r8);
					si.converter = r8;
					r8 = AUskipNTString(r8);

					m_dataStream = (const uint8_t *)r8;
					m_dataStreamStride = 16;

					m_ymType = (e_YM5a == sign) ? eYmType::eYM5a : eYmType::eYM6a;
					m_samplePerTick = si.hostReplayRate / si.playerTickRate;
					m_songDurationSample = m_subSongLenInTick[0] * m_samplePerTick;
					ret = true;
				}
			}
		}
		break;
		case e_MIX1:	// 'YMT1'
		{
			m_songInfo.playerTickRate = 50;
			r8 += 12;
			m_flags = StreamBE32(&r8);
			StreamBE32(&r8);			// skip total sample bank size
			m_sampleCount = StreamBE32(&r8);
			if (m_sampleCount <= kYmMaxSamples)
			{
				uint64_t duration = 0;
				for (int i = 0; i < m_sampleCount; i++)
				{
					m_samples[i].data = nullptr;
					m_samples[i].mixStart = StreamBE32(&r8);
					m_samples[i].len = StreamBE32(&r8);
					m_samples[i].repeat = StreamBE16(&r8);
					if (m_samples[i].repeat > 16)
						m_samples[i].repeat = 16;
					m_samples[i].replayRate = StreamBE16(&r8);
					assert(m_samples[i].replayRate > 0);
					duration += (uint64_t(m_samples[i].len * m_samples[i].repeat) * m_songInfo.hostReplayRate) / m_samples[i].replayRate;
				}
				m_songDurationSample = uint32_t(duration);
				si.musicName = r8;
				r8 = AUskipNTString(r8);
				si.musicAuthor = r8;
				r8 = AUskipNTString(r8);
				si.converter = r8;
				r8 = AUskipNTString(r8);
				m_mixBank = (const uint8_t*)r8;
				m_ymType = eYmType::eMIX1;
				m_mixFrac = 0;
				m_mixPatternPos = 0;
				m_mixCurrentRepeat = m_samples[0].repeat;
				m_mixSamplePos = 0;
				ret = true;
			}
		}
		break;
	}

	if (ret)
	{
		assert(ymClock > 0);
		assert(m_songInfo.playerTickRate > 0);

		if (m_songLoopTick >= m_subSongLenInTick[0])
			m_songLoopTick = 0;

		si.ym2149Clock = ymClock;
		si.subsongCount = 1;
		si.defaultSubsong = 1;
		si.fileType = eFileType::eYm;
	}

	return ret;
}

uint32_t YmRenderer::GetSubsongDurationSample(int subsongId) const
{
	if ((subsongId <= 0) || (subsongId > m_songInfo.subsongCount))
		return 0;

	assert(1 == subsongId);
	return m_songDurationSample;
}

bool YmRenderer::InitSubSong(int subSongId)
{
	bool ret = false;
	if ((subSongId >= 1) && (subSongId <= m_songInfo.subsongCount))
	{
		m_innerSamplePos = 0;
		m_tick = 0;

		m_ym2149.Reset(m_songInfo.hostReplayRate, m_songInfo.ym2149Clock);
		m_mfp.Reset(m_songInfo.hostReplayRate);
		MuteVoices(0);		// nothing is muted by default

		// enable timer A & B for potential 2 YM fx
		m_mfp.Write8(0x07, (1 << 5) | (1 << 0));
		m_mfp.Write8(0x13, (1 << 5) | (1 << 0));
		SetTimer(0, 0, 0);
		SetTimer(1, 0, 0);
		ret = true;

		#if D_DBG_OUTPUT
		sDbgH = fopen("ymRecorder_log.txt", "w");
		#endif

	}
	return ret;
}

uint32_t YmRenderer::ComputeCurrentVisualLevels()
{
	if (eYmType::eMIX1 != m_ymType)
		return m_ym2149.ComputeCurrentVisualLevels();

	int8_t v = m_mixLastSample >> 1;
	return uint32_t(v)<<24;
}

int16_t YmRenderer::ComputeNextSample()
{
	int16_t out = 0;
	if (eYmType::eMIX1 != m_ymType)
	{

		out = m_ym2149.ComputeNextSample();

		// tick 2 Atari timers, maybe one of them is running
		for (int t = 0; t < 2; t++)
		{
			if (m_mfp.Tick(t))
			{
				YmFx& fx = m_ymFx[t];
				if (eSid == fx.type)
				{
					fx.fxPhase++;
					const uint8_t r = (fx.fxPhase & 1)?fx.sidVol : 0;
					YmWrite(fx.ymVoice + 8, r);
				}
				else if (eSyncBuzzer == fx.type)
				{
					YmWrite(13, fx.syncBuzzShape);
				}
				else if (eDigidrum == fx.type)
				{
					const YmSample& smp = m_samples[fx.drumId];
					if (fx.fxPhase < smp.len)
					{
						YmWrite(fx.ymVoice + 8, smp.data[fx.fxPhase] & 15);
						fx.fxPhase++;
					}
					else
					{
						// end digidrum, switch off
						SetTimer(t, 0, 0);
						fx.type = eNone;
					}
				}
			}
		}
	}
	else
	{
		// Digimix YM driver
		const YmSample& smp = m_samples[m_mixPatternPos];
		m_mixLastSample = m_mixBank[smp.mixStart+m_mixSamplePos];

		m_mixFrac += smp.replayRate;
		if (m_mixFrac >= m_songInfo.hostReplayRate)
		{
			m_mixSamplePos++;
			if (m_mixSamplePos >= smp.len)
			{
				m_mixSamplePos = 0;
				m_mixCurrentRepeat--;
				if (m_mixCurrentRepeat <= 0)
				{
					m_mixPatternPos++;
					if (m_mixPatternPos >= m_sampleCount)
						m_mixPatternPos = 0;

					m_mixCurrentRepeat = m_samples[m_mixPatternPos].repeat;
				}
			}
			m_mixFrac -= m_songInfo.hostReplayRate;
		}
		out = int16_t(m_mixLastSample) << 7;
	}
	return out;
}

void YmRenderer::AudioRender(int16_t* buffer, uint32_t count)
{
	AudioRenderInternal(buffer, count, nullptr);
}

void YmRenderer::AudioRenderWithVisualInfos(int16_t* buffer, uint32_t sampleCount, uint32_t* pVisualSamples)
{
	AudioRenderInternal(buffer, sampleCount, pVisualSamples);
}

void YmRenderer::YmWrite(int reg, uint8_t d)
{
	m_ym2149.WritePort(0, reg);	// select reg
	m_ym2149.WritePort(2, d);	// write data
}

void YmRenderer::SetTimer(int slot, int prediv, int count)
{
	// drive Atari timer A or B for YM fx
	m_mfp.Write8((0 == slot) ? 0x19 : 0x1b, prediv);
	m_mfp.Write8((0 == slot) ? 0x1f : 0x21, count);
}

uint32_t YmRenderer::YmFxDecode(int fxSlot, int regCode, int regPrediv, int regCount)
{
	uint32_t skipMask = 0;

	YmFx& fx = m_ymFx[fxSlot];

	int code = ReadInterleaved(regCode)&0xf0;
	int prediv = (ReadInterleaved(regPrediv) >> 5) & 7;
	int count = ReadInterleaved(regCount);

	if (eYmType::eYM5a == m_ymType)
	{
		// ym5 fx, convert data into ym6 
		code &= 0x30;
		if (code)
		{
			static const uint8_t sFxCode[2] = {0x0, 0x40}; // ym5 SID & ym5 digidrum
			code |= sFxCode[fxSlot];
		}
	}

	if (code & 0x30)
	{
		fx.ymVoice = ((code&0x30)>>4)-1;
		switch (code & 0xc0)
		{
			case 0x00:		// SID
				fx.type = eSid;
				fx.sidVol = ReadInterleaved(fx.ymVoice + 8) & 15;
				skipMask = 1 << (fx.ymVoice + 8);
				SetTimer(fxSlot, prediv, count);
				break;

			case 0x40:		// DigiDrum
				fx.drumId = ReadInterleaved(fx.ymVoice + 8) & 31;
				if ((fx.drumId>=0) && (fx.drumId<m_sampleCount))
				{
					fx.type = eDigidrum;
					skipMask = 1 << (fx.ymVoice + 8);
					fx.fxPhase = 0;
					SetTimer(fxSlot, prediv, count);
				}
				break;

			case 0xc0:		// Sync-Buzzer.
				fx.type = eSyncBuzzer;
				fx.syncBuzzShape = ReadInterleaved(fx.ymVoice + 8) & 15;
				SetTimer(fxSlot, prediv, count);
				break;

			default:
				assert(false);
				break;
		}
	}
	else
	{
		// no fx, if a fx was running, switch off timer (except for digidrum, they stop by themself)
		if (fx.type != eDigidrum)
		{
			SetTimer(fxSlot, 0, 0);
			fx.type = eNone;
		}
	}

	return skipMask;
}

uint8_t YmRenderer::ReadInterleaved(int reg) const
{
	if (m_flags&1)
		return m_dataStream[m_subSongLenInTick[0]*reg + m_tick];	// stream interleaved

	return m_dataStream[m_tick * m_dataStreamStride + reg];
}


void YmRenderer::PlayerTick()
{

	#if D_DBG_OUTPUT
	int ymRegs[14];
	for (int r=0;r<14;r++)
		ymRegs[r] = ReadInterleaved(r);

	int ms = (m_tick * 1000) / m_songInfo.playerTickRate;

	int perB = (ymRegs[3] << 8) | ymRegs[2];

	fprintf(sDbgH, "F%4d (%3d.%03d) : ", m_tick, ms / 1000, ms % 1000);
	fprintf(sDbgH, "P:$%04x ", perB);

	if (perB > 0)
	{
		int hz = (2000000 / 8) / perB;
		fprintf(sDbgH, "(%4d Hz) ", hz);
	}

	fprintf(sDbgH, "V:$%02x", ymRegs[9]);
	if ( ymRegs[9]&0x10)
		fprintf(sDbgH, "(E) ");
	else
		fprintf(sDbgH, "    ");

	fprintf(sDbgH, "ENV: $%04x S:$%02x", (ymRegs[12] << 8) | ymRegs[11], ymRegs[13]);

	fprintf(sDbgH, "\n");

	#endif
	uint32_t skipMask = 0;
	if ((eYmType::eYM5a == m_ymType) || (eYmType::eYM6a == m_ymType))
	{
		skipMask |= YmFxDecode(0, 1, 6, 14);
		skipMask |= YmFxDecode(1, 3, 8, 15);
	}

	// very specific to ym format: if digidrm is running, switch off noise+tone on the voice
	uint8_t r7 = ReadInterleaved(7);
	for (int fx = 0; fx < 2; fx++)
	{
		if (m_ymFx[fx].type == eDigidrum)
			r7 |= ((1 << 0) | (1 << 3)) << m_ymFx[fx].ymVoice;
	}
	YmWrite(7, r7);
	skipMask |= (1 << 7);

	// some YM files have one frame delay between enabling env and setting env period.
	// Set a very long period to avoid high pich env a single player tick frame
	int envPer = (ReadInterleaved(12) << 8) | ReadInterleaved(11);
	if (0 == envPer)
	{
		YmWrite(11, 0xff);
		YmWrite(12, 0xff);
		skipMask |= (1 << 11) | (1 << 12);
	}

	// send data to YM
	for (int r = 0; r <= 12; r++)
	{
		if (0 == (skipMask&(1 << r)))
			YmWrite(r, ReadInterleaved(r));
	}
	uint8_t r13 = ReadInterleaved(13);
	if (r13 != 0xff)
		YmWrite(13, r13);

	// next ym music frame
	m_tick++;
	if (m_tick >= m_subSongLenInTick[0])
		m_tick = m_songLoopTick;
}

void	YmRenderer::AudioRenderInternal(int16_t* buffer, uint32_t count, uint32_t* pSampleViewInfo)
{
	while (count > 0)
	{
		uint32_t todo = count;
		if (eYmType::eMIX1 != m_ymType)
		{
			if (0 == m_innerSamplePos)
			{
				PlayerTick();
				m_innerSamplePos = m_samplePerTick;
			}

			todo = (m_innerSamplePos <= count) ? m_innerSamplePos : count;
			assert(m_innerSamplePos >= todo);
		}

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
					*pSampleViewInfo++ = ComputeCurrentVisualLevels();
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

void YmRenderer::MuteVoices(uint32_t muteVoiceMask)
{
	m_ym2149.MuteVoices(muteVoiceMask);
}
