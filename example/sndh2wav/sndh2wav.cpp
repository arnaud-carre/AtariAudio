#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include "../../src/AtariAudio.h"
#include "WavWriter.h"

static const int kHostReplayRate = 44100;
static const int kAudioBufferLen = kHostReplayRate*10;	// 10 seconds of audio buffer is enough

static int16_t audioBuffer[kAudioBufferLen];

void* LoadFile(const char* sFilename, size_t& sizeOut)
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
		fread(buffer, 1, sndhSize, h);
		sizeOut = sndhSize;
		fclose(h);
	}
	return buffer;
}



int	main(int argc, char* argv[])
{

	SndhFile sndh;

	printf("sndh2wav, convert atari SNDH music file into a wav\n");
	printf("Build using AtariAudio library v" ATARI_AUDIO_VERSION "\n");
	printf("\n");
	if (argc != 3)
	{
		printf("Usage:\n"
			   "\tsndh2wav <sndh file> <wav file>\n");
		return -1;
	}

	size_t sndhFileSize;
	void* sndhFileBuffer = LoadFile(argv[1], sndhFileSize);
	if ( sndhFileBuffer )
	{
		WavWriter wavWriter;
		if (wavWriter.Open(argv[2], kHostReplayRate, 1))
		{
			if (sndh.Load(sndhFileBuffer, int(sndhFileSize), kHostReplayRate))
			{
				int subsongCount = sndh.GetSubsongCount();

				// Loop over all subsongs
				for (int s = 1; s <= subsongCount; s++)
				{
					if (sndh.InitSubSong(s))
					{
						SndhFile::SubSongInfo info;
						sndh.GetSubsongInfo(s, info);
						const int duration = info.playerTickCount / info.playerTickRate;
						printf("Rendering %d:%02d sec of subsong #%d/#%d (%dHz player)\n", duration / 60, duration % 60, s, subsongCount, info.playerTickRate);

						// loop till the end
						int len;
						do
						{
							len = sndh.AudioRender(audioBuffer, kAudioBufferLen);
							wavWriter.AddAudioData(audioBuffer, len);
						}
						while (len == kAudioBufferLen);						
					}
				}
				sndh.Unload();
			}
			wavWriter.Close();
		}
	}
	return 0;
}
