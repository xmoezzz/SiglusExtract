#pragma once

#include "my.h"
#include "StreamWriter.h"

void ConvertNwaToWav(NtFileDisk& in, StreamWriter& out, int skip_count = 0, int in_size = -1);
void ConvertNwaToFlac(NtFileDisk& in, StreamWriter& out);
void ConvertNwaToFlac(NtFileDisk& in, LPCWSTR FileName);
void ConvertNwaToVorbis(NtFileDisk& in, LPCWSTR FileName);

void ConvertNwaToFlacInternalV2(NtFileMemory& in, LPCWSTR FileName);
void ConvertNwaToVorbisInternal(NtFileMemory& in, LPCWSTR FileName);