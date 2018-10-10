#pragma once

#include "my.h"
#include "iUnpackObject.h"
#include "DecodeControl.h"
#include "Tool.h"
#include <ShlObj.h>
#include <string>
#include "NwaConverter.h"
#include "endian.hpp"


class UnpackOVK : public iUnpackObject
{
	std::wstring m_FileName;

public:
	UnpackOVK(){}
	~UnpackOVK(){}

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
		std::wstring      FileName, FullPath, FullOutDirectory, FileDir, SaveFileName;
		ULONG_PTR         Size, Attribute;
		StreamWriter      Stream;
		WCHAR             ExeDirectory[MAX_PATH], SubName[MAX_PATH];
		BOOL              NeedSave;

		Code = (PDecodeControl)UserData;
		Status = File.Open(m_FileName.c_str());
		if (NT_FAILED(Status))
			return Status;

		RtlZeroMemory(ExeDirectory, sizeof(ExeDirectory));
		Nt_GetExeDirectory(ExeDirectory, countof(ExeDirectory));

		static WCHAR OutDirectory[] = L"__Unpack__\\OVK\\";

		FullOutDirectory = ExeDirectory + std::wstring(OutDirectory);
		Attribute = Nt_GetFileAttributes(FullOutDirectory.c_str());
		if (Attribute == 0xffffffff)
			SHCreateDirectory(NULL, FullOutDirectory.c_str());

		FileName = GetFileName(m_FileName);
		FullPath = FullOutDirectory + FileName;

		FileDir = GetFileNamePrefix(FullPath);
		SHCreateDirectory(NULL, FileDir.c_str());

		ULONG headblk_sz = 16;
		char buf[1024];
		memset(buf, 0, 1024);
		File.Read(buf, 4);
		int index = read_little_endian_int(buf);
		if (index <= 0)
		{
			File.Close();
			return STATUS_UNSUCCESSFUL;
		}

		int* tbl_off = new int[index];
		int* tbl_siz = new int[index];
		int* tbl_cnt = new int[index];
		int* tbl_origsiz = new int[index];

		for (INT i = 0; i < index; i++) 
		{
			File.Read(buf, headblk_sz);
			tbl_siz[i] = read_little_endian_int(buf);
			tbl_off[i] = read_little_endian_int(buf + 4);
			tbl_cnt[i] = read_little_endian_int(buf + 8);
			tbl_origsiz[i] = read_little_endian_int(buf + 12);
		}

		ULONG fsize = File.GetSize32();
		for (ULONG i = 0; i < index; i++) 
		{
			if (tbl_off[i] <= 0 || tbl_siz[i] <= 0 || tbl_off[i] + tbl_siz[i] > fsize)
				continue;
			
			FormatStringW(SubName, L"%d.ogg", tbl_cnt[i]);
			SaveFileName = FileDir + L"\\";
			SaveFileName += SubName;

			StreamWriter out;
			NtFileDisk   outf;
			if (NT_FAILED(outf.Create(SaveFileName.c_str())))
				continue;
			
			File.Seek(tbl_off[i], FILE_BEGIN);
			int sz = tbl_siz[i];
			char buf[32 * 1024];
			while (sz > 32 * 1024)
			{
				File.Read(buf, 32 * 1024);
				out.Write(buf, 32 * 1024);
				sz -= 1024 * 32;
			}
			if (sz > 0) 
			{
				File.Read(buf, sz);
				out.Write(buf, sz);
			}

			outf.Write(out.GetBuffer(), out.GetSize());
			outf.Close();
		}

		return STATUS_SUCCESS;
	}
};
