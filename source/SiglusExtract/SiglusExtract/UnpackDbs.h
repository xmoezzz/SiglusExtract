#pragma once

#include <string>
#include "iUnpackObject.h"
#include "Tool.h"
#include <my.h>
#include <memory>
#include <ShlObj.h>
#include "Compression.h"

class UnpackDbs : public iUnpackObject
{
	std::wstring m_FileName;


public:
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
		NtFileDisk        Reader, Writer;
		ULONG             Size;
		ULONG             AlignedSize;
		ULONG             UncompressedSize;
		ULONG             CompressedSize;
		ULONG             Line, Row;
		DWORD             Attribute;
		std::wstring      FullPath, FullOutDirectory, FileName;
		WCHAR             ExeDirectory[MAX_PATH];

		static ULONG StatusKey2[] =
		{
			0x7190c70e, 0x499bf135, 0x499bf135, 0x7190c70e, 0x7190c70e,
			0x499bf135, 0x499bf135, 0x7190c70e, 0x7190c70e, 0x499bf135,
			0x7190c70e, 0x7190c70e, 0x7190c70e, 0x499bf135, 0x7190c70e,
			0x499bf135, 0x499bf135, 0x7190c70e, 0x499bf135, 0x499bf135,
			0x499bf135, 0x499bf135, 0x499bf135, 0x7190c70e, 0x7190c70e
		};

		Status = Reader.Open(m_FileName.c_str());
		if (NT_FAILED(Status))
			return Status;

		Size = Reader.GetSize32();
		if (Size <= 12)
			return STATUS_BUFFER_TOO_SMALL;

		auto Buffer = AllocateMemorySafe<BYTE>(Size);
		if (!Buffer)
			return Status;

		Status = Reader.Read(Buffer.get(), Size);
		if (NT_FAILED(Status))
			return Status;

		AlignedSize = (Size - 4) / 4;
		for (ULONG i = 0; i < AlignedSize; ++i) {
			((PULONG)Buffer.get())[i + 1] ^= 0x89f4622d;
		}

		CompressedSize = *(PULONG)(Buffer.get() + 4) - 8;
		UncompressedSize = *(PULONG)(Buffer.get() + 8);

		if (UncompressedSize == 0) {
			UncompressedSize = CompressedSize;
		}

		if (CompressedSize > Size)
			return STATUS_BUFFER_OVERFLOW;

		auto UncompressedBuffer = AllocateMemorySafe<BYTE>(UncompressedSize);
		if (!UncompressedBuffer)
			return STATUS_NO_MEMORY;

		if (UncompressedSize != CompressedSize) {
			DecompressData(Buffer.get() + 12, UncompressedBuffer.get(), UncompressedSize);
		}
		else {
			RtlCopyMemory(UncompressedBuffer.get(), Buffer.get() + 12, UncompressedSize);
		}

		AlignedSize = UncompressedSize / 4;
		Line = 0;
		Row  = 0;

		for (ULONG i = 0; i < AlignedSize; ++i)
		{
			((PULONG)UncompressedBuffer.get())[i] ^= StatusKey2[Line * 5 + Row % 5];
			Row++;

			if (Row >= 16) 
			{ 
				Line++; 
				Row = 0; 
			}

			if (Line >= 5) {
				Line = 0;
			}
		}

		RtlZeroMemory(ExeDirectory, sizeof(ExeDirectory));
		Nt_GetExeDirectory(ExeDirectory, countof(ExeDirectory));

		static WCHAR OutDirectory[] = L"__Unpack__\\DBS\\";

		FullOutDirectory = ExeDirectory + std::wstring(OutDirectory);
		Attribute = GetFileAttributesW(FullOutDirectory.c_str());
		if (Attribute == 0xffffffff) {
			SHCreateDirectory(NULL, FullOutDirectory.c_str());
		}

		FileName = GetFileName(m_FileName);
		FullPath = FullOutDirectory + FileName;


		Status = Writer.Create(FullPath.c_str());
		if (NT_FAILED(Status))
			return Status;

		Status = Writer.Write(UncompressedBuffer.get(), UncompressedSize);
		if (NT_FAILED(Status))
			return Status;

		return Writer.Close();
	}

private:

	template <class T> inline std::shared_ptr<T> AllocateMemorySafe(SIZE_T Size)
	{
		return std::shared_ptr<T>(
			(T*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size),
			[](T* Ptr)
		{
			if (Ptr) {
				HeapFree(GetProcessHeap(), 0, Ptr);
			}
		});
	}

};

