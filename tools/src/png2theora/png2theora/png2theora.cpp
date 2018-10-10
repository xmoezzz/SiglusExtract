/********************************************************************
*                                                                  *
* THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
* USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
* GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
* IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
*                                                                  *
* THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2009                *
* by the Xiph.Org Foundation http://www.xiph.org/                  *
*                                                                  *
********************************************************************

function: example dumpvid application; dumps  Theora streams
last mod: $Id: dump_video.c,v 1.2 2004/03/24 19:12:42 derf Exp $

********************************************************************/

/* By Mauricio Piacentini (mauricio at xiph.org) */
/*  simply dump decoded YUV data, for verification of theora bitstream */

#if !defined(_REENTRANT)
#define _REENTRANT
#endif
#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#if !defined(_LARGEFILE_SOURCE)
#define _LARGEFILE_SOURCE
#endif
#if !defined(_LARGEFILE64_SOURCE)
#define _LARGEFILE64_SOURCE
#endif
#if !defined(_FILE_OFFSET_BITS)
#define _FILE_OFFSET_BITS 64
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
/*Yes, yes, we're going to hell.*/
#if defined(_WIN32)
#include <io.h>
#endif
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include "theora/theoradec.h"
#include "ogg/ogg.h"
#include "ogg/os_types.h"
#include "png.h"
#include <Windows.h>

#include "Color1.h"
#include "Color2.h"
#include "Color3.h"

#pragma comment(lib, "libogg_static.lib")
#pragma comment(lib, "libtheora_static.lib")
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "libpng16.lib")

/* Helper; just grab some more compressed bitstream and sync it for
page extraction */
int buffer_data(FILE *in, ogg_sync_state *oy){
	char *buffer = ogg_sync_buffer(oy, 4096);
	int bytes = fread(buffer, 1, 4096, in);
	ogg_sync_wrote(oy, bytes);
	return(bytes);
}

struct MemStream
{
	U8* start;
	U8* cur;
	unsigned len;
};


static void PngWrite(png_struct* png, png_byte* buff, png_size_t len)
{
	auto stream = (MemStream*)png_get_io_ptr(png);
	if (stream->cur - stream->start + len > stream->len)
	{
		png_error(png, "err");
	}

	memcpy(stream->cur, buff, len);
	stream->cur += len;
}

static void PngFlush(png_struct* png)
{

}

int EncodeBmpToPng(int width, int height, int bit, void* pallete, void* dib, MemStream* stream)
{
	int colorType;
	if (bit == 8)
		colorType = PNG_COLOR_TYPE_PALETTE;
	else if (bit == 24)
		colorType = PNG_COLOR_TYPE_RGB;
	else if (bit == 32)
		colorType = PNG_COLOR_TYPE_RGBA;
	else
	{
		return -1;
	}

	auto png = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!png)
		return -1;
	auto pngInfo = png_create_info_struct(png);
	if (!pngInfo)
	{
		png_destroy_write_struct(&png, 0);
		return -1;
	}

	if (setjmp(png_jmpbuf(png)))
	{
		png_destroy_write_struct(&png, &pngInfo);
		return -1;
	}

	png_set_write_fn(png, stream, PngWrite, PngFlush);

	int realHeight = height > 0 ? height : -height;

	png_set_IHDR(png, pngInfo, width, realHeight, 8, colorType,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_color* newPlte;
	if (bit == 8)
	{
		newPlte = new png_color[256];
		auto p1 = newPlte;
		auto p2 = (BYTE*)pallete;
		for (int i = 0; i < 256; i++)
		{
			p1->red = p2[2];
			p1->green = p2[1];
			p1->blue = p2[0];
			p1++;
			p2 += 4;
		}

		png_set_PLTE(png, pngInfo, newPlte, 256);
	}

	png_write_info(png, pngInfo);

	if (bit != 8)
		png_set_bgr(png);

	auto rowPointers = new png_byte*[realHeight];
	int bytesPerRow = (bit / 8 * width + 3)&~3;

	if (height < 0)
	{
		auto p = (png_byte*)dib;
		for (int i = 0; i < realHeight; i++)
		{
			rowPointers[i] = p;
			p += bytesPerRow;
		}
	}
	else
	{
		auto p = (png_byte*)dib + (realHeight - 1)*bytesPerRow;
		for (int i = 0; i < realHeight; i++)
		{
			rowPointers[i] = p;
			p -= bytesPerRow;
		}
	}

	png_write_image(png, rowPointers);
	png_write_end(png, pngInfo);

	if (bit == 8)
		delete[] newPlte;

	delete[] rowPointers;

	png_destroy_write_struct(&png, &pngInfo);

	return 0;
}

/* never forget that globals are a one-way ticket to Hell */
/* Ogg and codec state for demux/decode */
ogg_sync_state    oy;
ogg_page          og;
ogg_stream_state  vo;
ogg_stream_state  to;
th_info           ti;
th_comment        tc;
th_setup_info    *ts;
th_dec_ctx       *td;

int              theora_p = 0;
int              theora_processing_headers;
int              stateflag = 0;

/* single frame video buffering */
int          videobuf_ready = 0;
ogg_int64_t  videobuf_granulepos = -1;
double       videobuf_time = 0;


int got_sigint = 0;

static th_ycbcr_buffer ycbcr;

static void stripe_decoded(th_ycbcr_buffer _dst, th_ycbcr_buffer _src,
	int _fragy0, int _fragy_end){
	int pli;
	for (pli = 0; pli<3; pli++){
		int yshift;
		int y_end;
		int y;
		yshift = pli != 0 && !(ti.pixel_fmt & 2);
		y_end = _fragy_end << 3 - yshift;
		/*An implemention intending to display this data would need to check the
		crop rectangle before proceeding.*/
		for (y = _fragy0 << 3 - yshift; y<y_end; y++){
			memcpy(_dst[pli].data + y*_dst[pli].stride,
				_src[pli].data + y*_src[pli].stride, _src[pli].width);
		}
	}
}


static void open_video(void){
	th_stripe_callback cb;
	int                pli;
	/*Here we allocate a buffer so we can use the striped decode feature.
	There's no real reason to do this in this application, because we want to
	write to the file top-down, but the frame gets decoded bottom up, so we
	have to buffer it all anyway.
	But this illustrates how the API works.*/
	for (pli = 0; pli<3; pli++){
		int xshift;
		int yshift;
		xshift = pli != 0 && !(ti.pixel_fmt & 1);
		yshift = pli != 0 && !(ti.pixel_fmt & 2);
		ycbcr[pli].data = (unsigned char *)malloc(
			(ti.frame_width >> xshift)*(ti.frame_height >> yshift)*sizeof(char));
		ycbcr[pli].stride = ti.frame_width >> xshift;
		ycbcr[pli].width = ti.frame_width >> xshift;
		ycbcr[pli].height = ti.frame_height >> yshift;
	}
	/*Similarly, since ycbcr is a global, there's no real reason to pass it as
	the context.
	In a more object-oriented decoder, we could pass the "this" pointer
	instead (though in C++, platform-dependent calling convention differences
	prevent us from using a real member function pointer).*/
	cb.ctx = ycbcr;
	cb.stripe_decoded = (th_stripe_decoded_func)stripe_decoded;
	th_decode_ctl(td, TH_DECCTL_SET_STRIPE_CB, &cb, sizeof(cb));
}

#include <stdint.h>

void fix_bmp(uint32_t width, uint32_t height, uint8_t*data)
{
	int widthlen = width * 3;

	uint8_t* tmp_data = new uint8_t[widthlen*height];
	//widthlen * nHeight
	for (uint32_t m_height = 0; m_height<height; m_height++)
	{
		uint32_t destbuf = (uint32_t)tmp_data + (((height - m_height) - 1)*widthlen);
		uint32_t srcbuf = (uint32_t)data + widthlen * m_height;
		memcpy((void*)destbuf, (void*)srcbuf, widthlen);
	}
	memcpy(data, tmp_data, widthlen*height);
	delete tmp_data;
}

void fix_bmp32(uint32_t width, uint32_t height, uint8_t*data)
{
	int widthlen = width * 4;

	uint8_t* tmp_data = new uint8_t[widthlen*height];
	//widthlen * nHeight
	for (uint32_t m_height = 0; m_height<height; m_height++)
	{
		uint32_t destbuf = (uint32_t)tmp_data + (((height - m_height) - 1)*widthlen);
		uint32_t srcbuf = (uint32_t)data + widthlen * m_height;
		memcpy((void*)destbuf, (void*)srcbuf, widthlen);
	}
	memcpy(data, tmp_data, widthlen*height);
	delete tmp_data;
}

/*Write out the planar YUV frame, uncropped.*/
static void video_write(FILE* file){
	int pli;
	int i;
	int Size = 0;

	BYTE              plte[0x400];
	auto p = (BYTE*)plte;
	for (int i = 0; i < 0x100; i++)
	{
		p[0] = p[1] = p[2] = i;
		p[3] = 0;
		p += 4;
	}
	/*Uncomment the following to do normal, non-striped decoding.
	th_ycbcr_buffer ycbcr;
	th_decode_ycbcr_out(td,ycbcr);*/
	if (file){
		for (pli = 0; pli<3; pli++){
			for (i = 0; i<ycbcr[pli].height; i++){
				Size += ycbcr[pli].width;
			}
		}

		const U8* frame_data[3] = { ycbcr[0].data, ycbcr[1].data, ycbcr[2].data };
		const int frame_size[3] = { ycbcr[0].width, ycbcr[1].width, ycbcr[2].width };

		U8* dst = (U8*)malloc(ycbcr[0].width * ycbcr[0].height * 3);

		YUV444_BGR_F(dst, ycbcr[0].width * 3, frame_data, frame_size, ycbcr[0].width, ycbcr[0].height);
		fix_bmp32(ycbcr[0].width, ycbcr[0].height, dst);

		MemStream         Stream;
		Stream.start = Stream.cur = new BYTE[Size * 2];
		Stream.len = Size * 2;

		EncodeBmpToPng(ycbcr[0].width, ycbcr[0].height, 24, plte, (void*)dst, &Stream);
		Size = Stream.cur - Stream.start;
		fwrite(Stream.start, 1, Size, file);
		delete[] Stream.start;
		free(dst);
	}
}


static void video_write_fixed(FILE* file){
	int pli;
	int i;
	int Size = 0;

	BYTE              plte[0x400];
	auto p = (BYTE*)plte;
	for (int i = 0; i < 0x100; i++)
	{
		p[0] = p[1] = p[2] = i;
		p[3] = 0;
		p += 4;
	}
	/*Uncomment the following to do normal, non-striped decoding.
	th_ycbcr_buffer ycbcr;
	th_decode_ycbcr_out(td,ycbcr);*/
	if (file){
		for (pli = 0; pli<3; pli++){
			for (i = 0; i<ycbcr[pli].height; i++){
				Size += ycbcr[pli].width;
			}
		}

		const U8* frame_data[3] = { ycbcr[0].data, ycbcr[1].data, ycbcr[2].data };
		const int frame_size[3] = { ycbcr[0].width, ycbcr[1].width, ycbcr[2].width };

		int vidheight = ycbcr[0].height;
		int height = vidheight * 3 / 4;
		int vidwidth = ycbcr[0].width;

		U8* dst = (U8*)malloc(vidheight * vidwidth * 3);
		memcpy(dst, ycbcr[0].data, vidheight * vidwidth);
		memcpy(dst + vidheight * vidwidth, ycbcr[1].data, vidheight * vidwidth);
		memcpy(dst + vidheight * vidwidth * 2, ycbcr[2].data, vidheight * vidwidth);

		U8* buf = new byte[ycbcr[0].width * 4 * height];

		int Stride = ycbcr[0].width * 4;
		int offset = Stride < 0 ? -Stride * (ycbcr[0].height - 1) : 0;

		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < ycbcr[0].width; x++)
			{
				buf[offset + Stride * y + 4 * x + 0] = dst[vidwidth * (vidheight * 0 + y) + x];
				buf[offset + Stride * y + 4 * x + 1] = dst[vidwidth * (vidheight * 1 + y) + x];
				buf[offset + Stride * y + 4 * x + 2] = dst[vidwidth * (vidheight * 2 + y) + x];
				if (y < (height + 2) / 3)
				{
					buf[offset + Stride * y + 4 * x + 3] = dst[vidwidth * (height * 1 + y) + x];
				}
				else if (y < (height + 2) / 3 * 2)
				{
					buf[offset + Stride * y + 4 * x + 3] = dst[vidwidth * (height * 2 + y) + x];
				}
				else
				{
					buf[offset + Stride * y + 4 * x + 3] = dst[vidwidth * (height * 3 + y) + x];
				}
			}
		}

		fix_bmp32(ycbcr[0].width, height, buf);

		MemStream         Stream;
		Stream.start = Stream.cur = new BYTE[Size * 2];
		Stream.len = Size * 2;

		EncodeBmpToPng(ycbcr[0].width, height, 32, plte, (void*)buf, &Stream);
		Size = Stream.cur - Stream.start;
		fwrite(Stream.start, 1, Size, file);
		delete[] Stream.start;
		delete[] buf;
		free(dst);
	}
}

/* helper: push a page into the appropriate steam */
/* this can be done blindly; a stream won't accept a page
that doesn't belong to it */
static int queue_page(ogg_page *page){
	if (theora_p)ogg_stream_pagein(&to, page);
	return 0;
}


int main(int argc, char *argv[]){

	ogg_packet op;

	int long_option_index;
	int c;

	struct timeb start;
	struct timeb after;
	struct timeb last;
	int fps_only = 0;
	int frames = 0;

	FILE *infile = fopen("ed.ogv", "rb");

	/* start up Ogg stream synchronization layer */
	ogg_sync_init(&oy);

	/* init supporting Theora structures needed in header parsing */
	th_comment_init(&tc);
	th_info_init(&ti);

	/*Ogg file open; parse the headers.
	Theora (like Vorbis) depends on some initial header packets for decoder
	setup and initialization.
	We retrieve these first before entering the main decode loop.*/

	/* Only interested in Theora streams */
	while (!stateflag)
	{
		int ret = buffer_data(infile, &oy);
		if (ret == 0)break;
		while (ogg_sync_pageout(&oy, &og)>0){
			int got_packet;
			ogg_stream_state test;

			/* is this a mandated initial header? If not, stop parsing */
			if (!ogg_page_bos(&og)){
				/* don't leak the page; get it into the appropriate stream */
				queue_page(&og);
				stateflag = 1;
				break;
			}

			ogg_stream_init(&test, ogg_page_serialno(&og));
			ogg_stream_pagein(&test, &og);
			got_packet = ogg_stream_packetpeek(&test, &op);

			/* identify the codec: try theora */
			if ((got_packet == 1) && !theora_p && (theora_processing_headers =
				th_decode_headerin(&ti, &tc, &ts, &op)) >= 0){
				/* it is theora -- save this stream state */
				memcpy(&to, &test, sizeof(test));
				theora_p = 1;
				/*Advance past the successfully processed header.*/
				if (theora_processing_headers)ogg_stream_packetout(&to, NULL);
			}
			else{
				/* whatever it is, we don't care about it */
				ogg_stream_clear(&test);
			}
		}
		/* fall through to non-bos page parsing */
	}

	/* we're expecting more header packets. */
	while (theora_p && theora_processing_headers){
		int ret;

		/* look for further theora headers */
		while (theora_processing_headers && (ret = ogg_stream_packetpeek(&to, &op))){
			if (ret<0)continue;
			theora_processing_headers = th_decode_headerin(&ti, &tc, &ts, &op);
			if (theora_processing_headers<0){
				fprintf(stderr, "Error parsing Theora stream headers; "
					"corrupt stream?\n");
				exit(1);
			}
			else if (theora_processing_headers>0){
				/*Advance past the successfully processed header.*/
				ogg_stream_packetout(&to, NULL);
			}
			theora_p++;
		}

		/*Stop now so we don't fail if there aren't enough pages in a short
		stream.*/
		if (!(theora_p && theora_processing_headers))break;

		/* The header pages/packets will arrive before anything else we
		care about, or the stream is not obeying spec */

		if (ogg_sync_pageout(&oy, &og)>0){
			queue_page(&og); /* demux into the appropriate stream */
		}
		else{
			int ret = buffer_data(infile, &oy); /* someone needs more data */
			if (ret == 0){
				exit(1);
			}
		}
	}

	/* and now we have it all.  initialize decoders */
	if (theora_p)
	{
		td = th_decode_alloc(&ti, ts);
	}
	else
	{
		/* tear down the partial theora setup */
		th_info_clear(&ti);
		th_comment_clear(&tc);
	}
	/*Either way, we're done with the codec setup data.*/
	th_setup_free(ts);

	/* open video */
	if (theora_p)open_video();



	/*Finally the main decode loop.

	It's one Theora packet per frame, so this is pretty straightforward if
	we're not trying to maintain sync with other multiplexed streams.

	The videobuf_ready flag is used to maintain the input buffer in the libogg
	stream state.
	If there's no output frame available at the end of the decode step, we must
	need more input data.
	We could simplify this by just using the return code on
	ogg_page_packetout(), but the flag system extends easily to the case where
	you care about more than one multiplexed stream (like with audio
	playback).
	In that case, just maintain a flag for each decoder you care about, and
	pull data when any one of them stalls.

	videobuf_time holds the presentation time of the currently buffered video
	frame.
	We ignore this value.*/

	stateflag = 0; /* playback has not begun */
	/* queue any remaining pages from data we buffered but that did not
	contain headers */
	while (ogg_sync_pageout(&oy, &og)>0){
		queue_page(&og);
	}

	if (fps_only){
		ftime(&start);
		ftime(&last);
	}

	while (!got_sigint){

		while (theora_p && !videobuf_ready){
			/* theora is one in, one out... */
			if (ogg_stream_packetout(&to, &op)>0){

				if (th_decode_packetin(td, &op, &videobuf_granulepos) >= 0){
					videobuf_time = th_granule_time(td, videobuf_granulepos);
					videobuf_ready = 1;
					frames++;
					if (fps_only)
						ftime(&after);
				}

			}
			else
				break;
		}

		if (fps_only && (videobuf_ready || fps_only == 2)){
			long ms =
				after.time*1000. + after.millitm -
				(last.time*1000. + last.millitm);

			if (ms>500 || fps_only == 1 ||
				(feof(infile) && !videobuf_ready)){
				float file_fps = (float)ti.fps_numerator / ti.fps_denominator;
				fps_only = 2;

				ms = after.time*1000. + after.millitm -
					(start.time*1000. + start.millitm);

				fprintf(stderr, "\rframe:%d rate:%.2fx           ",
					frames,
					frames*1000. / (ms*file_fps));
				memcpy(&last, &after, sizeof(last));
			}
		}

		if (!videobuf_ready && feof(infile))break;

		if (!videobuf_ready){
			/* no data yet for somebody.  Grab another page */
			buffer_data(infile, &oy);
			while (ogg_sync_pageout(&oy, &og)>0){
				queue_page(&og);
			}
		}
		/* dumpvideo frame, and get new one */
		else
		{
			char FileName[_MAX_PATH];
			sprintf(FileName, "mov/%06d.png", frames);
			FILE* file = fopen(FileName, "wb");
			video_write_fixed(file);
			fclose(file);
		}

		videobuf_ready = 0;
	}

	/* end of decoder loop -- close everything */

	if (theora_p){
		ogg_stream_clear(&to);
		th_decode_free(td);
		th_comment_clear(&tc);
		th_info_clear(&ti);
	}
	ogg_sync_clear(&oy);

	fclose(infile);

	fprintf(stderr, "\n\n%d frames\n", frames);
	fprintf(stderr, "\nDone.\n");

	return(0);

}
