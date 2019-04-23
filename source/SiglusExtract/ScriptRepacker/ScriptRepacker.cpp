#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <vector>
#include <windows.h>
#include <fstream>
#include <iostream>
#include <string>
#include <my.h>

using namespace std;
typedef unsigned char byte;
unsigned char* buf;
typedef unsigned int ulong;

typedef struct string_table_s
{
	wchar_t* old_string;
	wchar_t* new_string;
}string_table_t;

typedef struct scene_file_info_s
{
	ulong offset;
	ulong length;
} scene_file_info_t;

typedef struct header_pair_s
{
	ulong offset;
	ulong count;
}header_pair_t;

typedef struct scene_header_s
{
	ulong headerLength;
	header_pair_t v1;
	header_pair_t string_index_pair;
	header_pair_t string_data_pair;
	header_pair_t v4;
	header_pair_t v5;
	header_pair_t v6;
	header_pair_t v7;
	header_pair_t v8;
	header_pair_t v9;
	header_pair_t v10;
	header_pair_t v11;
	header_pair_t v12;
	header_pair_t v13;
	header_pair_t v14;
	header_pair_t v15;
	header_pair_t v16;
}scene_header_t;

vector <string_table_t*> string_table;

void decrypt_string(wchar_t* str_buf, wchar_t* new_buf, int length, int key)
{
	key *= 0x7087;
	for (int i = 0; i<length; i++)
	{
		new_buf[i] = str_buf[i] ^ key;
	}
}


int wmain(int argc, wchar_t* argv[])
{
	if (argc < 3)
	{
		wprintf(L"Usage : %s Script Text\n", argv[0]);
		return 0;
	}

	NtFileDisk File;

	auto Status = File.Open(argv[2]);
	if (NT_FAILED(Status))
		return 0;

	auto Size = File.GetSize32();
	if (Size >= 2)
	{
		Size = Size - 2;
	}
	else
	{
		PrintConsoleW(L"Invalid file size..\n");
		File.Close();
		return 0;
	}

	File.Seek(2, FILE_BEGIN); //ship bom
	auto Buffer = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ROUND_UP(Size + 1, 4));
	if (!Buffer)
	{
		File.Close();
		return 0;
	}

	File.Read(Buffer, Size);
	File.Close();

	std::vector<std::wstring> ReadLine;

	auto GetLines = [](std::vector<std::wstring>& ReadPool, PWCHAR Buffer, ULONG Size)->BOOL
	{
		std::wstring ReadLine;
		ULONG        iPos = 0;
		ReadLine.clear();

		while (true)
		{
			if (iPos >= Size)
				break;

			if (Buffer[iPos] == L'\r')
			{
				ReadPool.push_back(ReadLine);
				ReadLine.clear();
				iPos++;

				if (Buffer[iPos] == L'\n')
					iPos++;
			}

			if (Buffer[iPos] == L'\n')
			{
				ReadPool.push_back(ReadLine);
				ReadLine.clear();
				iPos++;
			}

			ReadLine += Buffer[iPos];
			iPos++;
		}

		if (ReadLine.length())
			ReadPool.push_back(ReadLine);

		return TRUE;
	};

	GetLines(ReadLine, (PWCHAR)Buffer, Size);
	if (Buffer)
		HeapFree(GetProcessHeap(), 0, Buffer);

	Buffer = NULL;

	auto Lines = ReadLine;
	
	FILE* script = _wfopen(argv[1], L"rb");
	fseek(script, 0, SEEK_END);
	size_t length = ftell(script);
	fseek(script, 0, SEEK_SET);

	byte* script_buf = new byte[length];
	fread(script_buf, length, 1, script);
	fclose(script);

	scene_header_t * header = (scene_header_t*)script_buf;

	scene_file_info_t* string_index = (scene_file_info_t*)&script_buf[header->string_index_pair.offset];
	wchar_t* string_data = (wchar_t*)&script_buf[header->string_data_pair.offset];
	for (ulong x = 0; x<header->string_index_pair.count; x++)
	{
		scene_file_info_t* info = &string_index[x];
		wchar_t* info_str = &string_data[info->offset];
		wchar_t* new_str = new wchar_t[info->length + sizeof(wchar_t)];
		memset(new_str, 0, info->length * sizeof(wchar_t) + sizeof(wchar_t));

		decrypt_string(info_str, new_str, info->length, x);
		string_table_t *item = new string_table_t;
		item->old_string = new_str;
		item->new_string = 0;
		string_table.push_back(item);
	}

	int index = 0;

	for (auto& Line : Lines)
	{
		if (index < string_table.size())
		{
			string_table[index]->new_string = new wchar_t[Line.length() + 1];
			RtlZeroMemory(string_table[index]->new_string, (Line.length() + 1) * 2);
			wcscpy(string_table[index]->new_string, Line.c_str());
		}
		else
		{
			printf("Index error(to large)\n");
			return 0;
		}
		index++;
	}

	WCHAR tmp_buffer[2048] = {0};
	byte* buffer = (byte*)malloc(0);
	size_t offsets = 0;
	for (size_t x = 0; x<string_table.size(); x++)
	{
		RtlZeroMemory(tmp_buffer, sizeof(tmp_buffer));
		scene_file_info_t* info = &string_index[x];
		if (string_table[x]->new_string)
		{
			wcscpy(tmp_buffer, string_table[x]->new_string);
		}
		else
		{
			wcscpy(tmp_buffer, string_table[x]->old_string);
		}
		size_t len = wcslen(tmp_buffer) * 2;
		string_index[x].offset = offsets / sizeof(wchar_t);
		string_index[x].length = len / 2;
		buffer = (byte*)realloc(buffer, offsets + len);
		decrypt_string(tmp_buffer, (wchar_t*)(&buffer[offsets]), len / 2, x);
		offsets += len;
	}
	header->string_data_pair.offset = length;
	script_buf = (byte*)realloc(script_buf, length + offsets);
	memcpy(&script_buf[length], buffer, offsets);
	WCHAR newName[260];
	wsprintfW(newName, L"%s.out", argv[1]);
	FILE* f = _wfopen(newName, L"wb");
	fwrite(script_buf, length + offsets, 1, f);
	fclose(f);
	return 0;
}
