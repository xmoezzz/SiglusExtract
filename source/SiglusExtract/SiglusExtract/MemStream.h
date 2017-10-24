#pragma once

#include "my.h"

struct MemStream
{
	BYTE* start;
	BYTE* cur;
	DWORD len;
};

