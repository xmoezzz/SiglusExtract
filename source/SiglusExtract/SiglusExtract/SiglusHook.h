#pragma once


#include "SiglusExtractDialog.h"
#include "SiglusExtractInitDialog.h"
#include <my.h>
#include <vector>

#define PAGE_SIZE 0x1000

class SiglusHook
{
public:
	SiglusHook();
	~SiglusHook();

	FORCEINLINE Void SetDllModule(HMODULE hModule) { DllModule = hModule; }
	FORCEINLINE Void SetExeModule(HMODULE hModule) { ExeModule = hModule; }
	Void UnInit();
	BOOL InitWindow();
	BOOL DetactNeedDumper();

public:
	PVOID ExeModule, DllModule;
	PBYTE ImageBuffer, ImageBufferEx;
	ULONG ImageSize, ImageSizeEx;
	std::vector<BYTE> ExtBuffer;
	HANDLE GuiHandle;
	CSiglusExtractDialog Dialog;
	CSiglusExtractInitDialog InitDialog;
	PVOID               ExtModuleHandle;

	std::wstring        SceneName, GameexeName, DllPath;

	BOOL                ExtraDecInPck;
	BOOL                ExtraDecInDat;
	BOOL                DisablePrivateKey;
	BYTE                GameexeBytes[24];

public:
	API_POINTER(CreateFileW) StubCreateFileW;
	API_POINTER(ReadFile)    StubReadFile;
	HANDLE                   GameexeHandle;
	PBYTE                    AccessPtr;
	BOOL                     InitKey;
	BOOL                     ExInit;
};

SiglusHook* GetSiglusHook();
