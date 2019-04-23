#pragma once

#include "my.h"
#include "iUnpackObject.h"
#include "DecodeControl.h"
#include "Tool.h"
#include <ShlObj.h>
#include <string>

#pragma comment(lib, "Shell32.lib")

class UnpackOGG : public iUnpackObject
{
	std::wstring m_FileName;

public:
	UnpackOGG(){};
	~UnpackOGG(){};

	VOID     FASTCALL SetFile(LPCWSTR FileName)
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

		BYTE Mask = 'O';

		Code = (PDecodeControl)UserData;
		Status = File.Open(m_FileName.c_str());
		if (NT_FAILED(Status))
			return Status;

		Size = File.GetSize32();
		Buffer = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
		if (!Buffer)
		{
			File.Close();
			return STATUS_NO_MEMORY;
		}

		File.Read(Buffer, Size);
		File.Close();

		Mask ^= Buffer[0];
		
		switch (Code->OGGFlag)
		{
		case OGG_DECODE:
			for (SIZE_T i = 0; i < Size; i++)
				Buffer[i] ^= Mask;

			break;
		default:
			break;
		}

		RtlZeroMemory(ExeDirectory, sizeof(ExeDirectory));
		Nt_GetExeDirectory(ExeDirectory, countof(ExeDirectory));

		static WCHAR OutDirectory[] = L"__Unpack__\\OWP\\";

		FullOutDirectory = ExeDirectory + std::wstring(OutDirectory);
		Attribute = GetFileAttributesW(FullOutDirectory.c_str());
		if (Attribute == 0xffffffff)
			SHCreateDirectory(NULL, FullOutDirectory.c_str());

		FileName = GetFileName(m_FileName);
		FullPath = FullOutDirectory + FileName;

		FullPath = ReplaceFileNameExtension(FullPath, L".ogg");
		Status = Writer.Create(FullPath.c_str());
		if (NT_FAILED(Status))
		{
			HeapFree(GetProcessHeap(), 0, Buffer);
			return Status;
		}

		Writer.Write(Buffer, Size);
		Writer.Close();
		HeapFree(GetProcessHeap(), 0, Buffer);
		return STATUS_SUCCESS;
	}
};

