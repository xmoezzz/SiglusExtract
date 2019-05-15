#pragma once

#include "my.h"
#include "iUnpackObject.h"
#include "DecodeControl.h"
#include "Tool.h"
#include <ShlObj.h>
#include <string>
#include "NwaConverter.h"

class UnpackNWA : public iUnpackObject
{
	std::wstring m_FileName;

public:
	UnpackNWA(){}
	~UnpackNWA(){}

	VOID FASTCALL SetFile(LPCWSTR FileName)
	{
		m_FileName = FileName;
	}

	PCWSTR FASTCALL GetName()
	{
		return m_FileName.c_str();
	}

	NTSTATUS FASTCALL Unpack(PVOID UserData)
	{
		NTSTATUS          Status;
		NtFileDisk        File, Writer;
		PDecodeControl    Code;
		std::wstring      FileName, FullPath, FullOutDirectory;
		ULONG_PTR         Attribute;
		StreamWriter      Stream;
		WCHAR             ExeDirectory[MAX_PATH];
		BOOL              NeedSave;

		Code = (PDecodeControl)UserData;
		Status = File.Open(m_FileName.c_str());
		if (NT_FAILED(Status))
			return Status;

		RtlZeroMemory(ExeDirectory, sizeof(ExeDirectory));
		Nt_GetExeDirectory(ExeDirectory, countof(ExeDirectory));

		static WCHAR OutDirectory[] = L"__Unpack__\\NWA\\";

		FullOutDirectory = ExeDirectory + std::wstring(OutDirectory);
		Attribute = GetFileAttributesW(FullOutDirectory.c_str());
		if (Attribute == 0xffffffff)
			SHCreateDirectory(NULL, FullOutDirectory.c_str());

		FileName = GetFileName(m_FileName);
		FullPath = FullOutDirectory + FileName;

		NeedSave = TRUE;
		switch (Code->NWAFlag)
		{
		default:
		case NWA_WAV:
			ConvertNwaToWav(File, Stream);
			FullPath = ReplaceFileNameExtension(FullPath, L".wav");
			break;

		case NWA_FLAC:
			NeedSave = FALSE;
			FullPath = ReplaceFileNameExtension(FullPath, L".flac");
			ConvertNwaToFlac(File, FullPath.c_str());
			break;

		case NWA_VORBIS:
			NeedSave = FALSE;
			FullPath = ReplaceFileNameExtension(FullPath, L".ogg");
			ConvertNwaToVorbis(File, FullPath.c_str());
			break;
		}

		File.Close();

		if (!NeedSave)
			return STATUS_SUCCESS;

		Status = Writer.Create(FullPath.c_str());
		if (NT_FAILED(Status))
			return Status;

		Writer.Write(Stream.GetBuffer(), Stream.GetSize());
		Writer.Close();
		return STATUS_SUCCESS;
	}
};