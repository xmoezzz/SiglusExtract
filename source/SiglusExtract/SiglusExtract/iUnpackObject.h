#pragma once

#include "my.h"

class iUnpackObject
{
public:
	virtual VOID     FASTCALL SetFile(LPCWSTR FileName) = 0;
	virtual NTSTATUS FASTCALL Unpack(PVOID UserData) = 0;
};

