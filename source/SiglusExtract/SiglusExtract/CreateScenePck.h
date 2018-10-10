#pragma once

#include "my.h"

NTSTATUS FASTCALL CreateScenePck(HWND hParent, PBYTE PrivateKey, LPCWSTR Path, LPCWSTR OutputFileName, BOOL NeedExtraKey);
