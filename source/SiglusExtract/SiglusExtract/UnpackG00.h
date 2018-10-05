#pragma once

#include "my.h"
#include "iUnpackObject.h"
#include "DecodeControl.h"
#include "Tool.h"
#include <ShlObj.h>
#include <string>
#include <stdint.h>
#include <vector>
#include "lodepng.h"
#include "png.h"
#include "MemStream.h"
#include <gdiplus.h>

#pragma comment(lib, "..\\Release\\zlib.lib")
#pragma comment(lib, "..\\Release\\libpng16.lib")

enum g00_type_info
{
	type_24bit,
	type_8bit,
	type_dir
};


#pragma pack(1)

typedef struct g00_header_s
{
	uint8_t type;
	uint16_t width;
	uint16_t height;
}g00_header_t;
typedef struct g00_24bit_header_s
{
	uint32_t compress_length;
	uint32_t decompress_length;
}g00_24bit_header_t;
typedef struct g02_info_s{
	uint32_t orig_x;			// 原点在屏幕中的位置
	uint32_t orig_y;
	uint32_t end_x;			// 终点在屏幕中的位置
	uint32_t end_y;
	uint32_t unknown[2];		// 0
}g02_info_t;
#pragma pack()
//D0
typedef struct g02_part_info_s
{
	uint16_t type;					// type 0 1 2					//0
	uint16_t block_count;			// g02_block_info_t count     //2
	uint32_t hs_orig_x;				//hot part x        //4
	uint32_t hs_orig_y;				//hot part y			//8
	uint32_t width;					//pic width						//C
	uint32_t height;				//pic height					//10
	uint32_t screen_show_x;											//14
	uint32_t screen_show_y;											//18
	uint32_t full_part_width;		//0x101							//1C
	uint32_t full_part_height;		//0x30							//20
	uint32_t reserved[20];			//unknown						..
}g02_part_info_t;




typedef struct g02_block_info_s
{
	uint16_t orig_x; //block start x
	uint16_t orig_y;  //block start y
	uint16_t info;		//0 = center 
	uint16_t width;
	uint16_t height;
	uint16_t reserved[41];//zero?
}g02_block_info_t;

typedef struct g02_pair_s
{
	uint32_t offset;
	uint32_t length;
}g02_pair_t;

typedef struct lzss_compress_head_s
{
	uint32_t compress_length;
	uint32_t decompress_length;
}lzss_compress_head_t;


typedef struct g00_extract_imginfo_s
{
	uint32_t type;
	uint32_t full_part_width;
	uint32_t full_part_height;
	uint32_t screen_show_x;
	uint32_t screen_show_y;
	uint32_t hs_orig_x;
	uint32_t hs_orig_y;
	uint32_t part_width;
	uint32_t part_height;
}g00_extract_imginfo_t;
typedef struct g00_extract_info_s
{
	uint32_t type;
	uint32_t orig_x;
	uint32_t orig_y;
	uint32_t width;
	uint32_t height;
	uint32_t info;
}g00_extract_info_t;

#pragma pack()


typedef __int8 tjs_int8;
typedef unsigned __int8 tjs_uint8;
typedef __int16 tjs_int16;
typedef unsigned __int16 tjs_uint16;
typedef __int32 tjs_int32;
typedef unsigned __int32 tjs_uint32;
typedef __int64 tjs_int64;
typedef unsigned __int64 tjs_uint64;
typedef int tjs_int;    /* at least 32bits */
typedef unsigned int tjs_uint;    /* at least 32bits */


#ifndef TJS_INTF_METHOD
#define TJS_INTF_METHOD __cdecl
#endif

#define TJS_BS_SEEK_SET 0
#define TJS_BS_SEEK_CUR 1
#define TJS_BS_SEEK_END 2

class tTJSBinaryStream2
{
private:
public:
	//-- must implement
	virtual tjs_uint64 TJS_INTF_METHOD Seek(tjs_int64 offset, tjs_int whence) = 0;
	/* if error, position is not changed */

	//-- optionally to implement
	virtual tjs_uint TJS_INTF_METHOD Read(void *buffer, tjs_uint read_size) = 0;
	/* returns actually read size */

	virtual tjs_uint TJS_INTF_METHOD Write(const void *buffer, tjs_uint write_size) = 0;
	/* returns actually written size */

	virtual void TJS_INTF_METHOD SetEndOfStorage();
	// the default behavior is raising a exception
	/* if error, raises exception */

	//-- should re-implement for higher performance
	virtual tjs_uint64 TJS_INTF_METHOD GetSize() = 0;

	virtual ~tTJSBinaryStream2() { ; }

	tjs_uint64 GetPosition();

	void SetPosition(tjs_uint64 pos);

	void ReadBuffer(void *buffer, tjs_uint read_size);
	void WriteBuffer(const void *buffer, tjs_uint write_size);

	tjs_uint64 ReadI64LE(); // reads little-endian integers
	tjs_uint32 ReadI32LE();
	tjs_uint16 ReadI16LE();
};


void TJS_INTF_METHOD tTJSBinaryStream2::SetEndOfStorage()
{
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTJSBinaryStream2::GetSize()
{
	tjs_uint64 orgpos = GetPosition();
	tjs_uint64 size = Seek(0, TJS_BS_SEEK_END);
	Seek(orgpos, SEEK_SET);
	return size;
}
//---------------------------------------------------------------------------
tjs_uint64 tTJSBinaryStream2::GetPosition()
{
	return Seek(0, SEEK_CUR);
}
//---------------------------------------------------------------------------
void tTJSBinaryStream2::SetPosition(tjs_uint64 pos)
{
	Seek(pos, TJS_BS_SEEK_SET);
}
//---------------------------------------------------------------------------
void tTJSBinaryStream2::ReadBuffer(void *buffer, tjs_uint read_size)
{
	Read(buffer, read_size);
}
//---------------------------------------------------------------------------
void tTJSBinaryStream2::WriteBuffer(const void *buffer, tjs_uint write_size)
{
	Write(buffer, write_size);
}
//---------------------------------------------------------------------------
tjs_uint64 tTJSBinaryStream2::ReadI64LE()
{
	tjs_uint64 temp;
	ReadBuffer(&temp, 8);
	return temp;
}
//---------------------------------------------------------------------------
tjs_uint32 tTJSBinaryStream2::ReadI32LE()
{
	tjs_uint32 temp;
	ReadBuffer(&temp, 4);
	return temp;
}
//---------------------------------------------------------------------------
tjs_uint16 tTJSBinaryStream2::ReadI16LE()
{
	tjs_uint16 temp;
	ReadBuffer(&temp, 2);
	return temp;
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders  
	UINT  size = 0;         // size of the image encoder array in bytes  

	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;

	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)AllocateMemoryP(size);
	if (pImageCodecInfo == NULL)
		return -1;

	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (StrCompareW(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			FreeMemoryP(pImageCodecInfo);
			return j;
		}
	}

	FreeMemoryP(pImageCodecInfo);
	return -1;
}


class tTVPMemoryStream : public tTJSBinaryStream2
{
protected:
	void * Block;
	tjs_uint Size;
	tjs_uint AllocSize;
	tjs_uint CurrentPos;

public:
	tTVPMemoryStream();
	tTVPMemoryStream(const void * block, tjs_uint size);
	~tTVPMemoryStream();

	tjs_uint64 TJS_INTF_METHOD Seek(tjs_int64 offset, tjs_int whence);

	tjs_uint TJS_INTF_METHOD Read(void *buffer, tjs_uint read_size);
	tjs_uint TJS_INTF_METHOD Write(const void *buffer, tjs_uint write_size);
	void TJS_INTF_METHOD SetEndOfStorage();

	tjs_uint64 TJS_INTF_METHOD GetSize() { return Size; }

	// non-tTJSBinaryStream based methods
	void * GetInternalBuffer()  const { return Block; }
	void Clear(void);
	void SetSize(tjs_uint size);

protected:
	void Init();

protected:
	virtual void * Alloc(size_t size);
	virtual void * Realloc(void *orgblock, size_t size);
	virtual void Free(void *block);
};


tTVPMemoryStream::tTVPMemoryStream()
{
	Init();
}
//---------------------------------------------------------------------------
tTVPMemoryStream::tTVPMemoryStream(const void * block, tjs_uint size)
{
	Init();
	Block = (void*)block;
	Size = size;
	AllocSize = size;
	CurrentPos = 0;
}
//---------------------------------------------------------------------------
tTVPMemoryStream::~tTVPMemoryStream()
{
	if (Block) Free(Block);
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTVPMemoryStream::Seek(tjs_int64 offset, tjs_int whence)
{
	tjs_int64 newpos;
	switch (whence)
	{
	case TJS_BS_SEEK_SET:
		if (offset >= 0)
		{
			if (offset <= Size) CurrentPos = static_cast<tjs_uint>(offset);
		}
		return CurrentPos;

	case TJS_BS_SEEK_CUR:
		if ((newpos = offset + (tjs_int64)CurrentPos) >= 0)
		{
			tjs_uint np = (tjs_uint)newpos;
			if (np <= Size) CurrentPos = np;
		}
		return CurrentPos;

	case TJS_BS_SEEK_END:
		if ((newpos = offset + (tjs_int64)Size) >= 0)
		{
			tjs_uint np = (tjs_uint)newpos;
			if (np <= Size) CurrentPos = np;
		}
		return CurrentPos;
	}
	return CurrentPos;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPMemoryStream::Read(void *buffer, tjs_uint read_size)
{
	if (CurrentPos + read_size >= Size)
	{
		read_size = Size - CurrentPos;
	}

	memcpy(buffer, (tjs_uint8*)Block + CurrentPos, read_size);

	CurrentPos += read_size;

	return read_size;
}
//---------------------------------------------------------------------------
tjs_uint TJS_INTF_METHOD tTVPMemoryStream::Write(const void *buffer, tjs_uint write_size)
{
	tjs_uint newpos = CurrentPos + write_size;
	if (newpos >= AllocSize)
	{
		// exceeds AllocSize
		tjs_uint onesize;
		if (AllocSize < 64 * 1024) onesize = 4 * 1024;
		else if (AllocSize < 512 * 1024) onesize = 16 * 1024;
		else if (AllocSize < 4096 * 1024) onesize = 256 * 1024;
		else onesize = 2024 * 1024;
		AllocSize += onesize;

		if (CurrentPos + write_size >= AllocSize) // still insufficient ?
		{
			AllocSize = CurrentPos + write_size;
		}

		Block = Realloc(Block, AllocSize);

		if (AllocSize && !Block)
			return 0;
	}

	memcpy((tjs_uint8*)Block + CurrentPos, buffer, write_size);

	CurrentPos = newpos;

	if (CurrentPos > Size) Size = CurrentPos;

	return write_size;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPMemoryStream::SetEndOfStorage()
{
	Size = CurrentPos;
	AllocSize = Size;
	Block = Realloc(Block, Size);
}
//---------------------------------------------------------------------------
void tTVPMemoryStream::Clear(void)
{
	if (Block) Free(Block);
	Init();
}
//---------------------------------------------------------------------------
void tTVPMemoryStream::SetSize(tjs_uint size)
{
	if (Size > size)
	{
		// decrease
		Size = size;
		AllocSize = size;
		Block = Realloc(Block, size);
		if (CurrentPos > Size) CurrentPos = Size;
		if (size && !Block)
			return;
	}
	else
	{
		// increase
		AllocSize = size;
		Size = size;
		Block = Realloc(Block, size);
		if (size && !Block)
			return;

	}
}
//---------------------------------------------------------------------------
void tTVPMemoryStream::Init()
{
	Block = NULL;
	Size = 0;
	AllocSize = 0;
	CurrentPos = 0;
}
//---------------------------------------------------------------------------
void * tTVPMemoryStream::Alloc(size_t size)
{
	return AllocateMemoryP(size);
}
//---------------------------------------------------------------------------
void * tTVPMemoryStream::Realloc(void *orgblock, size_t size)
{
	return ReAllocateMemoryP(orgblock, size);
}
//---------------------------------------------------------------------------
void tTVPMemoryStream::Free(void *block)
{
	FreeMemoryP(block);
}

class tTVPIStreamAdapter2 : public IStream
{
private:
	tTJSBinaryStream2 *Stream;
	ULONG RefCount;

public:
	tTVPIStreamAdapter2(tTJSBinaryStream2 *ref);
	/*
	the stream passed by argument here is freed by this instance'
	destruction.
	*/

	~tTVPIStreamAdapter2();


	// IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
		void **ppvObject);
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);

	// ISequentialStream
	HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead);
	HRESULT STDMETHODCALLTYPE Write(const void *pv, ULONG cb,
		ULONG *pcbWritten);

	// IStream
	HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove,
		DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
	HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize);
	HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm, ULARGE_INTEGER cb,
		ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
	HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags);
	HRESULT STDMETHODCALLTYPE Revert(void);
	HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset,
		ULARGE_INTEGER cb, DWORD dwLockType);
	HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset,
		ULARGE_INTEGER cb, DWORD dwLockType);
	HRESULT STDMETHODCALLTYPE Stat(STATSTG *pstatstg, DWORD grfStatFlag);
	HRESULT STDMETHODCALLTYPE Clone(IStream **ppstm);

	void ClearStream() {
		Stream = NULL;
	}
};



tTVPIStreamAdapter2::tTVPIStreamAdapter2(tTJSBinaryStream2 *ref)
{
	Stream = ref;
	RefCount = 1;
}
//---------------------------------------------------------------------------
tTVPIStreamAdapter2::~tTVPIStreamAdapter2()
{
	delete Stream;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::QueryInterface(REFIID riid,
	void **ppvObject)
{
	if (!ppvObject) return E_INVALIDARG;

	*ppvObject = NULL;
	if (RtlCompareMemory(&riid, &IID_IUnknown, 16) == 16)
		*ppvObject = (IUnknown*)this;
	else if (RtlCompareMemory(&riid, &IID_ISequentialStream, 16) == 16)
		*ppvObject = (ISequentialStream*)this;
	else if (RtlCompareMemory(&riid, &IID_IStream, 16) == 16)
		*ppvObject = (IStream*)this;

	if (*ppvObject)
	{
		AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}
//---------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE tTVPIStreamAdapter2::AddRef(void)
{
	return ++RefCount;
}
//---------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE tTVPIStreamAdapter2::Release(void)
{
	if (RefCount == 1)
	{
		delete this;
		return 0;
	}
	else
	{
		return --RefCount;
	}
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	ULONG read;
	read = Stream->Read(pv, cb);
	if (pcbRead) *pcbRead = read;
	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::Write(const void *pv, ULONG cb,
	ULONG *pcbWritten)
{
	return E_FAIL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::Seek(LARGE_INTEGER dlibMove,
	DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
	switch (dwOrigin)
	{
	case STREAM_SEEK_SET:
		if (plibNewPosition)
			(*plibNewPosition).QuadPart =
			Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_SET);
		else
			Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_SET);
		break;
	case STREAM_SEEK_CUR:
		if (plibNewPosition)
			(*plibNewPosition).QuadPart =
			Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_CUR);
		else
			Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_CUR);
		break;
	case STREAM_SEEK_END:
		if (plibNewPosition)
			(*plibNewPosition).QuadPart =
			Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_END);
		else
			Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_END);
		break;
	default:
		return E_FAIL;
	}
	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::SetSize(ULARGE_INTEGER libNewSize)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::CopyTo(IStream *pstm, ULARGE_INTEGER cb,
	ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::Commit(DWORD grfCommitFlags)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::Revert(void)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::LockRegion(ULARGE_INTEGER libOffset,
	ULARGE_INTEGER cb, DWORD dwLockType)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::UnlockRegion(ULARGE_INTEGER libOffset,
	ULARGE_INTEGER cb, DWORD dwLockType)
{
	return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
	if (pstatstg)
	{
		ZeroMemory(pstatstg, sizeof(*pstatstg));

		if (!(grfStatFlag &  STATFLAG_NONAME))
		{
			// anyway returns an empty string
			LPWSTR str = (LPWSTR)CoTaskMemAlloc(sizeof(*str));
			if (str == NULL) return E_OUTOFMEMORY;
			*str = L'\0';
			pstatstg->pwcsName = str;
		}

		// type
		pstatstg->type = STGTY_STREAM;

		pstatstg->cbSize.QuadPart = Stream->GetSize();
		pstatstg->grfMode = STGM_DIRECT | STGM_READWRITE | STGM_SHARE_DENY_WRITE;
		pstatstg->grfLocksSupported = 0;

	}
	else
	{
		return E_INVALIDARG;
	}

	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter2::Clone(IStream **ppstm)
{
	return E_NOTIMPL;
}


class UnpackG00 : public iUnpackObject
{
	std::wstring m_FileName;

public:
	UnpackG00(){};
	~UnpackG00(){};

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
		std::wstring      FileName, FullPath, FullOutDirectory, DirName;
		ULONG_PTR         Size, Attribute;
		WCHAR             ExeDirectory[MAX_PATH];
		BOOL              NeedSave;

		Code = (PDecodeControl)UserData;
		Status = File.Open(m_FileName.c_str());
		if (NT_FAILED(Status))
			return Status;

		Size = File.GetSize32();
		Buffer = (PBYTE)AllocateMemoryP(Size);
		if (!Buffer)
		{
			File.Close();
			return STATUS_NO_MEMORY;
		}

		File.Read(Buffer, Size);
		File.Close();

		RtlZeroMemory(ExeDirectory, sizeof(ExeDirectory));
		Nt_GetExeDirectory(ExeDirectory, countof(ExeDirectory));

		static WCHAR OutDirectory[] = L"__Unpack__\\G00\\";

		FullOutDirectory = ExeDirectory + std::wstring(OutDirectory);
		Attribute = Nt_GetFileAttributes(FullOutDirectory.c_str());
		if (Attribute == 0xffffffff)
			SHCreateDirectory(NULL, FullOutDirectory.c_str());

		FileName = GetFileName(m_FileName);
		FullPath = FullOutDirectory + FileName;

		switch (Code->G00Flag)
		{
		default:
		case G00_BMP:
			FullPath = ReplaceFileNameExtension(FullPath, L".bmp");
			break;

		case G00_PNG:
			FullPath = ReplaceFileNameExtension(FullPath, L".png");
			break;

		case G00_JPG:
			FullPath = ReplaceFileNameExtension(FullPath, L".jpg");
			break;
		}

		auto pheader = (g00_header_t*)Buffer;

		switch (pheader->type)
		{
		case type_24bit:
			extract_g00_type0_pic(pheader, Code, FullPath);
			break;

		case type_8bit:
			extract_g00_type1_pic(pheader, Code, FullPath);
			break;

		case type_dir:
			DirName = GetFileNamePrefix(FullPath);
			SHCreateDirectory(NULL, DirName.c_str());
			extract_g00_type2_pic(pheader, Code, DirName);
			break;
		default:
			break;
		}



		FreeMemoryP(Buffer);
		return STATUS_SUCCESS;
	}

private:
	uint32_t get_g02_index_entries(g00_header_t* pheader)
	{
		uint8_t * pbuf = (uint8_t*)pheader;

		pbuf += sizeof(g00_header_t);
		return *(uint32_t*)pbuf;
	}

	int RealLive_g00_type1_uncompress(BYTE *compr, BYTE **ret_uncompr,
		DWORD *ret_uncomprlen)
	{
		DWORD comprlen = *(u32 *)compr - 8;
		DWORD uncomprlen = *(u32 *)&compr[4];
		if (uncomprlen) {
			BYTE *uncompr = new BYTE[uncomprlen + 64];
			if (!uncompr)
				return -1;
			memset(uncompr, 0, uncomprlen + 64);

			comprlen += 8;
			DWORD curbyte = 8;
			DWORD act_uncomprlen = 0;
			DWORD bit_count = 0;
			DWORD flag;
			while (act_uncomprlen < uncomprlen && curbyte < comprlen) {
				if (!bit_count) {
					flag = compr[curbyte++];
					bit_count = 8;
				}
				if (flag & 1)
					uncompr[act_uncomprlen++] = compr[curbyte++];
				else {
					DWORD count = compr[curbyte++];
					DWORD offset = (compr[curbyte++] << 4) | (count >> 4);
					count = (count & 0xf) + 2;
					for (DWORD i = 0; i < count; ++i) {
						uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
						++act_uncomprlen;
					}
				}
				flag >>= 1;
				--bit_count;
			}
			*ret_uncompr = uncompr;
			*ret_uncomprlen = uncomprlen;
		}
		else {
			BYTE *actual_data = new BYTE[comprlen];
			if (!actual_data)
				return -1;
			memcpy(actual_data, compr + 8, comprlen);
			*ret_uncompr = actual_data;
			*ret_uncomprlen = comprlen;
		}

		return 0;
	}

	void lzss_decompress_24bit(unsigned char* src, unsigned char* dst, unsigned char* dst_end)
	{
		__asm
		{
			mov esi, src;
			mov edi, dst;
			xor edx, edx;
			cld;
		Loop1:
			cmp edi, dst_end;
			je End;
			mov dl, byte ptr[esi];
			inc esi;
			mov dh, 0x8;
		Loop2:
			cmp edi, dst_end;
			je End;
			test dl, 1;
			je DecompTag;
			movsw;
			movsb;
			mov byte ptr[edi], 0xFF;
			inc edi;
			jmp DecompTag2;
		DecompTag:
			xor eax, eax;
			lods word ptr[esi];
			mov ecx, eax;
			shr eax, 4;
			shl eax, 2;
			and ecx, 0xF;
			add ecx, 0x1;
			mov ebx, esi;
			mov esi, edi;
			sub esi, eax;
			rep movsd;
			mov esi, ebx;
		DecompTag2:
			shr dl, 1;
			dec dh;
			jnz Loop2;
			jmp Loop1;
		End:
		}
	}

	void lzss_decompress(unsigned char* src, unsigned char* dst, unsigned char* dst_end)
	{
		__asm
		{
			mov esi, src;
			mov edi, dst;
			xor edx, edx;
			cld;
		Loop1:
			cmp edi, dst_end;
			je End;
			mov dl, byte ptr[esi];
			inc esi;
			mov dh, 0x8;
		Loop2:
			cmp edi, dst_end;
			je End;
			test dl, 1;
			je DecompTag;
			movsb;
			jmp DecompTag2;
		DecompTag:
			xor eax, eax;
			lods word ptr[esi];
			mov ecx, eax;
			shr eax, 4;
			and ecx, 0xF;
			add ecx, 0x2;
			mov ebx, esi;
			mov esi, edi;
			sub esi, eax;
			rep movsb;
			mov esi, ebx;
		DecompTag2:
			shr dl, 1;
			dec dh;
			jnz Loop2;
			jmp Loop1;
		End:
		}
	}

	void AlphaBlend(BYTE* dib, int width, int height)
	{
		auto p = dib;
		for (int i = 0; i < width*height; i++)
		{
			p[0] = p[0] * p[3] / 255 + 255 - p[3];
			p[1] = p[1] * p[3] / 255 + 255 - p[3];
			p[2] = p[2] * p[3] / 255 + 255 - p[3];
			p += 4;
		}
	}


	bool savebmp(const wchar_t *file, int width, int height, void *data)
	{
		int nAlignWidth = (width * 32 + 31) / 32;
		FILE *pfile;


		BITMAPFILEHEADER Header;
		BITMAPINFOHEADER HeaderInfo;
		Header.bfType = 0x4D42;
		Header.bfReserved1 = 0;
		Header.bfReserved2 = 0;
		Header.bfOffBits = (uint32_t)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
		Header.bfSize = (uint32_t)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + nAlignWidth* height * 4);
		HeaderInfo.biSize = sizeof(BITMAPINFOHEADER);
		HeaderInfo.biWidth = width;
		HeaderInfo.biHeight = height;
		HeaderInfo.biPlanes = 1;
		HeaderInfo.biBitCount = 32;
		HeaderInfo.biCompression = 0;
		HeaderInfo.biSizeImage = 4 * nAlignWidth * height;
		HeaderInfo.biXPelsPerMeter = 0;
		HeaderInfo.biYPelsPerMeter = 0;
		HeaderInfo.biClrUsed = 0;
		HeaderInfo.biClrImportant = 0;


		if (!(pfile = _wfopen(file, L"wb")))
		{
			return false;
		}


		fwrite(&Header, 1, sizeof(BITMAPFILEHEADER), pfile);
		fwrite(&HeaderInfo, 1, sizeof(BITMAPINFOHEADER), pfile);
		fwrite(data, 1, HeaderInfo.biSizeImage, pfile);
		fclose(pfile);

		return false;
	}

	void fix_bmp(uint32_t width, uint32_t height, uint8_t*data)
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
		delete[] tmp_data;
	}

	void part_extract_buf(g00_extract_info_t* info, uint32_t width_length, uint8_t* src, uint8_t* dst)
	{
		switch (info->type)
		{
		case 2:
		{
			uint32_t hotpart_width_length = (info->width * 4);
			uint32_t line_length = width_length - hotpart_width_length;

			for (uint32_t n = 0; n<info->height; n++)
			{
				memcpy(dst, src, hotpart_width_length);
				src += hotpart_width_length;
				dst += width_length;
			}
			break;
		}
		}
	}

	void extract_g02_part(g02_part_info_t* part_info, uint8_t** out_buf, uint32_t* out_len)
	{
		uint8_t* buf = (uint8_t*)part_info;
		uint8_t* dib_buf;
		uint32_t dib_length;
		buf += sizeof(g02_part_info_t);

		//extract info
		g00_extract_imginfo_t img_info;
		g00_extract_info_t info;


		switch (part_info->type)
		{
		case 0:
		{
			break;
		}
		case 2:
		case 1:
		{

			img_info.type = 2;
			img_info.full_part_width = part_info->full_part_width;
			img_info.full_part_height = part_info->full_part_height;
			img_info.screen_show_x = part_info->screen_show_x;
			img_info.screen_show_y = part_info->screen_show_y;
			img_info.hs_orig_x = part_info->hs_orig_x;
			img_info.hs_orig_y = part_info->hs_orig_y;
			img_info.part_width = part_info->width + part_info->hs_orig_x;
			img_info.part_height = part_info->height + part_info->hs_orig_y;

			dib_length = part_info->width * part_info->height * 4;
			dib_buf = (uint8_t*)malloc(dib_length);
			memset(dib_buf, 0, dib_length);

			//hot part
			for (uint32_t n = 0; n<part_info->block_count; n++)
			{
				g02_block_info_t* block = (g02_block_info_t*)buf;
				info.type = img_info.type;
				info.orig_x = block->orig_x;
				info.orig_y = block->orig_y;
				info.width = block->width;
				info.height = block->height;
				buf += sizeof(g02_block_info_t);
				uint32_t width_length = (part_info->width * 4);
				uint8_t* dst = dib_buf + ((info.orig_y - img_info.hs_orig_y) * width_length) + (info.orig_x - img_info.hs_orig_x) * 4;


				part_extract_buf(&info, width_length, buf, dst);
				buf += block->width * block->height * 4; // 32 bit map

			}
			*out_buf = dib_buf;
			*out_len = dib_length;
			break;
		}
		}
	}

	void extract_g00_type2_pic(g00_header_t *pheader, PDecodeControl Code, std::wstring& FileDir)
	{
		uint8_t* buf = ((uint8_t*)pheader) + sizeof(g00_header_t);
		uint8_t* debuf;


		std::vector <g02_info_t*> g02_info_list;
		lzss_compress_head_t* compress_info;


		uint32_t index_entries = *(uint32_t*)buf;

		uint32_t debuf_entries;

		buf += sizeof(uint32_t);

		for (uint32_t i = 0; i<index_entries; i++)
		{
			g02_info_t* m_info = new g02_info_t;
			memcpy(m_info, buf, sizeof(g02_info_t));
			buf += sizeof(g02_info_t);
			g02_info_list.push_back(m_info);
		}


		compress_info = (lzss_compress_head_t*)buf;

		debuf = new uint8_t[compress_info->decompress_length];

		buf += sizeof(lzss_compress_head_t);

		//decompress
		lzss_decompress(buf, debuf, debuf + compress_info->decompress_length);

		debuf_entries = *(uint32_t*)debuf;
		g02_pair_t* entries = (g02_pair_t*)(debuf + 4);

		for (uint32_t i = 0; i<debuf_entries; i++)
		{
			uint8_t* img_buf;
			uint32_t img_len;
			if (entries[i].offset && entries[i].length)
			{
				WCHAR OutFileName[MAX_PATH];

				switch (Code->G00Flag)
				{
				default:
				case G00_BMP:
					FormatStringW(OutFileName, L"%s\\%04d.bmp", FileDir.c_str(), i);
					break;

				case G00_PNG:
					FormatStringW(OutFileName, L"%s\\%04d.png", FileDir.c_str(), i);
					break;

				case G00_JPG:
					FormatStringW(OutFileName, L"%s\\%04d.jpg", FileDir.c_str(), i);
					break;
				}

				g02_part_info_t * g02_part = (g02_part_info_t*)&debuf[entries[i].offset];

				extract_g02_part(g02_part, &img_buf, &img_len);
				fix_bmp(g02_part->width, g02_part->height, img_buf);
				

				switch (Code->G00Flag)
				{
				default:
				case G00_BMP:
					savebmp(OutFileName, g02_part->width, g02_part->height, img_buf);
					break;

				case G00_PNG:
					savepng(OutFileName, g02_part->width, g02_part->height, img_buf);
					break;

				case G00_JPG:
					savejpg(OutFileName, g02_part->width, g02_part->height, img_buf);
					break;
				}

				free(img_buf);
			}
		}
		delete[] debuf;
		for (auto& Item : g02_info_list)
			delete Item;

		g02_info_list.clear();
	}

	void extract_g00_type1_pic(g00_header_t *pheader, PDecodeControl Code, std::wstring& FileName)
	{
		uint8_t* buf = ((uint8_t*)pheader) + sizeof(g00_header_t);
		uint8_t* debuf;
		DWORD    desize = 0;

		RealLive_g00_type1_uncompress(buf, &debuf, &desize);

		fix_bmp(pheader->width, pheader->height, debuf);
		switch (Code->G00Flag)
		{
		default:
		case G00_BMP:
			savebmp(FileName.c_str(), pheader->width, pheader->height, debuf);
			break;

		case G00_PNG:
			savepng(FileName.c_str(), pheader->width, pheader->height, debuf);
			break;

		case G00_JPG:
			savejpg(FileName.c_str(), pheader->width, pheader->height, debuf);
			break;
		}

		delete debuf;
	}

	void extract_g00_type0_pic(g00_header_t *pheader, PDecodeControl Code, std::wstring& FileName)
	{
		uint8_t* buf = ((uint8_t*)pheader) + sizeof(g00_header_t);
		lzss_compress_head_t* compress_info;
		uint8_t* debuf;
		compress_info = (lzss_compress_head_t*)buf;
		buf += sizeof(lzss_compress_head_t);


		debuf = new uint8_t[compress_info->decompress_length];
		lzss_decompress_24bit(buf, debuf, debuf + compress_info->decompress_length);
		

		fix_bmp(pheader->width, pheader->height, debuf);
		

		switch (Code->G00Flag)
		{
		default:
		case G00_BMP:
			savebmp(FileName.c_str(), pheader->width, pheader->height, debuf);
			break;

		case G00_PNG:
			savepng(FileName.c_str(), pheader->width, pheader->height, debuf);
			break;

		case G00_JPG:
			savejpg(FileName.c_str(), pheader->width, pheader->height, debuf);
			break;
		}

		delete[] debuf;

	}


	unsigned decodeBMP(std::vector<unsigned char>& image, unsigned& w, unsigned& h, const std::vector<unsigned char>& bmp)
	{
		static const unsigned MINHEADER = 54; //minimum BMP header size

		if (bmp.size() < MINHEADER) return -1;
		if (bmp[0] != 'B' || bmp[1] != 'M') return 1; //It's not a BMP file if it doesn't start with marker 'BM'
		unsigned pixeloffset = bmp[10] + 256 * bmp[11]; //where the pixel data starts
		//read width and height from BMP header
		w = bmp[18] + bmp[19] * 256;
		h = bmp[22] + bmp[23] * 256;
		//read number of channels from BMP header
		if (bmp[28] != 24 && bmp[28] != 32) return 2; //only 24-bit and 32-bit BMPs are supported.
		unsigned numChannels = bmp[28] / 8;

		//The amount of scanline bytes is width of image times channels, with extra bytes added if needed
		//to make it a multiple of 4 bytes.
		unsigned scanlineBytes = w * numChannels;
		if (scanlineBytes % 4 != 0) scanlineBytes = (scanlineBytes / 4) * 4 + 4;

		unsigned dataSize = scanlineBytes * h;
		if (bmp.size() < dataSize + pixeloffset) return 3; //BMP file too small to contain all pixels

		image.resize(w * h * 4);

		/*
		There are 3 differences between BMP and the raw image buffer for LodePNG:
		-it's upside down
		-it's in BGR instead of RGB format (or BRGA instead of RGBA)
		-each scanline has padding bytes to make it a multiple of 4 if needed
		The 2D for loop below does all these 3 conversions at once.
		*/
		for (unsigned y = 0; y < h; y++)
			for (unsigned x = 0; x < w; x++)
			{
				//pixel start byte position in the BMP
				unsigned bmpos = pixeloffset + (h - y - 1) * scanlineBytes + numChannels * x;
				//pixel start byte position in the new raw image
				unsigned newpos = 4 * y * w + 4 * x;
				if (numChannels == 3)
				{
					image[newpos + 0] = bmp[bmpos + 2]; //R
					image[newpos + 1] = bmp[bmpos + 1]; //G
					image[newpos + 2] = bmp[bmpos + 0]; //B
					image[newpos + 3] = 255;            //A
				}
				else
				{
					image[newpos + 0] = bmp[bmpos + 3]; //R
					image[newpos + 1] = bmp[bmpos + 2]; //G
					image[newpos + 2] = bmp[bmpos + 1]; //B
					image[newpos + 3] = bmp[bmpos + 0]; //A
				}
			}
		return 0;
	}


	bool savepng(const wchar_t *file, int width, int height, void *data)
	{
		std::vector<BYTE> BmpImage;
		BYTE              plte[0x400];
		MemStream         Stream;
		ULONG             Size;

		auto p = (BYTE*)plte;
		for (int i = 0; i < 0x100; i++)
		{
			p[0] = p[1] = p[2] = i;
			p[3] = 0;
			p += 4;
		}

		Size = 4 * abs(width) * abs(height) * 4;
		Stream.start = Stream.cur = new BYTE[Size];
		Stream.len = Size;

		if (EncodeBmpToPng(width, height, 32, plte, data, &Stream))
			return false;

		Size = Stream.cur - Stream.start;

		FILE* File = _wfopen(file, L"wb");
		if (!File)
			return false;

		fwrite(Stream.start, 1, Size, File);
		fclose(File);

		delete[] Stream.start;
		return true;
	}

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

	bool savejpg(const wchar_t *file, int width, int height, void *data)
	{
		ULONG    Quality;
		IStream* Stream;
		CLSID    EncoderClsidJPG;
		std::vector<BYTE> Image;
		Gdiplus::EncoderParameters JPGEncoderParameters;

		savebmp_buffer(width, height, data, Image);

		GetEncoderClsid(L"image/jpeg", &EncoderClsidJPG);
		Stream = new tTVPIStreamAdapter2(new tTVPMemoryStream(&Image[0], Image.size()));

		JPGEncoderParameters.Count = 1;
		JPGEncoderParameters.Parameter[0].Guid = Gdiplus::EncoderQuality;
		JPGEncoderParameters.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
		JPGEncoderParameters.Parameter[0].NumberOfValues = 1;
		Quality = 100;
		JPGEncoderParameters.Parameter[0].Value = &Quality;

		Gdiplus::Image* Img = new Gdiplus::Image(Stream);
		Gdiplus::Status GStatus = Img->Save(file, &EncoderClsidJPG, &JPGEncoderParameters);
		delete Img;
		delete Stream;

		return true;
	}

	bool savebmp_buffer(int width, int height, void *data, std::vector<BYTE>& Image)
	{
		int nAlignWidth = (width * 32 + 31) / 32;
		FILE *pfile;


		BITMAPFILEHEADER Header;
		BITMAPINFOHEADER HeaderInfo;
		Header.bfType = 0x4D42;
		Header.bfReserved1 = 0;
		Header.bfReserved2 = 0;
		Header.bfOffBits = (uint32_t)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
		Header.bfSize = (uint32_t)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + nAlignWidth* height * 4);
		HeaderInfo.biSize = sizeof(BITMAPINFOHEADER);
		HeaderInfo.biWidth = width;
		HeaderInfo.biHeight = height;
		HeaderInfo.biPlanes = 1;
		HeaderInfo.biBitCount = 32;
		HeaderInfo.biCompression = 0;
		HeaderInfo.biSizeImage = 4 * nAlignWidth * height;
		HeaderInfo.biXPelsPerMeter = 0;
		HeaderInfo.biYPelsPerMeter = 0;
		HeaderInfo.biClrUsed = 0;
		HeaderInfo.biClrImportant = 0;

		for (ULONG i = 0; i < sizeof(Header); i++)
		{
			Image.push_back(((PBYTE)&Header)[i]);
		}

		for (ULONG i = 0; i < sizeof(HeaderInfo); i++)
		{
			Image.push_back(((PBYTE)&HeaderInfo)[i]);
		}

		for (ULONG i = 0; i < HeaderInfo.biSizeImage; i++)
		{
			Image.push_back(((PBYTE)data)[i]);
		}

		return true;
	}
};
