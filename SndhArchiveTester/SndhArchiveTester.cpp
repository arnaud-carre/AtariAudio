#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "extern/zip/src/zip.h"
#include "jobSystem.h"
#include "../src/AtariAudio.h"

static const int kMaxZipWorkers = 16;

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
	int m_entryCount;
	ZipEntry* m_entries;
};

bool ZipWalker::sJobZipItemProcessing(void* user, int itemId, int workerId)
{
	ZipWalker* zw = (ZipWalker*)user;
	ZipEntry& e = zw->m_entries[itemId];
	return zw->JobZipItemProcessing(itemId, workerId);
}

bool ZipWalker::JobZipItemProcessing(int itemId, int workerId)
{
	ZipEntry& e = m_entries[itemId];
	struct zip_t* hz = m_zipPerWorker[workerId];

	if (0 == zip_entry_openbyindex(hz, itemId))
	{
		int isdir = zip_entry_isdir(hz);
		if (!isdir)
		{
			const char* fname = zip_entry_name(hz);
			e.sFilename = _strdup(fname);
			printf("%s\n", e.sFilename);
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

		printf("browsing %d ZIP entries using %d threads...\n", m_entryCount, workers);

		for (int w = 1; w < workers; w++)
			m_zipPerWorker[w] = zip_open(sFilename, 0, 'r');

		JobSystem js;
		js.RunJobs(this, m_entryCount, sJobZipItemProcessing, nullptr, workers);
		int n = js.Join();

		for (int w = 0; w < workers; w++)
			zip_close(m_zipPerWorker[w]);
	}
}


int main()
{

	ZipWalker zw;

	zw.Browse("sndh2026_lf.zip");

	return 0;
}
