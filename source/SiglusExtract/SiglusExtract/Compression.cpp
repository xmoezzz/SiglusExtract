#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include "compression.h"

#define LOOKBACK 4096 // size of the sliding window buffer
//#define LEVEL 17 // maximum amount of bytes to accept for compression
#define LEVEL 8 // maximum amount of bytes to accept for compression
#define ACCEPTLEVEL 3 // lowest amount of bytes to accept for compression

#define DECOMPDEBUG 0
#define COMPDEBUG 0

int OffTable[LEVEL + 1];

void DecompressData(unsigned char *inbuffer, unsigned char *outbuffer, int decomplen)
{
	void *endbuf = outbuffer + decomplen;
	int totaldecomp = 0;

	for (; outbuffer < endbuf;)
	{
		short loop = 8, len = *inbuffer++;
		for (; loop>0 && outbuffer != endbuf; loop--, len >>= 1, *inbuffer++)
		{
			if (len & 1)
			{
				*outbuffer++ = *inbuffer;
				totaldecomp++;
			}
			else
			{
				unsigned short data = *(unsigned short*)inbuffer++;
				int templen = 0;

				templen = (data & 0x0f) + 2; // ecx
				data >>= 4; // eax

				totaldecomp += templen;
				while (templen-->0)
				{
					*outbuffer++ = *(outbuffer - data);
				}

			}
		}
	}

}

unsigned char *CompressData(unsigned char *inbuffer, int len, int *complen, int compression)
{
	unsigned char *output = (unsigned char*)calloc(len * 4, sizeof(unsigned char)), *outptr = output;
	int outlen = 0, i = 0, curlen = 0;

	output += 8;

	while (curlen <= len)
	{
		unsigned char *ctrlbyte = output;

		outlen++, output++;

		for (i = 0; i<8; i++)
		{
			int offset = 0, mlen = 0, tcurlen = curlen;

			if (tcurlen>0xfff)
				tcurlen = 0xfff;

			if (compression)
			{
				if (curlen + LEVEL>len) // LEVEL can be 2 minimum or 17 maximum
				{
					GenerateTable(inbuffer, LEVEL - ((curlen + LEVEL) - len));
					offset = SearchData(inbuffer, tcurlen, inbuffer, LEVEL - ((curlen + LEVEL) - len), &mlen);
				}
				else
				{
					GenerateTable(inbuffer, LEVEL);
					offset = SearchData(inbuffer, tcurlen, inbuffer, LEVEL, &mlen);
				}
			}

			if (offset == 0)
			{
				unsigned char bitmask[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
				int x = 0;

				*output++ = *inbuffer++, curlen++;
				*ctrlbyte |= bitmask[i];
			}
			else
			{
				unsigned short out = (offset << 4) | mlen & 0x0f;

				*(unsigned short*)output++ = out;

				curlen += mlen + 2, inbuffer += mlen + 2;
				output++, outlen++;
			}

			if (curlen >= len)
				break;

			outlen++;
		}

	}

	outlen += 8;

	*(int*)outptr = outlen;
	*(int*)(outptr + 4) = len;

	*complen = outlen;
	return outptr;
}



void GenerateTable(unsigned char *buffer, int len)
{
	int i = 0, x = 0;

	memset(OffTable, '\0', LEVEL + 1);

	OffTable[0] = -1;
	OffTable[1] = 0;

	for (i = 2; i<len;)
	{
		if (buffer[i - 1] == buffer[x])
		{
			OffTable[i] = x + 1;
			i++, x++;
		}
		else if (x>0)
		{
			x = OffTable[x];
		}
		else
		{
			OffTable[i] = 0;
			i++;
		}
	}
}

int SearchData(unsigned char *buffer, int datalen, unsigned char *comp, int complen, int *outlen)
{
	unsigned char *ptr = buffer - datalen;
	int i = 0, x = 0;
	int matchlen = 0, offset = 0;

	if (complen - 2 <= 0 || datalen <= complen)
		return 0;

	while (x < datalen)
	{
		if (comp[i] == ptr[i + x])
		{
			i++;

			if (i == complen)
			{
				matchlen = i;
				offset = x;
				break;
			}
		}
		else
		{
			if (i>matchlen)
			{
				matchlen = i;
				offset = x;
			}

			x += i - OffTable[i];

			if (OffTable[i] > 0)
				i = OffTable[i];
			else
				i = 0;
		}
	}

	if (matchlen<ACCEPTLEVEL)
		matchlen = 0, offset = 0;
	else
		offset = (int)buffer - ((int)ptr + offset);

	if (matchlen>2)
		matchlen -= 2;

	*outlen = matchlen;
	return offset;
}
