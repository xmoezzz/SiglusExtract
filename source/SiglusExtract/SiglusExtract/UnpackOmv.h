#pragma once

#include "my.h"
#include "iUnpackObject.h"
#include "DecodeControl.h"
#include "Tool.h"
#include <ShlObj.h>
#include <string>
#include <math.h>
#include "theora/theoradec.h"
#include "ogg/ogg.h"
#include "ogg/os_types.h"
#include "png.h"
#include "MemStream.h"
#include <stdint.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <vector>

//#include "Color1.h"
#include "Color2.h"
#include "Color3.h"

#pragma comment(lib, "..\\Release\\libtheora_static.lib")

typedef struct OMVHeader
{
	DWORD DataOffset; //ogv(1.0) or package index(1.1)
	BYTE  MainVersion;
	BYTE  SubVersion;
	WORD  Unknown1;
	DWORD UnkArray[8];
	DWORD EncryptType; //1--YUV三分之一混合 2--原始
	DWORD Width;
	DWORD Height;
	DWORD UnkArray2[2];
	DWORD FrameTime;
	ULARGE_INTEGER StreamID;
	BOOL  Flag2;
	DWORD DataPackageCount;
	DWORD FrameCount;

	//...
}OMVHeader, *POMVHeader;


typedef struct DataPackage
{
	DWORD PackageId;
	DWORD PackageType;
	DWORD PackageSize;
	DWORD PackageOffset;
	DWORD Unk;
	DWORD FrameCount;
	DWORD IndexOfFirstFrame;
}DataPackage, *PDataPackage;


typedef struct FramePackage
{
	DWORD FrameId;
	DWORD PackageId;
	DWORD IndexOfThisPackage;
	DWORD FrameTime;
	DWORD Unk[2];
	DWORD FrameStartTime;
	DWORD FrameEndTime;
}FramePackage, *PFramePackage;


class UnpackOMV : public iUnpackObject
{
	std::wstring m_FileName;

public:
	UnpackOMV()
	{
		theora_p = 0;
		stateflag = 0;
		videobuf_ready = 0;
		videobuf_granulepos = -1;
		videobuf_time = 0;
		got_sigint = 0;
	};

	~UnpackOMV(){};

	Void FASTCALL SetFile(LPCWSTR FileName)
	{
		m_FileName = FileName;
	}

	NTSTATUS FASTCALL Unpack(PVOID UserData)
	{
		NTSTATUS          Status;
		NtFileDisk        File, Writer;
		PDecodeControl    Code;
		PBYTE             Buffer;
		ULONG_PTR         Size, Attribute;
		std::wstring      FileName, FullPath, FullOutDirectory;
		WCHAR             ExeDirectory[MAX_PATH];
		BYTE              HeaderBuffer[0xA8];
		POMVHeader        OmvHeader;
		ULONG             Offset;
		

		Code = (PDecodeControl)UserData;
		Status = File.Open(m_FileName.c_str());
		if (NT_FAILED(Status))
			return Status;

		RtlZeroMemory(ExeDirectory, sizeof(ExeDirectory));
		Nt_GetExeDirectory(ExeDirectory, countof(ExeDirectory));

		static WCHAR OutDirectory[] = L"__Unpack__\\OMV\\";

		FullOutDirectory = ExeDirectory + std::wstring(OutDirectory);
		Attribute = Nt_GetFileAttributes(FullOutDirectory.c_str());
		if (Attribute == 0xffffffff)
			SHCreateDirectory(NULL, FullOutDirectory.c_str());

		FileName = GetFileName(m_FileName);
		FullPath = FullOutDirectory + FileName;

		File.Read(HeaderBuffer, sizeof(HeaderBuffer));
		OmvHeader = (POMVHeader)HeaderBuffer;
		
		if (OmvHeader->MainVersion != 1)
		{
			File.Close();
			return STATUS_UNSUCCESSFUL;
		}

		if (OmvHeader->SubVersion == 1)
			Offset = OmvHeader->DataOffset + OmvHeader->DataPackageCount * sizeof(DataPackage) + OmvHeader->FrameCount * sizeof(FramePackage);
		else if (OmvHeader->SubVersion == 0)
			Offset = OmvHeader->DataOffset;

		File.Seek(Offset, FILE_BEGIN);

		switch (OmvHeader->EncryptType)
		{
		case 1:
			Status = ExtractOmvType1(File, OmvHeader, Code, FullPath);
			break;

		case 2:
			Status = ExtractOmvType2(File, OmvHeader, Code, FullPath);
			break;

		default:
			Status = ExtractOmvTypeUnk(File, OmvHeader, Code, FullPath);
			break;
		}

		File.Close();
		return Status;
	}


private:

	NTSTATUS ExtractOmvTypeUnk(NtFileDisk& File, POMVHeader Header, PDecodeControl Code, std::wstring& FileName)
	{
		NTSTATUS   Status;
		NtFileDisk Writer;
		DWORD      OggsTag;
		BYTE       Buffer[0x1000];
		ULONG      Size, ReadSize, NeedRead;
		LARGE_INTEGER TranslatedSize;
		
		File.Read(&OggsTag, sizeof(OggsTag));
		if (OggsTag != TAG4('OggS'))
			return STATUS_INVALID_SIGNATURE;

		File.Seek(-4, FILE_CURRENT);
		
		Status = Writer.Create(FileName.c_str());
		if (NT_FAILED(Status))
			return Status;

		Size = File.GetSize32() - File.GetCurrentPos();
		ReadSize = 0;
		
		while (ReadSize < Size)
		{
			if (Size - ReadSize >= sizeof(Buffer))
				NeedRead = sizeof(Buffer);
			else
				NeedRead = Size - ReadSize;

			File.Read(Buffer, NeedRead, &TranslatedSize);
			ReadSize += TranslatedSize.LowPart;
			Writer.Write(Buffer, TranslatedSize.LowPart);
		}
		Writer.Close();
		return STATUS_SUCCESS;
	}

	NTSTATUS ExtractOmvType1(NtFileDisk& File, POMVHeader Header, PDecodeControl Code, std::wstring& FileName)
	{
		std::wstring FileDir;

		switch (Code->OGVFlag)
		{
		case OGV_DECODE:
		default:
			FileName = ReplaceFileNameExtension(FileName, L".ogv");
			return ExtractOmvTypeUnk(File, Header, Code, FileName);


		case OGV_PNG:
			FileDir = GetFileNamePrefix(FileName);
			SHCreateDirectory(NULL, FileDir.c_str());
			OmvToPng_Fixed(File, FileDir);
			break;
		}
		return STATUS_SUCCESS;
	}


	NTSTATUS ExtractOmvType2(NtFileDisk& File, POMVHeader Header, PDecodeControl Code, std::wstring& FileName)
	{
		std::wstring FileDir;

		switch (Code->OGVFlag)
		{
		case OGV_DECODE:
		default:
			FileName = ReplaceFileNameExtension(FileName, L".ogv");
			return ExtractOmvTypeUnk(File, Header, Code, FileName);

		case OGV_PNG:
			FileDir = GetFileNamePrefix(FileName);
			SHCreateDirectory(NULL, FileDir.c_str());
			OmvToPng_Normal(File, FileDir);
			break;
		}
		return STATUS_SUCCESS;
	}


	/*******************************/
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

	static void stripe_decoded(th_ycbcr_buffer _dst, th_ycbcr_buffer _src,
		int _fragy0, int _fragy_end)
	{
		int pli;
		for (pli = 0; pli<3; pli++){
			int yshift;
			int y_end;
			int y;
			yshift = pli != 0 && !(ti.pixel_fmt & 2);
			y_end = _fragy_end << 3 - yshift;
			/*An implemention intending to display this data would need to check the
			crop rectangle before proceeding.*/
			for (y = _fragy0 << 3 - yshift; y<y_end; y++)
			{
				memcpy(_dst[pli].data + y*_dst[pli].stride,
					_src[pli].data + y*_src[pli].stride, _src[pli].width);
			}
		}
	}

	void open_video(void){
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


	void video_write(FILE* file){
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

			U8* dst = NULL;
			if (ti.pixel_fmt == TH_PF_444)
			{
				const U8* frame_data[3] = { ycbcr[0].data, ycbcr[1].data, ycbcr[2].data };
				const int frame_size[3] = { ycbcr[0].width, ycbcr[0].width, ycbcr[0].width };

				dst = (U8*)malloc(ycbcr[0].width * ycbcr[0].height * 3);

				YUV444_BGR_F(dst, ycbcr[0].width * 3, frame_data, frame_size, ycbcr[0].width, ycbcr[0].height);
			}
			else if (ti.pixel_fmt == TH_PF_420)
			{
				std::vector<BYTE> src;
				for (pli = 0; pli<3; pli++){
					int FrameSize = ycbcr[pli].height * ycbcr[pli].width;
					std::vector<BYTE> frame(ycbcr[pli].data, ycbcr[pli].data + FrameSize);

					src.insert(src.end(), frame.begin(), frame.end());
				}

				int w = ycbcr[0].width;
				int h = ycbcr[0].height;
				U8* src2 = CreateYUV420Frame(&src[0], w, h);
				dst = (U8*)malloc(ycbcr[0].width * ycbcr[0].height * 3);
				const U8* frame_data[3] = { src2, src2 + w*h + w / 2 + 3, src2 + w*h + (w / 2 + 2)*(h / 2 + 2) + w / 2 + 3 };
				const int frame_size[3] = { w, w / 2 + 2, w / 2 + 2 };
				YUV420_BGR_F(dst, w * 3, frame_data, frame_size, w, h);
				free(src2);
			}
			else
			{
				return;
			}
			fix_bmp(ycbcr[0].width, ycbcr[0].height, dst);

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

	int queue_page(ogg_page *page){
		if (theora_p)ogg_stream_pagein(&to, page);
		return 0;
	}

	int buffer_data(NtFileDisk& in, ogg_sync_state *oy){
		char *buffer = ogg_sync_buffer(oy, 4096);
		LARGE_INTEGER bytes;
		in.Read(buffer, 4096, &bytes);
		ogg_sync_wrote(oy, bytes.LowPart);
		return(bytes.LowPart);
	}

	NTSTATUS OmvToPng_Normal(NtFileDisk& infile, std::wstring& FilePath)
	{
		ogg_packet op;

		int long_option_index;
		int c;

		struct timeb start;
		struct timeb after;
		struct timeb last;
		int fps_only = 0;
		int frames = 0;


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
					return STATUS_UNSUCCESSFUL;
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
					((infile.GetCurrentPos() == infile.GetSize32()) && !videobuf_ready)){
					float file_fps = (float)ti.fps_numerator / ti.fps_denominator;
					fps_only = 2;

					ms = after.time*1000. + after.millitm -
						(start.time*1000. + start.millitm);

					memcpy(&last, &after, sizeof(last));
				}
			}

			if (!videobuf_ready && (infile.GetCurrentPos() == infile.GetSize32()))break;

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
				WCHAR SubName[MAX_PATH];
				FormatStringW(SubName, L"\\%06d.png", frames);

				std::wstring WriteFileName = FilePath + SubName;
				FILE* file = _wfopen(WriteFileName.c_str(), L"wb");
				video_write(file);
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

		return(0);
	}

	void video_write_fixed(FILE* file){
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


	NTSTATUS OmvToPng_Fixed(NtFileDisk& infile, std::wstring& FilePath)
	{
		ogg_packet op;

		int long_option_index;
		int c;

		struct timeb start;
		struct timeb after;
		struct timeb last;
		int fps_only = 0;
		int frames = 0;


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
					return STATUS_UNSUCCESSFUL;
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
					((infile.GetCurrentPos() == infile.GetSize32()) && !videobuf_ready)){
					float file_fps = (float)ti.fps_numerator / ti.fps_denominator;
					fps_only = 2;

					ms = after.time*1000. + after.millitm -
						(start.time*1000. + start.millitm);

					memcpy(&last, &after, sizeof(last));
				}
			}

			if (!videobuf_ready && (infile.GetCurrentPos() == infile.GetSize32()))break;

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
				WCHAR SubName[MAX_PATH];
				FormatStringW(SubName, L"\\%06d.png", frames);

				std::wstring WriteFileName = FilePath + SubName;
				FILE* file = _wfopen(WriteFileName.c_str(), L"wb");
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

		return(0);
	}

	ogg_sync_state    oy;
	ogg_page          og;
	ogg_stream_state  vo;
	ogg_stream_state  to;
	static th_info    ti;
	th_comment        tc;
	th_setup_info    *ts;
	th_dec_ctx       *td;

	int              theora_p;
	int              theora_processing_headers;
	int              stateflag;
	int              videobuf_ready;
	ogg_int64_t      videobuf_granulepos;
	double           videobuf_time;
	int              got_sigint;
	th_ycbcr_buffer  ycbcr;

};