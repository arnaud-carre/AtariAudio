#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include "../../src/AtariAudio.h"
#include "wavwriter.h"
#include "jobSystem.h"

static const int kHostReplayRate = 48000;
static const int kAudioBufferLen = kHostReplayRate*10;	// 10 seconds of audio buffer is enough
static int16_t audioBuffer[kAudioBufferLen];

static const int kJobCount = 8;

// WARNING: For this MT test to work, do NOT forget to remove the non deterministic ym2149 random init state! ( ie m_toneEdges = stdLibRand() )

struct Job
{
	uint32_t bufferSize = 0;
	void*	buffer = nullptr;
	void* sndhFileMemory;
	uint32_t sndhFileMemorySize;

	void Execute(int index)
	{
		bufferSize = 0;
		buffer = nullptr;

		SndhRenderer* sr = SndhRenderer::Create(sndhFileMemory, sndhFileMemorySize, kHostReplayRate);
		if (sr)
		{
			const SndhRenderer::SongInfo& si = sr->GetSongInfo();
			printf("Job %2d: \"%s\" by %s\n", index, si.musicName, si.musicAuthor);

			// Loop over all subsongs
			for (int s = 1; s <= si.subsongCount; s++)
			{
				uint32_t sampleCount = sr->GetSubsongDurationSample(s);
				if (0 == sampleCount)
				{
					// a subsong of duration 0 means SNDH file doesn't provide any duration
					sampleCount = 3*60*kHostReplayRate;		// so decide to play 3 minutes by default
				}
				if (sr->InitSubSong(s))
				{
					const int durationInSec = sampleCount / kHostReplayRate;
					printf("Rendering %d:%02d sec of subsong #%d/#%d (%dHz player)\n", durationInSec / 60, durationInSec % 60, s, si.subsongCount, si.playerTickRate);
					bufferSize = sampleCount * sizeof(int16_t);
					buffer = malloc(bufferSize);
					sr->AudioRender((int16_t *)buffer, sampleCount);
				}
			}
			SndhRenderer::Destroy(sr);
		}
	}

	bool WavSave(const char* sFilename) const
	{
		bool ret = false;
		if ((buffer) && (bufferSize > 0))
		{
			WavWriter ww;
			if (ww.Open(sFilename, kHostReplayRate, 1))
			{
				ww.AddAudioData((const int16_t *)buffer, bufferSize / sizeof(int16_t));
				ww.Close();
			}
		}
		return ret;
	}

};

static Job sJobs[kJobCount];

static bool JobProcessFunction(void* array, int index)
{
	Job& job = ((Job*)array)[index];
	job.Execute(index);
	return true;
}

void* LoadFile(const char* sFilename, uint32_t& sizeOut)
{
	sizeOut = 0;
	void* buffer = nullptr;
	FILE* h = fopen(sFilename, "rb");
	if (h)
	{
		fseek(h, 0, SEEK_END);
		size_t sndhSize = ftell(h);
		buffer = malloc(sndhSize);
		fseek(h, 0, SEEK_SET);
		if (sndhSize == fread(buffer, 1, sndhSize, h))
		{
			sizeOut = uint32_t(sndhSize);
		}
		else
		{
			free(buffer);
			buffer = nullptr;
		}
		fclose(h);
	}
	return buffer;
}

int	main(int argc, char* argv[])
{

	printf("sndh2wav, convert atari SNDH music file into a wav\n");
	printf("Build using AtariAudio library v" ATARI_AUDIO_VERSION "\n");
	printf("https://github.com/arnaud-carre/AtariAudio\n");
	printf("\n");

	uint32_t sndhFileSize;
//	void* sndhFileBuffer = LoadFile("Avenger.sndh", sndhFileSize);
	void* sndhFileBuffer = LoadFile("Bullet_Sequence.sndh", sndhFileSize);
	if ( sndhFileBuffer )
	{
		JobSystem js;

		printf("Launching %d jobs...\n", kJobCount);

		for (int i = 0; i < kJobCount; i++)
		{
			sJobs[i].sndhFileMemory = sndhFileBuffer;
			sJobs[i].sndhFileMemorySize = sndhFileSize;
		}

		js.RunJobs(sJobs, kJobCount, JobProcessFunction);
		js.Complete();

		printf("checking...\n");
		const Job& job0 = sJobs[0];
		assert(job0.buffer);
		bool ok = true;
		for (int i = 1; i < kJobCount; i++)
		{
			bool jobOk = false;
			const Job& job = sJobs[i];
			if (job.buffer && job0.buffer)
			{
				if (job.bufferSize == job0.bufferSize)
				{
					if (0 == memcmp(job0.buffer, job.buffer, job0.bufferSize))
					{
						jobOk = true;
					}
				}
			}

			if (!jobOk)
			{
				printf("ERROR: job %d != job 0\n", i);
				ok = false;
				break;
			}
		}

		for (int i = 0; i < kJobCount; i++)
		{
			char sFilename[_MAX_PATH];
			sprintf(sFilename, "output_mt%d.wav", i);
			sJobs[i].WavSave(sFilename);
			free(sJobs[i].buffer);
		}

		printf("MT test: %s\n", ok ? "OK" : "Fail!");


	}
	return 0;
}
