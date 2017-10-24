#pragma once

#include "my.h"
#include "iUnpackObject.h"
#include "DecodeControl.h"
#include "Tool.h"
#include <ShlObj.h>
#include <string>
#include "Compression.h"
#include "PckCommon.h"



class UnpackScene : public iUnpackObject
{
	std::wstring m_FileName;

public:
	UnpackScene(){};
	~UnpackScene(){};

	Void FASTCALL SetFile(LPCWSTR FileName)
	{
		m_FileName = FileName;
	}

	NTSTATUS FASTCALL Unpack(PVOID UserData)
	{
		NTSTATUS          Status;
		NtFileDisk        File;
		PDecodeControl    Code;
		PBYTE             Buffer, CorrentBuffer, DecompBuffer;
		ULONG_PTR         Size, Attribute;
		DWORD             CompLen, DecompLen;
		std::wstring      FileName, FullPath, FullOutDirectory;
		WCHAR             ExeDirectory[MAX_PATH];
		WCHAR             ScriptFileName[MAX_PATH];
		WCHAR             OutScriptFileName[MAX_PATH];


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

		static WCHAR OutDirectory[] = L"__Unpack__\\Scene\\";

		FullOutDirectory = ExeDirectory + std::wstring(OutDirectory);
		Attribute = Nt_GetFileAttributes(FullOutDirectory.c_str());
		if (Attribute == 0xffffffff)
			SHCreateDirectory(NULL, FullOutDirectory.c_str());

		SCENEHEADER* header = (SCENEHEADER*)Buffer;
		HEADERPAIR* SceneNameLength = (HEADERPAIR*)&Buffer[header->SceneNameIndex.offset];
		PWSTR SceneNameString = (PWSTR)&Buffer[header->SceneName.offset];
		HEADERPAIR* SceneDataInfo = (HEADERPAIR*)&Buffer[header->SceneInfo.offset];
		byte* SceneData = (byte*)&Buffer[header->SceneData.offset];

		for (DWORD i = 0; i<header->SceneNameIndex.count; i++)
		{
			NtFileDisk Writer;

			compress_file_header_t compress_info;
			RtlZeroMemory(ScriptFileName, sizeof(ScriptFileName));
			RtlZeroMemory(OutScriptFileName, sizeof(OutScriptFileName));
			RtlCopyMemory(ScriptFileName, &SceneNameString[SceneNameLength[i].offset], SceneNameLength[i].count * 2);

			FormatStringW(OutScriptFileName, L"%d.%s.ss", i, ScriptFileName);

			byte* data = &SceneData[SceneDataInfo[i].offset];

			if (Code->PckNeedKey)
				decrypt_1(data, SceneDataInfo[i].count, Code->PrivateKey);

			decrypt_2(data, SceneDataInfo[i].count);

			RtlCopyMemory(&compress_info, data, sizeof(compress_file_header_t));

			byte* decompress_buf = (byte*)AllocateMemoryP(compress_info.decomp_size);
			byte* decompress_end = decompress_buf + compress_info.decomp_size;

			RtlZeroMemory(decompress_buf, compress_info.decomp_size);

			decompress_data(data + 8, decompress_buf, decompress_end);

			FullPath = FullOutDirectory + OutScriptFileName;
			Status = Writer.Create(FullPath.c_str());
			if (NT_FAILED(Status))
			{
				FreeMemoryP(decompress_buf);
				continue;
			}
			Writer.Write(decompress_buf, compress_info.decomp_size);
			Writer.Close();

			ExtractText(decompress_buf, FullPath, Code);

			FreeMemoryP(decompress_buf);

		}
		FreeMemoryP(Buffer);
		return STATUS_SUCCESS;
	}

private:

	void decrypt_1(byte* debuf, size_t desize, PBYTE key)
	{
		size_t key_idx = 0;
		size_t xor_idx = 0;

		for (xor_idx = 0; xor_idx<desize; xor_idx++, key_idx++, key_idx = key_idx & 0x8000000F)
		{
			debuf[xor_idx] ^= key[key_idx];
		}
	}

	void decrypt_2(byte* debuf, size_t desize)
	{
		size_t key_idx = 0;
		size_t xor_idx = 0;

		static byte key[] =
		{
			0x70, 0xF8, 0xA6, 0xB0, 0xA1, 0xA5, 0x28, 0x4F, 0xB5, 0x2F, 0x48, 0xFA, 0xE1, 0xE9, 0x4B, 0xDE,
			0xB7, 0x4F, 0x62, 0x95, 0x8B, 0xE0, 0x03, 0x80, 0xE7, 0xCF, 0x0F, 0x6B, 0x92, 0x01, 0xEB, 0xF8,
			0xA2, 0x88, 0xCE, 0x63, 0x04, 0x38, 0xD2, 0x6D, 0x8C, 0xD2, 0x88, 0x76, 0xA7, 0x92, 0x71, 0x8F,
			0x4E, 0xB6, 0x8D, 0x01, 0x79, 0x88, 0x83, 0x0A, 0xF9, 0xE9, 0x2C, 0xDB, 0x67, 0xDB, 0x91, 0x14,
			0xD5, 0x9A, 0x4E, 0x79, 0x17, 0x23, 0x08, 0x96, 0x0E, 0x1D, 0x15, 0xF9, 0xA5, 0xA0, 0x6F, 0x58,
			0x17, 0xC8, 0xA9, 0x46, 0xDA, 0x22, 0xFF, 0xFD, 0x87, 0x12, 0x42, 0xFB, 0xA9, 0xB8, 0x67, 0x6C,
			0x91, 0x67, 0x64, 0xF9, 0xD1, 0x1E, 0xE4, 0x50, 0x64, 0x6F, 0xF2, 0x0B, 0xDE, 0x40, 0xE7, 0x47,
			0xF1, 0x03, 0xCC, 0x2A, 0xAD, 0x7F, 0x34, 0x21, 0xA0, 0x64, 0x26, 0x98, 0x6C, 0xED, 0x69, 0xF4,
			0xB5, 0x23, 0x08, 0x6E, 0x7D, 0x92, 0xF6, 0xEB, 0x93, 0xF0, 0x7A, 0x89, 0x5E, 0xF9, 0xF8, 0x7A,
			0xAF, 0xE8, 0xA9, 0x48, 0xC2, 0xAC, 0x11, 0x6B, 0x2B, 0x33, 0xA7, 0x40, 0x0D, 0xDC, 0x7D, 0xA7,
			0x5B, 0xCF, 0xC8, 0x31, 0xD1, 0x77, 0x52, 0x8D, 0x82, 0xAC, 0x41, 0xB8, 0x73, 0xA5, 0x4F, 0x26,
			0x7C, 0x0F, 0x39, 0xDA, 0x5B, 0x37, 0x4A, 0xDE, 0xA4, 0x49, 0x0B, 0x7C, 0x17, 0xA3, 0x43, 0xAE,
			0x77, 0x06, 0x64, 0x73, 0xC0, 0x43, 0xA3, 0x18, 0x5A, 0x0F, 0x9F, 0x02, 0x4C, 0x7E, 0x8B, 0x01,
			0x9F, 0x2D, 0xAE, 0x72, 0x54, 0x13, 0xFF, 0x96, 0xAE, 0x0B, 0x34, 0x58, 0xCF, 0xE3, 0x00, 0x78,
			0xBE, 0xE3, 0xF5, 0x61, 0xE4, 0x87, 0x7C, 0xFC, 0x80, 0xAF, 0xC4, 0x8D, 0x46, 0x3A, 0x5D, 0xD0,
			0x36, 0xBC, 0xE5, 0x60, 0x77, 0x68, 0x08, 0x4F, 0xBB, 0xAB, 0xE2, 0x78, 0x07, 0xE8, 0x73, 0xBF,
			0xD8, 0x29, 0xB9, 0x16, 0x3D, 0x1A, 0x76, 0xD0, 0x87, 0x9B, 0x2D, 0x0C, 0x7B, 0xD1, 0xA9, 0x19,
			0x22, 0x9F, 0x91, 0x73, 0x6A, 0x35, 0xB1, 0x7E, 0xD1, 0xB5, 0xE7, 0xE6, 0xD5, 0xF5, 0x06, 0xD6,
			0xBA, 0xBF, 0xF3, 0x45, 0x3F, 0xF1, 0x61, 0xDD, 0x4C, 0x67, 0x6A, 0x6F, 0x74, 0xEC, 0x7A, 0x6F,
			0x26, 0x74, 0x0E, 0xDB, 0x27, 0x4C, 0xA5, 0xF1, 0x0E, 0x2D, 0x70, 0xC4, 0x40, 0x5D, 0x4F, 0xDA
		};
		for (xor_idx = 0; xor_idx<desize; xor_idx++, key_idx++, key_idx = key_idx & 0x800000FF)
		{
			debuf[xor_idx] ^= key[key_idx];
		}
	}


	void decompress_data(byte* comp_data, byte* decomp_begin, byte* decomp_end)
	{
		__asm
		{
			mov esi, comp_data
				mov edi, decomp_begin
				xor edx, edx
				cld
			Loop1 :
			mov dl, byte ptr[esi]
				inc esi
				mov dh, 0x8
			Loop2 :
				  cmp edi, decomp_end
				  je End
				  test dl, 1
				  je DecompTag
				  movsb
				  jmp DecompTag2
			  DecompTag :
			xor eax, eax
				lods word ptr[esi]
				mov ecx, eax
				shr eax, 4
				and ecx, 0xF
				add ecx, 0x2
				mov ebx, esi
				mov esi, edi
				sub esi, eax
				; rep movsb
				rep movs byte ptr[edi], byte ptr[esi]
				mov esi, ebx
			DecompTag2 :
			shr dl, 1
				dec dh
				jnz Loop2
				jmp Loop1

			End :
		}
	}

	void decrypt_string(wchar_t* str_buf, wchar_t* new_buf, int length, int key)
	{
		key *= 0x7087;
		for (int i = 0; i<length; i++)
		{
			new_buf[i] = str_buf[i] ^ key;
		}
	}

	void ExtractText(PBYTE Buffer, std::wstring FileName, PDecodeControl Code)
	{
		NTSTATUS   Status;
		NtFileDisk Writer;

		FileName += L".txt";
		SCENEHEADERV2 *sce_header = (SCENEHEADERV2*)Buffer;
		PFILE_INFO string_index = (PFILE_INFO)&Buffer[sce_header->string_index_pair.offset];
		PWSTR string_data = (PWSTR)&Buffer[sce_header->string_data_pair.offset];

		Status = Writer.Create(FileName.c_str());
		if (NT_FAILED(Status))
			return;
		
		Writer.Write("\xFF\xFE", 2);
		for (DWORD x = 0; x<sce_header->string_index_pair.count; x++)
		{
			PFILE_INFO info = &string_index[x];
			wchar_t* info_str = &string_data[info->offset];
			wchar_t* new_str = (wchar_t*)AllocateMemoryP(info->length * 4);
			RtlZeroMemory(new_str, sizeof(wchar_t) * info->length * 2);

			PrintConsole(L"%d\n", Code->SSDecode);
			if (Code->SSDecode == SSDecode::SS_V2)
				decrypt_string(info_str, new_str, info->length, x);
			else
				RtlCopyMemory(new_str, info_str, info->length * 2);

			for (int i = 0; i<info->length; i++)
			{
				Writer.Write(&new_str[i], 2);
			}
			Writer.Write(L"\r\n", 4);
			FreeMemoryP(new_str);
		}
		Writer.Close();
	}
};