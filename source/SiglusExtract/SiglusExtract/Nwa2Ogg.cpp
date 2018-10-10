#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "my.h"
#include "NwaConverter.h"
#include "StreamWriter.h"
#include "vorbis/vorbisenc.h"


#pragma comment(lib, "..\\Release\\libvorbis_static.lib")


#define READ 1024


void ConvertNwaToVorbisInternal(NtFileMemory& in, LPCWSTR FileName)
{
	ogg_stream_state os; /* take physical pages, weld into a logical
						 stream of packets */
	ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
	ogg_packet       op; /* one raw packet of data for decode */

	vorbis_info      vi; /* struct that stores all the static vorbis bitstream
						 settings */
	vorbis_comment   vc; /* struct that stores all the user comments */

	vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
	vorbis_block     vb; /* local working space for packet->PCM decode */

	signed char readbuffer[READ * 4 + 44]; /* out of the data segment, not the stack */

	int eos = 0, ret;
	int i, founddata;


	/* we cheat on the WAV header; we just bypass 44 bytes (simplest WAV
	header is 44 bytes) and assume that the data is 44.1khz, stereo, 16 bit
	little endian pcm samples. This is just an example, after all. */

	FILE* fout = _wfopen(FileName, L"wb");
	if (!fout)
		return;

	readbuffer[0] = '\0';
	for (i = 0, founddata = 0; i<30 && in.GetPosition32() != in.GetSize32(); i++)
	{
		in.Read(readbuffer, 2);
		if (!strncmp((char*)readbuffer, "da", 2)){
			founddata = 1;
			in.Read(readbuffer, 6);
			break;
		}
	}


	vorbis_info_init(&vi);


	/*********************************************************************
	Encoding using a VBR quality mode.  The usable range is -.1
	(lowest quality, smallest file) to 1. (highest quality, largest file).
	Example quality mode .4: 44kHz stereo coupled, roughly 128kbps VBR

	ret = vorbis_encode_init_vbr(&vi,2,44100,.4);

	---------------------------------------------------------------------

	Encoding using an average bitrate mode (ABR).
	example: 44kHz stereo coupled, average 128kbps VBR

	ret = vorbis_encode_init(&vi,2,44100,-1,128000,-1);

	---------------------------------------------------------------------

	Encode using a quality mode, but select that quality mode by asking for
	an approximate bitrate.  This is not ABR, it is true VBR, but selected
	using the bitrate interface, and then turning bitrate management off:

	ret = ( vorbis_encode_setup_managed(&vi,2,44100,-1,128000,-1) ||
	vorbis_encode_ctl(&vi,OV_ECTL_RATEMANAGE2_SET,NULL) ||
	vorbis_encode_setup_init(&vi));

	*********************************************************************/

	if (vorbis_encode_init_vbr(&vi, 2, 44100, 1.0))
		return;

	vorbis_comment_init(&vc);
	vorbis_comment_add_tag(&vc, "ENCODER", "SiglusExtract");

	vorbis_analysis_init(&vd, &vi);
	vorbis_block_init(&vd, &vb);

	srand(time(NULL));
	ogg_stream_init(&os, rand());


	{
		ogg_packet header;
		ogg_packet header_comm;
		ogg_packet header_code;

		vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
		ogg_stream_packetin(&os, &header); /* automatically placed in its own
										   page */
		ogg_stream_packetin(&os, &header_comm);
		ogg_stream_packetin(&os, &header_code);

		/* This ensures the actual
		* audio data will start on a new page, as per spec
		*/
		while (!eos){
			int result = ogg_stream_flush(&os, &og);
			if (result == 0)break;
			fwrite(og.header, 1, og.header_len, fout);
			fwrite(og.body, 1, og.body_len, fout);
		}

	}

	while (!eos)
	{
		long i;
		LARGE_INTEGER ReadSize;
		ReadSize.QuadPart = 0;

		in.Read(readbuffer, READ * 4, &ReadSize);/* stereo hardwired here */
		long bytes = ReadSize.LowPart;

		if (bytes == 0){
			/* end of file.  this can be done implicitly in the mainline,
			but it's easier to see here in non-clever fashion.
			Tell the library we're at end of stream so that it can handle
			the last frame and mark end of stream in the output properly */
			vorbis_analysis_wrote(&vd, 0);

		}
		else{
			/* data to encode */

			/* expose the buffer to submit data */
			float **buffer = vorbis_analysis_buffer(&vd, READ);

			/* uninterleave samples */
			for (i = 0; i<bytes / 4; i++){
				buffer[0][i] = ((readbuffer[i * 4 + 1] << 8) |
					(0x00ff & (int)readbuffer[i * 4])) / 32768.f;
				buffer[1][i] = ((readbuffer[i * 4 + 3] << 8) |
					(0x00ff & (int)readbuffer[i * 4 + 2])) / 32768.f;
			}

			/* tell the library how much we actually submitted */
			vorbis_analysis_wrote(&vd, i);
		}

		/* vorbis does some data preanalysis, then divvies up blocks for
		more involved (potentially parallel) processing.  Get a single
		block for encoding now */
		while (vorbis_analysis_blockout(&vd, &vb) == 1){

			/* analysis, assume we want to use bitrate management */
			vorbis_analysis(&vb, NULL);
			vorbis_bitrate_addblock(&vb);

			while (vorbis_bitrate_flushpacket(&vd, &op)){

				/* weld the packet into the bitstream */
				ogg_stream_packetin(&os, &op);

				/* write out pages (if any) */
				while (!eos){
					int result = ogg_stream_pageout(&os, &og);
					if (result == 0)break;
					fwrite(og.header, 1, og.header_len, fout);
					fwrite(og.body, 1, og.body_len, fout);

					/* this could be set above, but for illustrative purposes, I do
					it here (to show that vorbis does know where the stream ends) */

					if (ogg_page_eos(&og))eos = 1;
				}
			}
		}
	}

	/* clean up and exit.  vorbis_info_clear() must be called last */

	ogg_stream_clear(&os);
	vorbis_block_clear(&vb);
	vorbis_dsp_clear(&vd);
	vorbis_comment_clear(&vc);
	vorbis_info_clear(&vi);

	fclose(fout);
}

void ConvertNwaToVorbis(NtFileDisk& in, LPCWSTR FileName)
{
	StreamWriter WavStream;
	NtFileMemory WavMemory;

	ConvertNwaToWav(in, WavStream);

	WavMemory.Open(WavStream.GetBuffer(), WavStream.GetSize());
	ConvertNwaToVorbisInternal(WavMemory, FileName);
}
