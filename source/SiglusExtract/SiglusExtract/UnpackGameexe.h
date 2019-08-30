#pragma once

#include "my.h"
#include "iUnpackObject.h"
#include "DecodeControl.h"
#include "Tool.h"
#include <ShlObj.h>
#include <string>
#include "Compression.h"



class UnpackGameexe : public iUnpackObject
{
	std::wstring m_FileName;

public:
	UnpackGameexe(){}
	~UnpackGameexe(){};

	static BYTE GameExeKey[256];

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
		PBYTE             Buffer, CorrentBuffer, DecompBuffer;
		ULONG_PTR         Size, Attribute;
		DWORD             CompLen, DecompLen;
		std::wstring      FileName, FullPath, FullOutDirectory;
		WCHAR             ExeDirectory[MAX_PATH];

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

		CorrentBuffer = Buffer + 8;

		for (ULONG i = 0; i < Size - 8; i++)
		{
			if (Code->DatNeedKey)
			{
				CorrentBuffer[i] ^= Code->PrivateKey[i & 0x0f];
			}

			CorrentBuffer[i] ^= GameExeKey[i & 0xff];
		}

		CompLen = *(PDWORD)(CorrentBuffer);
		DecompLen = *(PDWORD)(CorrentBuffer + 4);
		DecompBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, DecompLen);
		if (!DecompBuffer)
		{
			PrintConsoleW(L"Failed to allocate memory for decompression (size = 0x%x)\n", DecompLen);
			if (DecompLen > 1024 * 1024 * 200)
			{
				if (!Code->IsValidKey())
					MessageBoxW(NULL,
						L"internal exception :\n"
						L"you must restart this game and try this operation angin",
						L"FATAL (Invalid key)",
						MB_OK | MB_ICONERROR
					);
				else
					MessageBoxW(NULL,
						L"internal exception :\n"
						L"you must restart this game and try this operation angin",
						L"FATAL",
						MB_OK | MB_ICONERROR
					);

			}
			return STATUS_NO_MEMORY;
		}

		DecompressData(CorrentBuffer + 8, DecompBuffer, DecompLen);

		RtlZeroMemory(ExeDirectory, sizeof(ExeDirectory));
		Nt_GetExeDirectory(ExeDirectory, countof(ExeDirectory));

		static WCHAR OutDirectory[] = L"__Unpack__\\Gameexe\\";

		FullOutDirectory = ExeDirectory + std::wstring(OutDirectory);
		Attribute = GetFileAttributesW(FullOutDirectory.c_str());
		if (Attribute == 0xffffffff)
			SHCreateDirectory(NULL, FullOutDirectory.c_str());

		FileName = GetFileName(m_FileName);
		FullPath = FullOutDirectory + FileName;

		FullPath = ReplaceFileNameExtension(FullPath, L".txt");
		Status = Writer.Create(FullPath.c_str());
		if (NT_FAILED(Status))
		{
			HeapFree(GetProcessHeap(), 0, Buffer);
			HeapFree(GetProcessHeap(), 0, DecompBuffer);
			return Status;
		}

		static BYTE Bom[] = { 0xff, 0xfe };

		Writer.Write(Bom, sizeof(Bom));
		Writer.Write(DecompBuffer, DecompLen);

		HeapFree(GetProcessHeap(), 0, Buffer);
		HeapFree(GetProcessHeap(), 0, DecompBuffer);
		Writer.Close();
		
		return STATUS_SUCCESS;
	}
};