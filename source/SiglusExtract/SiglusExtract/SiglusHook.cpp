#include "SiglusHook.h"
#include "MyHook.h"
#include <gdiplus.h>  
#include <Psapi.h>
#include "mt64.h"
#include <vector>
#include "SiglusPatch.h"
#include "resource.h"
#include "PckCommon.h"
#include <ImageHlp.h>
#include <fstream>
#include <string>

//#pragma comment(linker, "/SECTION:.text,ERW /MERGE:.rdata=.text /MERGE:.data=.text")
//#pragma comment(linker, "/SECTION:.Xmoe,ERW /MERGE:.text=.Xmoe")

#pragma comment(lib, "gdiplus")
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "Imagehlp.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "Version.lib")
#pragma comment(lib, "ntdll.lib")

static SiglusHook* g_Hook = NULL;

SiglusHook* GetSiglusHook()
{
	LOOP_ONCE
	{
		if (g_Hook)
		break;

		if (!g_Hook)
			g_Hook = new SiglusHook;

		if (g_Hook)
			break;

		MessageBoxW(NULL, L"Insufficient memory", L"SiglusExtract", MB_OK | MB_ICONERROR);
		Ps::ExitProcess(STATUS_NO_MEMORY);
	}
	return g_Hook;
}

void NTAPI ExecuteDumper(DWORD Pid, PBYTE* Buffer, ULONG* Size, PVOID(NTAPI *Allocater)(ULONG Size));

PVOID NTAPI LocalAllocater(ULONG Size)
{
	return AllocateMemoryP(Size);
}



SiglusHook::SiglusHook() :
ExeModule(NULL),
DllModule(NULL),
GameexeHandle(INVALID_HANDLE_VALUE),
StubReadFile(NULL),
StubCreateFileW(NULL),
ImageSize(0),
ImageBuffer(NULL),
ImageSizeEx(0),
ImageBufferEx(NULL),
InitKey(FALSE),
ExtraDecInPck(TRUE),
ExtraDecInDat(TRUE),
GuiHandle(INVALID_HANDLE_VALUE)
{
	std::fstream Override(L"Override.ini");
	std::string  Scene, Gameexe;
	WCHAR        WScene[MAX_PATH];
	WCHAR        WGameexe[MAX_PATH];
	NtFileDisk   File;
	NTSTATUS     Status;
	
	SceneName   = L"Scene.pck";
	GameexeName = L"Gameexe.dat";


	RtlZeroMemory(WScene,   sizeof(WScene));
	RtlZeroMemory(WGameexe, sizeof(WGameexe));

	auto FileIsExist = [](LPCWSTR FileName)->BOOL
	{
		DWORD Attribute = Nt_GetFileAttributes(FileName);
		return (Attribute != 0xFFFFFFFF) && !(Attribute & FILE_ATTRIBUTE_DIRECTORY);
	};

	LOOP_ONCE
	{
		if (!Override.is_open())
			break;

		if (!getline(Override, Scene))
			break;
		
		if (!getline(Override, Gameexe))
			break;

		Override.close();
		MultiByteToWideChar(0, 0, Scene.c_str(), Scene.length(), WScene, countof(WScene) - 1);
		MultiByteToWideChar(0, 0, Gameexe.c_str(), Gameexe.length(), WGameexe, countof(WGameexe) - 1);

		if (!(FileIsExist(WScene) && FileIsExist(WGameexe)))
			break;

		//too lazy to check file...
		SceneName   = WScene;
		GameexeName = WGameexe;
	}
}

SiglusHook::~SiglusHook()
{
}


Void SiglusHook::UnInit()
{
	FreeMemoryP(ImageBuffer);
}

DWORD NTAPI GuiWorkerThread(PVOID UserData)
{
	SiglusHook* Hook = (SiglusHook*)UserData;
	Hook->Dialog.DoModal();
	return 0;
}


DWORD NTAPI DelayDumperThreadGUI(LPVOID _This)
{
	SiglusHook*         This;


	This = (SiglusHook*)_This;

	This->InitDialog.Create(IDD_INIT_DIALOG);
	This->InitDialog.Initialize();
	This->InitDialog.ShowWindow(SW_SHOW);

	while (This->ExInit != TRUE)
	{
		Ps::Sleep(10);
	}

	This->InitDialog.EndDialog(0);

	return 0;
}

HANDLE NTAPI HookCreateFileW(
	LPCWSTR lpFileName,
	DWORD   dwDesiredAccess,
	DWORD   dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD  dwCreationDisposition,
	DWORD  dwFlagsAndAttributes,
	HANDLE hTemplateFile
	)
{
	NTSTATUS                 Status;
	HANDLE                   Handle;
	SiglusHook*              Data;
	SIZE_T                   ByteTransferred;

	Data = GetSiglusHook();

	auto IsScenePack = [&](LPCWSTR FileName)->BOOL
	{
		LONG_PTR iPos = 0;

		for (LONG_PTR i = 0; i < StrLengthW(FileName); i++)
		{
			if (FileName[i] == L'\\' || FileName[i] == L'/')
				iPos = i;
		}

		if (iPos != 0)
			iPos++;

		return StrCompareW(FileName + iPos, Data->SceneName.c_str()) == 0;
	};


	auto IsGameExe = [&](LPCWSTR FileName)->BOOL
	{
		LONG_PTR iPos = 0;

		for (LONG_PTR i = 0; i < StrLengthW(FileName); i++)
		{
			if (FileName[i] == L'\\' || FileName[i] == L'/')
				iPos = i;
		}

		if (iPos != 0)
			iPos++;

		return StrCompareW(FileName + iPos, Data->GameexeName.c_str()) == 0;
	};


	auto IsMainExe = []()->BOOL
	{
		MODULEINFO  Info;
		GetModuleInformation(GetCurrentProcess(), (HMODULE)Nt_GetExeModuleHandle(), &Info, sizeof(Info));

		if (((PBYTE)_ReturnAddress() > (PBYTE)Info.lpBaseOfDll) &&
			((PBYTE)_ReturnAddress() < (PBYTE)Info.lpBaseOfDll + Info.SizeOfImage))
		{
			return TRUE;
		}

		return FALSE;
	};


	Handle = Data->StubCreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
		lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	if (IsGameExe(lpFileName) && IsMainExe() && (!Data->InitKey))
	{
		Data->GameexeHandle = Handle;
		Data->InitKey = TRUE;
	}

	return Handle;
}

#include "hardwarebp.h"

HardwareBreakpoint m_Bp;
BOOL  InitOnce = FALSE;
PVOID ExceptionHandler = NULL;

LONG NTAPI FindPrivateKeyHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
	NTSTATUS    Status;
	SiglusHook* Data;
	DWORD       OldFlags;
	PBYTE       Buffer, AccessBuffer;
	BYTE        PrivateKey[16];

	static BYTE Key[256] =
	{
		0xD8, 0x29, 0xB9, 0x16, 0x3D, 0x1A, 0x76, 0xD0, 0x87, 0x9B, 0x2D, 0x0C, 0x7B, 0xD1, 0xA9, 0x19,
		0x22, 0x9F, 0x91, 0x73, 0x6A, 0x35, 0xB1, 0x7E, 0xD1, 0xB5, 0xE7, 0xE6, 0xD5, 0xF5, 0x06, 0xD6,
		0xBA, 0xBF, 0xF3, 0x45, 0x3F, 0xF1, 0x61, 0xDD, 0x4C, 0x67, 0x6A, 0x6F, 0x74, 0xEC, 0x7A, 0x6F,
		0x26, 0x74, 0x0E, 0xDB, 0x27, 0x4C, 0xA5, 0xF1, 0x0E, 0x2D, 0x70, 0xC4, 0x40, 0x5D, 0x4F, 0xDA,
		0x9E, 0xC5, 0x49, 0x7B, 0xBD, 0xE8, 0xDF, 0xEE, 0xCA, 0xF4, 0x92, 0xDE, 0xE4, 0x76, 0x10, 0xDD,
		0x2A, 0x52, 0xDC, 0x73, 0x4E, 0x54, 0x8C, 0x30, 0x3D, 0x9A, 0xB2, 0x9B, 0xB8, 0x93, 0x29, 0x55,
		0xFA, 0x7A, 0xC9, 0xDA, 0x10, 0x97, 0xE5, 0xB6, 0x23, 0x02, 0xDD, 0x38, 0x4C, 0x9B, 0x1F, 0x9A,
		0xD5, 0x49, 0xE9, 0x34, 0x0F, 0x28, 0x2D, 0x1B, 0x52, 0x39, 0x5C, 0x36, 0x89, 0x56, 0xA7, 0x96,
		0x14, 0xBE, 0x2E, 0xC5, 0x3E, 0x08, 0x5F, 0x47, 0xA9, 0xDF, 0x88, 0x9F, 0xD4, 0xCC, 0x69, 0x1F,
		0x30, 0x9F, 0xE7, 0xCD, 0x80, 0x45, 0xF3, 0xE7, 0x2A, 0x1D, 0x16, 0xB2, 0xF1, 0x54, 0xC8, 0x6C,
		0x2B, 0x0D, 0xD4, 0x65, 0xF7, 0xE3, 0x36, 0xD4, 0xA5, 0x3B, 0xD1, 0x79, 0x4C, 0x54, 0xF0, 0x2A,
		0xB4, 0xB2, 0x56, 0x45, 0x2E, 0xAB, 0x7B, 0x88, 0xC5, 0xFA, 0x74, 0xAD, 0x03, 0xB8, 0x9E, 0xD5,
		0xF5, 0x6F, 0xDC, 0xFA, 0x44, 0x49, 0x31, 0xF6, 0x83, 0x32, 0xFF, 0xC2, 0xB1, 0xE9, 0xE1, 0x98,
		0x3D, 0x6F, 0x31, 0x0D, 0xAC, 0xB1, 0x08, 0x83, 0x9D, 0x0D, 0x10, 0xD1, 0x41, 0xF9, 0x00, 0xBA,
		0x1A, 0xCF, 0x13, 0x71, 0xE4, 0x86, 0x21, 0x2F, 0x23, 0x65, 0xC3, 0x45, 0xA0, 0xC3, 0x92, 0x48,
		0x9D, 0xEA, 0xDD, 0x31, 0x2C, 0xE9, 0xE2, 0x10, 0x22, 0xAA, 0xE1, 0xAD, 0x2C, 0xC4, 0x2D, 0x7F
	};

	Data = GetSiglusHook();
	
	if (InitOnce == FALSE && pExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)
	{
		m_Bp.Clear(pExceptionInfo->ContextRecord);
		Buffer = &Data->GameexeBytes[8];

		AccessBuffer = Data->AccessPtr - 16;
		for (ULONG i = 0; i < 16; i++)
		{
			PrivateKey[i] = AccessBuffer[i] ^ Buffer[i];
		}

		Data->Dialog.SetPrivateKey(PrivateKey);
		Status = Nt_CreateThread(GuiWorkerThread, Data, FALSE, NtCurrentProcess(), &Data->GuiHandle);
		if (NT_FAILED(Status))
			MessageBoxW(NULL, L"Failed to open SiglusExtract's main window", L"SiglusExtract", MB_OK | MB_ICONERROR);

		InitOnce = TRUE;
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}


BOOL WINAPI HookReadFile(
	HANDLE hFile,
	LPVOID lpBuffer,
	DWORD nNumberOfBytesToRead,
	LPDWORD lpNumberOfBytesRead,
	LPOVERLAPPED lpOverlapped
	)
{
	BOOL        Result;
	DWORD       OldFlags;
	SiglusHook* Data;
	PVOID       RtlDispatchExceptionAddress;

	Data = GetSiglusHook();

	Result = Data->StubReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);

	if (hFile == Data->GameexeHandle)
	{
		Data->AccessPtr = (PBYTE)lpBuffer + 8 + 16;

		m_Bp.Set((PBYTE)lpBuffer + 8 + 16, 1, HardwareBreakpoint::Condition::Write);
		ExceptionHandler = AddVectoredExceptionHandler(1, FindPrivateKeyHandler);

		Data->GameexeHandle = INVALID_HANDLE_VALUE;
	}

	return Result;
}


int
WINAPI
HookMultiByteToWideChar(
UINT CodePage,
DWORD dwFlags,
LPCCH lpMultiByteStr,
int cbMultiByte,
LPWSTR lpWideCharStr,
int cchWideChar
)
{
	switch (CodePage)
	{
	case CP_ACP:
	case CP_OEMCP:
	case CP_THREAD_ACP:
		CodePage = 932;
		break;
	}

	return MultiByteToWideChar(CodePage,
		dwFlags,
		lpMultiByteStr,
		cbMultiByte,
		lpWideCharStr,
		cchWideChar);
}




BOOL SelfPrivilegeUp(void)
{
	BOOL   ret;
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tp;

	ret = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
	if (!ret)
	{
		return FALSE;
	}

	ret = LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);
	if (!ret)
	{
		CloseHandle(hToken);
		return FALSE;
	}
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	ret = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
	if (!ret)
	{
		CloseHandle(hToken);
		return FALSE;
	}
	CloseHandle(hToken);
	return TRUE;
}


DWORD NTAPI HookMapFileAndCheckSumA(
	_In_  PCSTR  Filename,
	_Out_ PDWORD HeaderSum,
	_Out_ PDWORD CheckSum
	)
{
	return CHECKSUM_SUCCESS;
}

API_POINTER(GetFileVersionInfoSizeW) StubGetFileVersionInfoSizeW = NULL;

DWORD
APIENTRY
HookGetFileVersionInfoSizeW(
_In_        LPCWSTR lptstrFilename, /* Filename of version stamped file */
_Out_opt_   LPDWORD lpdwHandle       /* Information for use by GetFileVersionInfo */
)
{
	auto IsKernel32 = [](LPCWSTR lpFileName)->BOOL
	{
		/*
		ULONG Length = StrLengthW(lpFileName);
		if (Length < 12)
		return FALSE;

		//sarcheck.dll
		return CHAR_UPPER4W(*(PULONG64)&lpFileName[Length - 0xC]) == TAG4W('KERN') &&
		CHAR_UPPER4W(*(PULONG64)&lpFileName[Length - 0x8]) == TAG4W('AL32') &&
		CHAR_UPPER4W(*(PULONG64)&lpFileName[Length - 0x4]) == TAG4W('.DLL');
		*/

		return wcsstr(lpFileName, L"kernel32.dll") != NULL;
	};

	if (IsKernel32(lptstrFilename))
		return StubGetFileVersionInfoSizeW(L"SiglusExtract.dll", lpdwHandle);

	return StubGetFileVersionInfoSizeW(lptstrFilename, lpdwHandle);
}


API_POINTER(GetFileVersionInfoW) StubGetFileVersionInfoW = NULL;

BOOL
APIENTRY
HookGetFileVersionInfoW(
_In_                LPCWSTR lptstrFilename, /* Filename of version stamped file */
_Reserved_          DWORD dwHandle,          /* Information from GetFileVersionSize */
_In_                DWORD dwLen,             /* Length of buffer for info */
LPVOID lpData            /* Buffer to place the data structure */
)
{
	auto IsKernel32 = [](LPCWSTR lpFileName)->BOOL
	{
		/*
		ULONG Length = StrLengthW(lpFileName);
		if (Length < 12)
		return FALSE;

		//sarcheck.dll
		return CHAR_UPPER4W(*(PULONG64)&lpFileName[Length - 0xC]) == TAG4W('KERN') &&
		CHAR_UPPER4W(*(PULONG64)&lpFileName[Length - 0x8]) == TAG4W('AL32') &&
		CHAR_UPPER4W(*(PULONG64)&lpFileName[Length - 0x4]) == TAG4W('.DLL');
		*/

		return wcsstr(lpFileName, L"kernel32.dll") != NULL;
	};

	if (IsKernel32(lptstrFilename))
		return StubGetFileVersionInfoW(L"SiglusExtract.dll", dwHandle, dwLen, lpData);

	return StubGetFileVersionInfoW(lptstrFilename, dwHandle, dwLen, lpData);
}

API_POINTER(GetTimeZoneInformation) StubGetTimeZoneInformation = NULL;

DWORD
WINAPI
HookGetTimeZoneInformation(
_Out_ LPTIME_ZONE_INFORMATION lpTimeZoneInformation
)
{
	static WCHAR StdName[] = L"TOKYO";
	static WCHAR DayName[] = L"Tokyo Daylight Time";

	StubGetTimeZoneInformation(lpTimeZoneInformation);

	lpTimeZoneInformation->Bias = -540;
	lpTimeZoneInformation->StandardBias = 0;

	RtlCopyMemory(lpTimeZoneInformation->StandardName, StdName, countof(StdName) * 2);
	RtlCopyMemory(lpTimeZoneInformation->DaylightName, DayName, countof(DayName) * 2);
	return 0;
}


API_POINTER(GetLocaleInfoW) StubGetLocaleInfoW = NULL;

int
WINAPI
HookGetLocaleInfoW(
LCID     Locale,
LCTYPE   LCType,
LPWSTR lpLCData,
int      cchData)
{
	if (Locale == 0x800u && LCType == 1)
	{
		RtlCopyMemory(lpLCData, L"0411", 10);
		return 5;
	}

	return StubGetLocaleInfoW(Locale, LCType, lpLCData, cchData);
}


API_POINTER(CreateProcessInternalW) StubCreateProcessInternalW = NULL;

BOOL
WINAPI
HookCreateProcessInternalW(
HANDLE                  hToken,
LPCWSTR                 lpApplicationName,
LPWSTR                  lpCommandLine,
LPSECURITY_ATTRIBUTES   lpProcessAttributes,
LPSECURITY_ATTRIBUTES   lpThreadAttributes,
BOOL                    bInheritHandles,
ULONG                   dwCreationFlags,
LPVOID                  lpEnvironment,
LPCWSTR                 lpCurrentDirectory,
LPSTARTUPINFOW          lpStartupInfo,
LPPROCESS_INFORMATION   lpProcessInformation,
PHANDLE                 phNewToken
)
{
	BOOL             Result, IsSuspended;
	NTSTATUS         Status;
	UNICODE_STRING   FullDllPath;

	auto Data = GetSiglusHook();

	RtlInitUnicodeString(&FullDllPath, Data->DllPath.c_str());

	IsSuspended = !!(dwCreationFlags & CREATE_SUSPENDED);
	dwCreationFlags |= CREATE_SUSPENDED;
	Result = StubCreateProcessInternalW(
		hToken,
		lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation,
		phNewToken);

	if (!Result)
		return Result;

	Status = InjectDllToRemoteProcess(
		lpProcessInformation->hProcess,
		lpProcessInformation->hThread,
		&FullDllPath,
		IsSuspended
		);

	return TRUE;
}


BOOL SiglusHook::DetactNeedDumper()
{
	NTSTATUS   Status;
	NtFileDisk File;
	BYTE       Buffer[100];

	DisablePrivateKey = FALSE;


	LOOP_ONCE
	{
		Status = File.Open(SceneName.c_str());
		if (NT_FAILED(Status))
		{
			DisablePrivateKey = TRUE;
			MessageBoxW(NULL, L"Couldn't find Scene.pck", L"SiglusExtract", MB_OK | MB_ICONERROR);
			Ps::ExitProcess(0);
		}

		File.Read(Buffer, sizeof(SCENEHEADER));
		if (((SCENEHEADER*)Buffer)->ExtraKeyUse == 0)
		{
			ExtraDecInPck = FALSE;
		}
		File.Close();

		Status = File.Open(GameexeName.c_str());
		if (NT_FAILED(Status))
		{
			DisablePrivateKey = TRUE;
			MessageBoxW(NULL, L"Couldn't find Gameexe.dat", L"SiglusExtract", MB_OK | MB_ICONERROR);
			Ps::ExitProcess(0);
		}
		File.Read(Buffer, 8 + 16);
		RtlCopyMemory(GameexeBytes, Buffer, 24);
		File.Close();
		if (*(PDWORD)&Buffer[4] == 0)
		{
			ExtraDecInDat = FALSE;
		}
	}

	if (ExtraDecInDat || ExtraDecInPck)
	{
		Mp::PATCH_MEMORY_DATA p[] =
		{
			Mp::FunctionJumpVa(ReadFile, HookReadFile, &StubReadFile)
		};

		Mp::PatchMemory(p, countof(p));
	}

	return ExtraDecInDat || ExtraDecInPck;
}

BOOL SiglusHook::InitWindow()
{
	MODULEINFO          ModuleInfo;
	HANDLE              Handle;
	NTSTATUS            NtStatus;
	BOOL                Status;
	WCHAR               Message[MAX_PATH], Exec[MAX_PATH];
	DWORD               WaitStatus, BytesCount;
	HANDLE              hPipe, hEvent;
	HRSRC               ResourceHandle;
	ULONG               Size;
	PBYTE               Buffer;
	HGLOBAL             hGlobal;


	SelfPrivilegeUp();

	LOOP_ONCE
	{
		ExeModule = Nt_GetExeModuleHandle();
		DetactNeedDumper();

		ExtModuleHandle = NULL;

		//Bypass VA's Japanese locale check
		{
			
			Mp::PATCH_MEMORY_DATA p[] =
			{
				//check
				Mp::FunctionJumpVa(GetTimeZoneInformation,  HookGetTimeZoneInformation,  &StubGetTimeZoneInformation ),
				Mp::FunctionJumpVa(GetLocaleInfoW,          HookGetLocaleInfoW,          &StubGetLocaleInfoW         ),
				Mp::FunctionJumpVa(GetFileVersionInfoW,     HookGetFileVersionInfoW,     &StubGetFileVersionInfoW    ),
				Mp::FunctionJumpVa(GetFileVersionInfoSizeW, HookGetFileVersionInfoSizeW, &StubGetFileVersionInfoSizeW),

				//hook create process(steam)
				Mp::FunctionJumpVa(CreateProcessInternalW,  HookCreateProcessInternalW,  &StubCreateProcessInternalW)
			};

			Mp::PatchMemory(p, countof(p));
		}


		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		ULONG_PTR                    gdiplusToken;
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

		Mp::PATCH_MEMORY_DATA f[] =
		{
			Mp::FunctionJumpVa(CreateFileW, HookCreateFileW, &StubCreateFileW )
		};
		
		Status = NT_SUCCESS(Mp::PatchMemory(f, countof(f)));

		if (LookupImportTable(ExeModule, "KERNEL32.DLL", KERNEL32_MultiByteToWideChar) != IMAGE_INVALID_VA)
		{
			IAT_PATCH_DATA p[] =
			{
				{ ExeModule, MultiByteToWideChar, HookMultiByteToWideChar, "KERNEL32.DLL" }
			};

			Status = NT_SUCCESS(IATPatchMemory(p, countof(p)));
		}

	}
	return Status;
}
