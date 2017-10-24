#pragma once

#include "my.h"


typedef struct FILE_INFO{
	DWORD offset;
	DWORD length;
} FILE_INFO, *PFILE_INFO;

typedef struct HEADERPAIR{
	int offset;
	int count;
}HEADERPAIR;

typedef struct ss_header{
	DWORD szZiped;
	DWORD szOrigianl;
}ss_header;

typedef struct SCENEHEADER{
	int headerLength;
	HEADERPAIR varInfo;
	HEADERPAIR varNameIndex;
	HEADERPAIR varName;
	HEADERPAIR cmdInfo;
	HEADERPAIR cmdNameIndex;
	HEADERPAIR cmdName;
	HEADERPAIR SceneNameIndex;
	HEADERPAIR SceneName;
	HEADERPAIR SceneInfo;
	HEADERPAIR SceneData;
	int ExtraKeyUse;
	int SourceHeaderLength;
}SCENEHEADER;


typedef struct SCENEHEADERV2{
	int headerLength;
	HEADERPAIR v1;
	HEADERPAIR string_index_pair;
	HEADERPAIR string_data_pair;
	HEADERPAIR v4;
	HEADERPAIR v5;
	HEADERPAIR v6;
	HEADERPAIR v7;
	HEADERPAIR v8;
	HEADERPAIR v9;
	HEADERPAIR v10;
	HEADERPAIR v11;
	HEADERPAIR v12;
	HEADERPAIR v13;
	HEADERPAIR v14;
	HEADERPAIR v15;
	HEADERPAIR v16;
}SCENEHEADERV2;
typedef struct file_offset_info_s
{
	DWORD offsets;
	DWORD sizes;
}file_offset_info_t;


typedef struct compress_file_header_s
{
	DWORD comp_size;
	DWORD decomp_size;
}compress_file_header_t;

