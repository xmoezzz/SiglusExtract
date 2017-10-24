#include "my.h"
#include "theora/theora.h"
#include "theora/theoradec.h"
#include "ogg/ogg.h"
#include "ogg/os_types.h"
#include <stdint.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <string>
#include <ShlObj.h>
#include "Color2.h"
#include "Color3.h"
#include "pnglibconf.h"
#include "png.h"
#include "pngconf.h"
#include <vector>
#include <Vfw.h>

#pragma comment(lib, "libogg_static.lib")
#pragma comment(lib, "libtheora_static.lib")
#pragma comment(lib, "Vfw32.lib")

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


struct MemStream
{
	BYTE* start;
	BYTE* cur;
	DWORD len;
};


ForceInline std::wstring FASTCALL GetFileName(std::wstring& Path)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L"\\");
	if (Ptr == std::wstring::npos)
		return Path;

	return Path.substr(Ptr + 1, std::wstring::npos);
}


ForceInline std::wstring FASTCALL GetFileNameExtension(std::wstring& Path)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L".");
	if (Ptr == std::wstring::npos)
		return NULL;

	return Path.substr(Ptr + 1, std::wstring::npos);
}


ForceInline std::wstring FASTCALL GetFileNamePrefix(std::wstring& Path)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L".");
	if (Ptr == std::wstring::npos)
		return Path;

	return Path.substr(0, Ptr);
}

ForceInline std::wstring FASTCALL ReplaceFileNameExtension(std::wstring& Path, PCWSTR NewExtensionName)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L".");
	if (Ptr == std::wstring::npos)
		return Path + NewExtensionName;

	return Path.substr(0, Ptr) + NewExtensionName;
}


class UnpackOMV
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

	NTSTATUS FASTCALL Unpack()
	{
		NTSTATUS          Status;
		NtFileDisk        File, Writer;
		PBYTE             Buffer;
		ULONG_PTR         Size, Attribute;
		std::wstring      FileName, FullPath;
		BYTE              HeaderBuffer[0xA8];
		POMVHeader        OmvHeader;
		ULONG             Offset;
		DWORD             OggsTag;

		Status = File.Open(m_FileName.c_str());
		if (NT_FAILED(Status))
			return Status;

		FileName = GetFileName(m_FileName);
		FullPath = FileName;

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

		File.Read(&OggsTag, sizeof(OggsTag));
		if (OggsTag != TAG4('OggS'))
		{
			PrintConsole(L"Not a omv stream. [%p]\n", Offset);
			return STATUS_INVALID_SIGNATURE;
		}

		File.Seek(-4, FILE_CURRENT);

		PrintConsole(L"Converting...\n");
		switch (OmvHeader->EncryptType)
		{
		case 1:
			PrintConsole(L"Convert to 32bit avi file\n");
			Status = ExtractOmvType1(File, OmvHeader, FullPath);
			break;

		case 2:
			PrintConsole(L"Valid theora video\n");
			Status = ExtractOmvType2(File, OmvHeader, FullPath);
			break;
		}

		File.Close();
		return Status;
	}


private:

	IAVIFile* m_aviFile;
	IAVIStream* m_aviVideoStream;

	ULONG   m_Width;
	ULONG   m_Height;

	NTSTATUS ExtractOmvType1(NtFileDisk& File, POMVHeader Header, std::wstring& FileName)
	{
		FileName = ReplaceFileNameExtension(FileName, L".avi");

		m_aviFile = NULL;
		AVIFileOpenW(&m_aviFile, FileName.c_str(), OF_CREATE | OF_WRITE, 0);

		AVISTREAMINFOW m_aviVideoStreamInfo = {0};
		m_aviVideoStreamInfo.fccType = streamtypeVIDEO;
		m_aviVideoStreamInfo.fccHandler = 0;
		m_aviVideoStreamInfo.dwFlags = 0;
		m_aviVideoStreamInfo.dwCaps = 0;

		m_aviVideoStreamInfo.wPriority = 0;
		m_aviVideoStreamInfo.wLanguage = 0;

//#error ""
		m_aviVideoStreamInfo.dwRate = 2997; //每秒5帧

		m_aviVideoStreamInfo.dwScale = 100;
		m_aviVideoStreamInfo.dwQuality = 0;
		m_aviVideoStreamInfo.dwSuggestedBufferSize = Header->Width * Header->Height * 4;

		m_aviVideoStreamInfo.dwStart = 0;

		m_aviVideoStreamInfo.dwInitialFrames = 0;

		m_aviVideoStreamInfo.dwSampleSize = 0;

		m_aviVideoStreamInfo.rcFrame.left = 0;

		m_aviVideoStreamInfo.rcFrame.top = 0;

		m_aviVideoStreamInfo.rcFrame.right = Header->Width;

		m_aviVideoStreamInfo.rcFrame.bottom = Header->Height;

		m_aviVideoStreamInfo.dwEditCount = 0;

		m_aviVideoStreamInfo.dwFormatChangeCount = 0;

		StrCopyW(m_aviVideoStreamInfo.szName, L"file.avi");
		m_aviVideoStreamInfo.dwLength = 0;

		m_aviVideoStream = NULL;
		AVIFileCreateStreamW(m_aviFile, &m_aviVideoStream, (AVISTREAMINFOW *)&m_aviVideoStreamInfo);

		m_Width = Header->Width;
		m_Height = Header->Height;

		OmvToPng_Fixed(File);

		if (m_aviVideoStream)
		{
			m_aviVideoStream->Release();
			m_aviVideoStream = NULL;
		}

		if (m_aviFile)
		{
			m_aviFile->Release();
			m_aviFile = NULL;
		}

		return STATUS_SUCCESS;
	}

	NTSTATUS ExtractOmvType2(NtFileDisk& File, POMVHeader Header, std::wstring& FileName)
	{
		FileName = ReplaceFileNameExtension(FileName, L".ogv");
		return ExtractOmvTypeUnk(File, Header, FileName);
		return STATUS_SUCCESS;
	}

	NTSTATUS ExtractOmvTypeUnk(NtFileDisk& File, POMVHeader Header, std::wstring& FileName)
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



	void video_write_fixed(ULONG Frame){
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
		if (true){
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

			BITMAPINFOHEADER bmpInfoHdr;
			bmpInfoHdr.biSize = sizeof(BITMAPINFOHEADER);
			bmpInfoHdr.biWidth = ycbcr[0].width;
			bmpInfoHdr.biHeight = height;
			bmpInfoHdr.biBitCount = 32;
			bmpInfoHdr.biCompression = BI_RGB;
			bmpInfoHdr.biSizeImage = 0;
			bmpInfoHdr.biClrImportant = 0;
			bmpInfoHdr.biClrUsed = 0;
			bmpInfoHdr.biXPelsPerMeter = 0;
			bmpInfoHdr.biYPelsPerMeter = 0;
			bmpInfoHdr.biPlanes = 1;

			AVIStreamSetFormat(m_aviVideoStream, Frame, &bmpInfoHdr, sizeof(bmpInfoHdr));
			AVIStreamWrite(m_aviVideoStream, Frame, 1, (LPBYTE)buf, ycbcr[0].width * height * 4, AVIIF_KEYFRAME, NULL, NULL);

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


	NTSTATUS OmvToPng_Fixed(NtFileDisk& infile)
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
					PrintConsole(L"Invalid package %d\n", theora_processing_headers);
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
				PrintConsole(L"Converting frame %d\n", frames);
				video_write_fixed(frames);
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

th_info UnpackOMV::ti;

/*
int wmain(int argc, WCHAR* argv[])
{
	ml::MlInitialize();
	AVIFileInit();
	if (argc != 2)
	{
		PrintConsoleW(L"Usage : %s [input.omv]\n", argv[0]);
		return 0;
	}
	UnpackOMV Unpacker;

	//Unpacker.SetFile(argv[1]);
	Unpacker.SetFile(L"test.omv");
	Unpacker.Unpack();

	AVIFileExit();
	return 0;
}

*/



/* Helper; just grab some more compressed bitstream and sync it for
page extraction */
int buffer_data(FILE *in, ogg_sync_state *oy){
	char *buffer = ogg_sync_buffer(oy, 4096);
	int bytes = fread(buffer, 1, 4096, in);
	ogg_sync_wrote(oy, bytes);
	return(bytes);
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
int          raw = 0;


int got_sigint = 0;
static void sigint_handler(int signal) {
	got_sigint = 1;
}

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

IAVIFile* m_aviFile = NULL;
IAVIStream* m_aviVideoStream = NULL;



/*Write out the planar YUV frame, uncropped.*/
void video_write(ULONG Frame){
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
	if (true){
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

		BITMAPINFOHEADER bmpInfoHdr;
		bmpInfoHdr.biSize = sizeof(BITMAPINFOHEADER);
		bmpInfoHdr.biWidth = ycbcr[0].width;
		bmpInfoHdr.biHeight = height;
		bmpInfoHdr.biBitCount = 32;
		bmpInfoHdr.biCompression = BI_RGB;
		bmpInfoHdr.biSizeImage = 0;
		bmpInfoHdr.biClrImportant = 0;
		bmpInfoHdr.biClrUsed = 0;
		bmpInfoHdr.biXPelsPerMeter = 0;
		bmpInfoHdr.biYPelsPerMeter = 0;
		bmpInfoHdr.biPlanes = 1;

		AVIStreamSetFormat(m_aviVideoStream, Frame, &bmpInfoHdr, sizeof(bmpInfoHdr));
		AVIStreamWrite(m_aviVideoStream, Frame, 1, (LPBYTE)buf, ycbcr[0].width * height * 4, AVIIF_KEYFRAME, NULL, NULL);

		delete[] buf;
		free(dst);
	}
}



/* dump the theora comment header */
static int dump_comments(th_comment *_tc){
	return 0;
}

/* helper: push a page into the appropriate steam */
/* this can be done blindly; a stream won't accept a page
that doesn't belong to it */
static int queue_page(ogg_page *page){
	if (theora_p)ogg_stream_pagein(&to, page);
	return 0;
}

NTSTATUS ExtractOmvTypeUnk(FILE* File, POMVHeader Header, std::wstring& FileName);

NTSTATUS ExtractOmvType2(FILE* File, POMVHeader Header, std::wstring& FileName)
{
	FileName = ReplaceFileNameExtension(FileName, L".ogv");
	return ExtractOmvTypeUnk(File, Header, FileName);
	return STATUS_SUCCESS;
}

NTSTATUS ExtractOmvTypeUnk(FILE* File, POMVHeader Header, std::wstring& FileName)
{
	NTSTATUS   Status;
	NtFileDisk Writer;
	DWORD      OggsTag;
	BYTE       Buffer[0x1000];
	ULONG      Size, ReadSize, NeedRead;
	LARGE_INTEGER TranslatedSize;

	fread(&OggsTag, 1, sizeof(OggsTag), File);
	if (OggsTag != TAG4('OggS'))
		return STATUS_INVALID_SIGNATURE;

	fseek(File, -4, FILE_CURRENT);
	int CurrentPos = ftell(File);

	Status = Writer.Create(FileName.c_str());
	if (NT_FAILED(Status))
		return Status;

	fseek(File, 0, SEEK_END);
	Size = ftell(File) - CurrentPos;
	fseek(File, CurrentPos, SEEK_SET);
	ReadSize = 0;

	while (ReadSize < Size)
	{
		if (Size - ReadSize >= sizeof(Buffer))
			NeedRead = sizeof(Buffer);
		else
			NeedRead = Size - ReadSize;

		TranslatedSize.LowPart = fread(Buffer, 1, NeedRead, File);
		ReadSize += TranslatedSize.LowPart;
		Writer.Write(Buffer, TranslatedSize.LowPart);
	}
	Writer.Close();
	return STATUS_SUCCESS;
}

int ExtractOmvType2Internal(FILE *infile, POMVHeader Header);

NTSTATUS ExtractOmvType1(FILE* File, POMVHeader Header, std::wstring& FileName)
{
	AVIFileInit();

	FileName = ReplaceFileNameExtension(FileName, L".avi");

	m_aviFile = NULL;
	auto Result = AVIFileOpenW(&m_aviFile, FileName.c_str(), OF_CREATE | OF_WRITE, 0);
	if (FAILED(Result))
	{
		PrintConsoleW(L"Failed [%p]\n", Result);
		return STATUS_UNSUCCESSFUL;
	}

	ExtractOmvType2Internal(File, Header);


	return STATUS_SUCCESS;
}

NTSTATUS FASTCALL Unpack(std::wstring& m_FileName)
{
	NTSTATUS          Status;
	PBYTE             Buffer;
	ULONG_PTR         Size, Attribute;
	std::wstring      FileName, FullPath;
	BYTE              HeaderBuffer[0xA8];
	POMVHeader        OmvHeader;
	ULONG             Offset;
	DWORD             OggsTag;

	FILE *infile = _wfopen(m_FileName.c_str(), L"rb");
	FileName = GetFileName(m_FileName);
	FullPath = FileName;

	fread(HeaderBuffer, 1, sizeof(HeaderBuffer), infile);
	OmvHeader = (POMVHeader)HeaderBuffer;

	if (OmvHeader->MainVersion != 1)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if (OmvHeader->SubVersion == 1)
		Offset = OmvHeader->DataOffset + OmvHeader->DataPackageCount * sizeof(DataPackage) + OmvHeader->FrameCount * sizeof(FramePackage);
	else if (OmvHeader->SubVersion == 0)
		Offset = OmvHeader->DataOffset;

	fseek(infile, Offset, FILE_BEGIN);

	fread(&OggsTag, 1, sizeof(OggsTag), infile);
	if (OggsTag != TAG4('OggS'))
	{
		PrintConsole(L"Not a omv stream. [%p]\n", Offset);
		return STATUS_INVALID_SIGNATURE;
	}

	fseek(infile, -4, FILE_CURRENT);

	PrintConsole(L"Converting...\n");
	switch (OmvHeader->EncryptType)
	{
	case 1:
		PrintConsole(L"Convert to 32bit avi file\n");
		Status = ExtractOmvType1(infile, OmvHeader, FullPath);
		break;

	case 2:
		PrintConsole(L"Valid theora video\n");
		Status = ExtractOmvType2(infile, OmvHeader, FullPath);
		break;
	}

	return Status;
}


static BOOL InitOnce = FALSE;

int ExtractOmvType2Internal(FILE *infile, POMVHeader Header){

	ogg_packet op;

	int long_option_index;
	int c;

	struct timeb start;
	struct timeb after;
	struct timeb last;
	int fps_only = 0;
	int frames = 0;


	/*Ok, Ogg parsing.
	The idea here is we have a bitstream that is made up of Ogg pages.
	The libogg sync layer will find them for us.
	There may be pages from several logical streams interleaved; we find the
	first theora stream and ignore any others.
	Then we pass the pages for our stream to the libogg stream layer which
	assembles our original set of packets out of them.
	It's the packets that libtheora actually knows how to handle.*/

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
	while (!stateflag){
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
				fprintf(stderr, "corrupt stream\n");
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
				fprintf(stderr, "End of file while searching for codec headers.\n");
				exit(1);
			}
		}
	}

	/* and now we have it all.  initialize decoders */
	if (theora_p){
		dump_comments(&tc);
		td = th_decode_alloc(&ti, ts);

		if (InitOnce == FALSE)
		{
			AVISTREAMINFOW m_aviVideoStreamInfo = { 0 };
			m_aviVideoStreamInfo.fccType = streamtypeVIDEO;
			m_aviVideoStreamInfo.fccHandler = 0;
			m_aviVideoStreamInfo.dwFlags = 0;
			m_aviVideoStreamInfo.dwCaps = 0;

			m_aviVideoStreamInfo.wPriority = 0;
			m_aviVideoStreamInfo.wLanguage = 0;

			//#error ""
			m_aviVideoStreamInfo.dwRate = ti.fps_numerator;

			m_aviVideoStreamInfo.dwScale = ti.fps_denominator;
			m_aviVideoStreamInfo.dwQuality = 0;
			m_aviVideoStreamInfo.dwSuggestedBufferSize = Header->Width * Header->Height * 4;

			m_aviVideoStreamInfo.dwStart = 0;

			m_aviVideoStreamInfo.dwInitialFrames = 0;

			m_aviVideoStreamInfo.dwSampleSize = 0;

			m_aviVideoStreamInfo.rcFrame.left = 0;

			m_aviVideoStreamInfo.rcFrame.top = 0;

			m_aviVideoStreamInfo.rcFrame.right = Header->Width;

			m_aviVideoStreamInfo.rcFrame.bottom = Header->Height;

			m_aviVideoStreamInfo.dwEditCount = 0;

			m_aviVideoStreamInfo.dwFormatChangeCount = 0;

			StrCopyW(m_aviVideoStreamInfo.szName, L"file.avi");
			m_aviVideoStreamInfo.dwLength = 0;

			m_aviVideoStream = NULL;
			AVIFileCreateStreamW(m_aviFile, &m_aviVideoStream, (AVISTREAMINFOW *)&m_aviVideoStreamInfo);


			InitOnce = TRUE;
		}
	}
	else{
		/* tear down the partial theora setup */
		th_info_clear(&ti);
		th_comment_clear(&tc);
	}
	/*Either way, we're done with the codec setup data.*/
	th_setup_free(ts);

	/* open video */
	if (theora_p)open_video();


	/* install signal handler */
	signal(SIGINT, sigint_handler);

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
		video_write(frames);

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


	if (m_aviVideoStream)
	{
		m_aviVideoStream->Release();
		m_aviVideoStream = NULL;
	}

	if (m_aviFile)
	{
		m_aviFile->Release();
		m_aviFile = NULL;
	}

	return(0);

}


int wmain(int argc, WCHAR* argv[])
{
	if (argc != 2)
		return 0;

	Unpack(std::wstring(argv[1]));
}