#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "extern/zip/src/zip.h"
#include "jobSystem.h"
#include "../src/AtariAudio.h"

static const int kMaxZipWorkers = 16;
static const int kHostReplayRate = 48000;

static const int kTestBufferLen = kHostReplayRate * 1;	// 10 seconds
static const int kTestBufferLenBytes = kTestBufferLen*sizeof(int16_t);	// 10 seconds

struct ZipEntry
{
	const char* sFilename;
	bool directory;
	SndhRenderer* sr;
};

class ZipWalker
{
public:
	ZipWalker()
	{
	}

	void Browse(const char* sFilename);


private:
	static bool sJobZipItemProcessing(void* user, int itemId, int workerId);
	bool JobZipItemProcessing(int itemId, int workerId);
	struct zip_t* m_zipPerWorker[kMaxZipWorkers];
	int16_t* m_audioBuffer[kMaxZipWorkers];
	int m_entryCount;
	ZipEntry* m_entries;
	std::atomic<int> m_entryOk;
	std::atomic<int> m_entryFail;
};

bool ZipWalker::sJobZipItemProcessing(void* user, int itemId, int workerId)
{
	ZipWalker* zw = (ZipWalker*)user;
	ZipEntry& e = zw->m_entries[itemId];
	return zw->JobZipItemProcessing(itemId, workerId);
}

static bool IsSilent(const int16_t* buffer, int len)
{
	for (int i = 0; i < len; i++)
	{
		if (buffer[i])
			return false;
	}
	return true;
}

bool ZipWalker::JobZipItemProcessing(int itemId, int workerId)
{
	ZipEntry& e = m_entries[itemId];
	struct zip_t* hz = m_zipPerWorker[workerId];
	int16_t* audioBuffer = m_audioBuffer[workerId];

	if (0 == zip_entry_openbyindex(hz, itemId))
	{
		int isdir = zip_entry_isdir(hz);
		if (!isdir)
		{
			const char* fname = zip_entry_name(hz);
			e.sFilename = _strdup(fname);
			bool ok = false;
			size_t size = zip_entry_size(hz);
			void* unpack = malloc(size);
			size_t depackSize = zip_entry_noallocread(hz, unpack, size);
			if (depackSize == size)
			{
				SndhRenderer* sr = SndhRenderer::Create(unpack, uint32_t(size), kHostReplayRate);		// dummy host replay rate
				if (sr)		// dummy host replay rate
				{
					const SndhRenderer::SongInfo& si = sr->GetSongInfo();
					bool noTiming = false;
					for (int s = 0; s < si.subsongCount; s++)
					{
						noTiming |= (0 == sr->GetSubsongDurationSample(s + 1));

						if (sr->InitSubSong(s + 1))
						{
							memset(audioBuffer, 0, kTestBufferLenBytes);
							sr->AudioRender(audioBuffer, kTestBufferLen);
							{
								if ( !IsSilent(audioBuffer, kTestBufferLen))
									ok = true;
							}
						}
					}

					if (!ok)
					{
						printf("ERROR: %s\n", e.sFilename);
					}
					SndhRenderer::Destroy(sr);
				}
			}
			else
			{
				printf("ERROR during ZIP depacking! (%s)\n", e.sFilename);
			}
			free(unpack);

			if (ok)
				m_entryOk.fetch_add(1);
			else
				m_entryFail.fetch_add(1);

//			printf("%s\n", e.sFilename);
		}
		else
		{
			e.directory = true;
//			printf("%s\n", zip_entry_name(hz));
		}
	}
	zip_entry_close(hz);
	return true;
}

void ZipWalker::Browse(const char* sFilename)
{
	m_zipPerWorker[0] = zip_open(sFilename, 0, 'r');
	if (m_zipPerWorker[0])
	{
		m_entryCount = int(zip_entries_total(m_zipPerWorker[0]));

		m_entries = (ZipEntry*)malloc(m_entryCount * sizeof(ZipEntry));
		memset(m_entries, 0, m_entryCount * sizeof(ZipEntry));

		int workers = JobSystem::GetHardwareWorkerCount();
		if (workers > kMaxZipWorkers)
			workers = kMaxZipWorkers;

		for (int w = 0; w < workers; w++)
			m_audioBuffer[w] = (int16_t *)malloc(kTestBufferLenBytes);

		printf("browsing %d ZIP entries using %d threads...\n", m_entryCount, workers);

		for (int w = 1; w < workers; w++)
			m_zipPerWorker[w] = zip_open(sFilename, 0, 'r');

		JobSystem js;
		m_entryOk = 0;
		m_entryFail = 0;
		js.RunJobs(this, m_entryCount, sJobZipItemProcessing, nullptr, workers);
		int n = js.Join();

		for (int w = 0; w < workers; w++)
		{
			zip_close(m_zipPerWorker[w]);
			free(m_audioBuffer[w]);
		}

		printf("Entry ok..: %d\n", int(m_entryOk));
		printf("Entry fail: %d\n", int(m_entryFail));


	}
}


int main()
{

	ZipWalker zw;

	zw.Browse("sndh2026_lf.zip");


	return 0;
}
