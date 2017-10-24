#include "my.h"
#include "MyHook.h"
#include <string>
#include <map>

#pragma comment(lib, "Version.lib")

ForceInline std::wstring FASTCALL ReplaceFileNameExtension(std::wstring& Path, PCWSTR NewExtensionName)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L".");
	if (Ptr == std::wstring::npos)
		return Path + NewExtensionName;

	return Path.substr(0, Ptr) + NewExtensionName;
}


ForceInline std::wstring FASTCALL GetFileName(std::wstring& Path)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L"\\");
	if (Ptr == std::wstring::npos)
		return Path;

	return Path.substr(Ptr + 1, std::wstring::npos);
}


ForceInline std::wstring FASTCALL GetFileNameExtension(std::wstring& Path)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L".");
	if (Ptr == std::wstring::npos)
		return NULL;

	return Path.substr(Ptr + 1, std::wstring::npos);
}


ForceInline std::wstring FASTCALL GetFileNamePrefix(std::wstring& Path)
{
	ULONG_PTR Ptr;

	Ptr = Path.find_last_of(L".");
	if (Ptr == std::wstring::npos)
		return Path;

	return Path.substr(0, Ptr);
}


//////////////////////////////////////////////////

NAKED VOID SarCheckFake()
{
	INLINE_ASM
	{
		mov esp, ebp;
		mov eax, 1;
		ret;
	}
}

#define MAX_SECTION_COUNT 64

inline PDWORD FASTCALL GetOffset(PBYTE ModuleBase, DWORD v)
{
	ULONG_PTR            Offset;
	IMAGE_SECTION_HEADER SectionTable[MAX_SECTION_COUNT];
	PIMAGE_DOS_HEADER    pDosHeader;
	PIMAGE_NT_HEADERS32  pNtHeader;

	pDosHeader = (PIMAGE_DOS_HEADER)ModuleBase;
	pNtHeader = (PIMAGE_NT_HEADERS32)(ModuleBase + pDosHeader->e_lfanew);
	RtlZeroMemory(SectionTable, sizeof(SectionTable));
	RtlCopyMemory(SectionTable, ModuleBase + sizeof(IMAGE_NT_HEADERS32) + pDosHeader->e_lfanew,
		pNtHeader->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));

	for (ULONG_PTR i = 0; i < pNtHeader->FileHeader.NumberOfSections; i++)
	{
		if (SectionTable[i].VirtualAddress <= v && v <= SectionTable[i].VirtualAddress + SectionTable[i].SizeOfRawData)
		{
			ULONG_PTR Delta = v - SectionTable[i].VirtualAddress;
			v = SectionTable[i].PointerToRawData + Delta;
			break;
		}
	}
	v += (ULONG_PTR)ModuleBase;
	return (PDWORD)v;
}

typedef struct XBundler
{
	PCHAR pDllName;
	PBYTE pBuffer;
	DWORD dwSize;
}XBundler, *PXBundler;

PXBundler pSarcheck = NULL;

API_POINTER(ZwAllocateVirtualMemory) StubZwAllocateVirtualMemory = NULL;

NTSTATUS NTAPI HookZwAllocateVirtualMemory(
	IN HANDLE ProcessHandle,
	IN OUT PVOID *BaseAddress,
	IN ULONG ZeroBits,
	IN OUT PULONG RegionSize,
	IN ULONG AllocationType,
	IN ULONG Protect
	)
{

	NTSTATUS          Status;
	PIMAGE_DOS_HEADER DosHeader;
	PIMAGE_NT_HEADERS NtHeader;
	DWORD             OldProtect;

	if ( pSarcheck &&
		!IsBadReadPtr(pSarcheck->pDllName, MAX_PATH) &&
		!IsBadReadPtr(pSarcheck->pBuffer, pSarcheck->dwSize) &&
		pSarcheck->pDllName + StrLengthA(pSarcheck->pDllName) + 5 == (PCHAR)pSarcheck->pBuffer&&
		*(PWORD)pSarcheck->pBuffer == 'ZM')
	{
		Status = StubZwAllocateVirtualMemory(ProcessHandle, BaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
		
		Mp::PATCH_MEMORY_DATA p[] =
		{
			Mp::FunctionJumpVa(ZwAllocateVirtualMemory, HookZwAllocateVirtualMemory, (PVOID*)&StubZwAllocateVirtualMemory),
		};

		Mp::RestoreMemory(p, countof(p));
		Nt_ProtectMemory(NtCurrentProcess(), pSarcheck->pBuffer, pSarcheck->dwSize, PAGE_EXECUTE_READWRITE, &OldProtect);

		DosHeader = (PIMAGE_DOS_HEADER)pSarcheck->pBuffer;
		NtHeader = (PIMAGE_NT_HEADERS32)((PBYTE)DosHeader + DosHeader->e_lfanew);
		PDWORD pEntryPoint = GetOffset(pSarcheck->pBuffer, NtHeader->OptionalHeader.AddressOfEntryPoint);
		PIMAGE_EXPORT_DIRECTORY pIET = (PIMAGE_EXPORT_DIRECTORY)GetOffset(pSarcheck->pBuffer, NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		*GetOffset(pSarcheck->pBuffer, pIET->AddressOfFunctions) = NtHeader->OptionalHeader.AddressOfEntryPoint + 3;


		
		//ret 0xc
		//ret 
		*pEntryPoint = 0xC3000CC2;
		//VirtualProtect(pEntryPoint, 4, OldProtect, &OldProtect);

		Nt_ProtectMemory(NtCurrentProcess(), pSarcheck->pBuffer, pSarcheck->dwSize, OldProtect, &OldProtect);

		return Status;
	}
	return StubZwAllocateVirtualMemory(ProcessHandle, BaseAddress, ZeroBits, RegionSize, AllocationType, Protect);
}

API_POINTER(VirtualAlloc) StubVirtualAlloc = NULL;

PVOID WINAPI HookVirtualAlloc(
	IN LPVOID lpAddress,
	IN SIZE_T dwSize,
	IN DWORD flAllocationType,
	IN DWORD flProtect
	)
{
	PWORD pByte = (PWORD)((PBYTE)_ReturnAddress() - 6);
	if (*pByte == 0x95FF)//call dword ptr[ebp+]
	{
		pSarcheck = (PXBundler)StubVirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);

		Mp::PATCH_MEMORY_DATA r[] =
		{
			Mp::FunctionJumpVa(VirtualAlloc, HookVirtualAlloc, &StubVirtualAlloc),
		};

		Mp::RestoreMemory(r, countof(r));

		Mp::PATCH_MEMORY_DATA p[] =
		{
			Mp::FunctionJumpVa(ZwAllocateVirtualMemory, HookZwAllocateVirtualMemory, (PVOID*)&StubZwAllocateVirtualMemory),
		};
		

		Mp::PatchMemory(p, countof(p));
		return pSarcheck;
	}
	return StubVirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
}


API_POINTER(GetProcAddress) StubGetProcAddress = NULL;

FARPROC WINAPI HookGetProcAddress(
	IN HMODULE hModule,
	IN LPCSTR lpProcName
	)
{
	if (!IsBadReadPtr(lpProcName, 9) &&
		!StrNICompareA(lpProcName, "Sarcheck", 9, StrCmp_ToLower))
	{
		Mp::PATCH_MEMORY_DATA p[] =
		{
			Mp::FunctionJumpVa(GetProcAddress, HookGetProcAddress, &StubGetProcAddress),
		};

		Mp::RestoreMemory(p, countof(p));

		return (FARPROC)SarCheckFake;
	}
	return StubGetProcAddress(hModule, lpProcName);
}


API_POINTER(GetTimeZoneInformation) StubGetTimeZoneInformation = NULL;

DWORD
WINAPI
HookGetTimeZoneInformation(
_Out_ LPTIME_ZONE_INFORMATION lpTimeZoneInformation
)
{
	static WCHAR StdName[] = L"TOKYO Standard Time";
	static WCHAR DayName[] = L"TOKYO Daylight Time";

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


API_POINTER(GetFileVersionInfoSizeW) StubGetFileVersionInfoSizeW = NULL;

DWORD
APIENTRY
HookGetFileVersionInfoSizeW(
_In_        LPCWSTR lptstrFilename, /* Filename of version stamped file */
_Out_opt_ LPDWORD lpdwHandle       /* Information for use by GetFileVersionInfo */
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
		return StubGetFileVersionInfoSizeW(L"SiglusUniversalPatch.dll", lpdwHandle);
	
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
		return StubGetFileVersionInfoW(L"SiglusUniversalPatch.dll", dwHandle, dwLen, lpData);

	return StubGetFileVersionInfoW(lptstrFilename, dwHandle, dwLen, lpData);
}




static struct SiglusConfig
{
	BOOL  PatchFontWidth;
	BOOL  PatchFontEnum;

	ULONG Address;
	BYTE  Code[8];

	SiglusConfig() : PatchFontWidth(FALSE), PatchFontEnum(FALSE), Address(0xFFFFFFFF){}
}m_SiglusConfig;


/*
___:00680C1C                 mov     ds:byte_AC3CC0[esi], al
___:00680C22                 inc     esi
___:00680C23                 cmp     esi, 10000h
*/

BOOL FASTCALL PatchFontWidthTableGenerator(ULONG_PTR ReturnAddress = 0)
{
	BOOL                  Success;
	PIMAGE_DOS_HEADER     DosHeader;
	PIMAGE_NT_HEADERS32   NtHeader;
	PIMAGE_SECTION_HEADER SectionHeader;
	ULONG_PTR             FirstSection;
	ULONG_PTR             FirstSize;
	PBYTE                 CurrentSection;
	ULONG_PTR             CurrentSectionSize;
	ULONG_PTR             CurrentCodePtr, CodeSize;

	DosHeader     = (PIMAGE_DOS_HEADER)Nt_GetExeModuleHandle();
	NtHeader      = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);
	SectionHeader = IMAGE_FIRST_SECTION(NtHeader);
	Success       = FALSE;

	auto PatchMemoryWithNope = [](PBYTE Addr, ULONG Size)->BOOL
	{
		BOOL  Success;
		DWORD OldFlag;

		LOOP_ONCE
		{
			Success = VirtualProtect(Addr, Size, PAGE_EXECUTE_READWRITE, &OldFlag);
			if (!Success)
				return Success;

			memcpy(m_SiglusConfig.Code, Addr, 6);
			memset(Addr, 0x90, Size);
			Success = VirtualProtect(Addr, Size, OldFlag, &OldFlag);
		}
		return Success;
	};

	//PrintConsole(L"patch...\n");

	for (ULONG_PTR i = 0; i < NtHeader->FileHeader.NumberOfSections; i++, SectionHeader++)
	{
		CurrentSection     = SectionHeader->VirtualAddress + (PBYTE)DosHeader;
		CurrentSectionSize = SectionHeader->Misc.VirtualSize;
		CurrentCodePtr     = 0;

		//PrintConsoleW(L"%08x %08x %08x\n", CurrentSection, ReturnAddress, (ULONG_PTR)CurrentSection + CurrentSectionSize);
		if (IN_RANGE((ULONG_PTR)CurrentSection, ReturnAddress, (ULONG_PTR)CurrentSection + CurrentSectionSize) && !IsBadReadPtr(CurrentSection, CurrentSectionSize))
		{
			//PrintConsoleW(L"enter patch\n");
			while (CurrentCodePtr < CurrentSectionSize)
			{
				//mov mem[offset], xl
				if (CurrentSection[CurrentCodePtr] == 0x88 &&
					GetOpCodeSize32(&CurrentSection[CurrentCodePtr]) == 6)
				{
					CurrentCodePtr += 6;
					//PrintConsole(L"st [%08x] -> %02x\n", SectionHeader->VirtualAddress + (ULONG)Nt_GetExeModuleHandle() + CurrentCodePtr, CurrentSection[CurrentCodePtr]);
					//inc esi
					if (CurrentSection[CurrentCodePtr] == 0x46 &&
						GetOpCodeSize32(&CurrentSection[CurrentCodePtr]) == 1)
					{
						CurrentCodePtr += 1;
						
						//cmp esi, 1000h
						if (*(PWORD) &CurrentSection[CurrentCodePtr]     == 0xFE81 &&
							*(PDWORD)&CurrentSection[CurrentCodePtr + 2] == 0x10000 &&
							GetOpCodeSize32(&CurrentSection[CurrentCodePtr]) == 6)
						{
							CurrentCodePtr += 6;
							//ok, then patch it.
							auto PatchAddr = CurrentCodePtr - 13 + CurrentSection;
							m_SiglusConfig.Address = (ULONG)PatchAddr;
							PatchMemoryWithNope(PatchAddr, 6);

							//PrintConsole(L"Patch Address : %p\n", PatchAddr);
							return TRUE;
						}
					}
				}
				else
				{
					CodeSize = GetOpCodeSize32(&CurrentSection[CurrentCodePtr]);
					CurrentCodePtr += CodeSize;
				}
			}
		}
	}
	return Success;
}

static BOOL ExtraPatchIsInited = FALSE;


LONG NTAPI PatchExceptionHandler(EXCEPTION_POINTERS *ExceptionInfo)
{
	auto RestoreMemoryPatch = [](PBYTE Addr, ULONG Size)->BOOL
	{
		BOOL  Success;
		DWORD OldFlag;

		LOOP_ONCE
		{
			Success = VirtualProtect(Addr, Size, PAGE_EXECUTE_READWRITE, &OldFlag);
			if (!Success)
				return Success;

			memcpy(Addr, m_SiglusConfig.Code, 6);
			Success = VirtualProtect(Addr, Size, OldFlag, &OldFlag);
		}
		return Success;
	};

	if (ExceptionInfo->ExceptionRecord->ExceptionAddress == (PVOID)m_SiglusConfig.Address)
	{
		RestoreMemoryPatch((PBYTE)m_SiglusConfig.Address, 6);
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

API_POINTER(CreateFileW) StubCreateFileW = NULL;

HANDLE
WINAPI
HookCreateFileW(
_In_ LPCWSTR lpFileName,
_In_ DWORD dwDesiredAccess,
_In_ DWORD dwShareMode,
_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
_In_ DWORD dwCreationDisposition,
_In_ DWORD dwFlagsAndAttributes,
_In_opt_ HANDLE hTemplateFile
)
{ 
	ULONG        Attribute;
	std::wstring CurFileName;

	auto IsScenePack = [](LPCWSTR FileName)->BOOL
	{
		ULONG_PTR iPos = 0;

		for (ULONG i = 0; i < StrLengthW(FileName); i++)
		{
			if (FileName[i] == L'\\' || FileName[i] == L'/')
				iPos = i;
		}

		if (iPos != 0)
			iPos++;

		return StrCompareW(FileName + iPos, L"Scene.pck") == 0;
	};


	auto IsGameExe = [](LPCWSTR FileName)->BOOL
	{
		ULONG_PTR iPos = 0;

		for (ULONG i = 0; i < StrLengthW(FileName); i++)
		{
			if (FileName[i] == L'\\' || FileName[i] == L'/')
				iPos = i;
		}

		if (iPos != 0)
			iPos++;

		return StrCompareW(FileName + iPos, L"Gameexe.dat") == 0;
	};

	auto IsG00Image = [](LPCWSTR FileName)->BOOL
	{
		ULONG Length = StrLengthW(FileName);
		if (Length <= 4)
			return FALSE;

		if (*(PULONG64)&FileName[Length - 4] == TAG4W('.g00'))
			return TRUE;

		if (CHAR_UPPER4W(*(PULONG64)&FileName[Length - 4]) == TAG4W('.G00'))
			return TRUE;

		return FALSE;
	};

	auto IsOmvVideo = [](LPCWSTR FileName)->BOOL
	{
		ULONG Length = StrLengthW(FileName);
		if (Length <= 4)
			return FALSE;

		if (*(PULONG64)&FileName[Length - 4] == TAG4W('.omv'))
			return TRUE;

		if (CHAR_UPPER4W(*(PULONG64)&FileName[Length - 4]) == TAG4W('.OMV'))
			return TRUE;

		return FALSE;
	};

	if (IsGameExe(lpFileName))
	{
		CurFileName = ReplaceFileNameExtension(std::wstring(lpFileName), L".dat2");
		Attribute = Nt_GetFileAttributes(CurFileName.c_str());

		if (ExtraPatchIsInited == FALSE)
		{

			//PrintConsole(L"Patch ....\n");
			if (m_SiglusConfig.PatchFontWidth && PatchFontWidthTableGenerator((ULONG_PTR)_ReturnAddress()))
				AddVectoredExceptionHandler(TRUE, PatchExceptionHandler);

			ExtraPatchIsInited = TRUE;
		}

		if ((Attribute == 0xffffffff) || (Attribute & FILE_ATTRIBUTE_DIRECTORY))
			CurFileName = lpFileName;
	}
	else if (IsScenePack(lpFileName))
	{
		CurFileName = ReplaceFileNameExtension(std::wstring(lpFileName), L".pck2");
		Attribute = Nt_GetFileAttributes(CurFileName.c_str());

		if ((Attribute == 0xffffffff) || (Attribute & FILE_ATTRIBUTE_DIRECTORY))
			CurFileName = lpFileName;
	}
	else if (IsG00Image(lpFileName))
	{
		CurFileName = ReplaceFileNameExtension(std::wstring(lpFileName), L".g01");
		Attribute = Nt_GetFileAttributes(CurFileName.c_str());

		if ((Attribute == 0xffffffff) || (Attribute & FILE_ATTRIBUTE_DIRECTORY))
			CurFileName = lpFileName;
	}
	else if (IsOmvVideo(lpFileName))
	{
		CurFileName = ReplaceFileNameExtension(std::wstring(lpFileName), L".om2");
		Attribute = Nt_GetFileAttributes(CurFileName.c_str());

		if ((Attribute == 0xffffffff) || (Attribute & FILE_ATTRIBUTE_DIRECTORY))
			CurFileName = lpFileName;
	}
	else
	{
		CurFileName = lpFileName;
	}

	return StubCreateFileW(
		CurFileName.c_str(),
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile
		);
}


#define ConfigName L"SiglusEnginePatch.ini"
#define FontName   L"SiglusEngineFont.ini"


std::wstring GameFont = L"ºÚÌå";

FORCEINLINE Void LoadFontConfig()
{
	NTSTATUS   Status;
	NtFileDisk File;
	WCHAR      Utf16Buf[MAX_PATH];
	CHAR       Utf8Buf[MAX_PATH];
	CHAR       Bom[4];
	ULONG      Size;

	LOOP_ONCE
	{
		Status = File.Open(FontName);
		if (NT_FAILED(Status))
			return;

		Size = File.GetSize32();
		if (Size < 3)
			break;

		RtlZeroMemory(Utf16Buf, sizeof(Utf16Buf));
		RtlZeroMemory(Utf8Buf, sizeof(Utf8Buf));

		File.Read(Bom, 3);
		if (Bom[0] == 0xFF && Bom[1] == 0xFE)
		{
			File.Seek(-1, FILE_CURRENT);
			if (Size % 2 != 0)
				break;

			File.Read(Utf16Buf, Size - 2);
		}
		else if (Bom[0] == 0xEF && Bom[1] == 0xBB && Bom[2] == 0xBF)
		{
			File.Read(Utf8Buf, Size - 3);
			MultiByteToWideChar(CP_UTF8, 0, Utf8Buf, StrLengthA(Utf8Buf), Utf16Buf, countof(Utf16Buf) - 1);
			GameFont = Utf16Buf;
		}
		else
		{
			File.Seek(-3, FILE_CURRENT);
			File.Read(Utf8Buf, Size);
			MultiByteToWideChar(CP_UTF8, 0, Utf8Buf, StrLengthA(Utf8Buf), Utf16Buf, countof(Utf16Buf) - 1);
			GameFont = Utf16Buf;
		}

		m_SiglusConfig.PatchFontEnum = TRUE;
	}
	File.Close();
}


FORCEINLINE Void LoadConfig()
{
	NTSTATUS   Status;
	NtFileDisk File;
	BYTE       Flag;

	LOOP_ONCE
	{
		Status = File.Open(ConfigName);
		if (NT_FAILED(Status))
			break;

		if (File.GetSize32() == 0)
			break;

		File.Read(&Flag, 1);
		if (Flag != 'P')
			break;

		m_SiglusConfig.PatchFontWidth = TRUE;
	}
	File.Close();
}


BOOL FASTCALL BypassAlphaRom(HMODULE DllModule)
{
	PIMAGE_DOS_HEADER     DosHeader;
	PIMAGE_NT_HEADERS32   NtHeader;
	PIMAGE_SECTION_HEADER SectionHeader;
	ULONG_PTR             FirstSection;
	ULONG_PTR             FirstSize;
	ULONG                 i;

	DosHeader = (PIMAGE_DOS_HEADER)Nt_GetExeModuleHandle();
	NtHeader = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);
	SectionHeader = IMAGE_FIRST_SECTION(NtHeader);
	FirstSection = SectionHeader->VirtualAddress + (ULONG_PTR)DosHeader;
	FirstSize = SectionHeader->Misc.VirtualSize;
	i = 0;

	while (i++ < NtHeader->FileHeader.NumberOfSections)
	{
		if (*(PDWORD)&SectionHeader++->Name[1] == 'crsr')
		{
			while (*(PDWORD)&SectionHeader->Name[1] != 'tadi'&&
				*(PWORD)&SectionHeader->Name[5] != 'a'&&
				++i < NtHeader->FileHeader.NumberOfSections)//Ñ°ÕÒ.idata¶Î
			{
				SectionHeader++;
			}

			if (*(PDWORD)&SectionHeader->Name[1] == 'ttes'&&
				*(PWORD)&SectionHeader->Name[5] == 'ce')//settec ---AlphaROM
			{
				//hGFNA = InstallHookStub(GetModuleFileNameA, My_GetModuleFileNameA);
			}

			if (i < NtHeader->FileHeader.NumberOfSections)
			{
				Mp::PATCH_MEMORY_DATA p[] =
				{
					Mp::FunctionJumpVa(VirtualAlloc, HookVirtualAlloc, &StubVirtualAlloc),
				};

				Mp::PatchMemory(p, countof(p));
			}
			else
			{
				DWORD dwOld;
				VirtualProtect((PVOID)FirstSection, FirstSize, PAGE_EXECUTE_READWRITE, &dwOld);

				Mp::PATCH_MEMORY_DATA p[] =
				{
					Mp::FunctionJumpVa(GetProcAddress, HookGetProcAddress, &StubGetProcAddress),
				};

				Mp::PatchMemory(p, countof(p));
			}
			break;
		}
	}

	Mp::PATCH_MEMORY_DATA p[] =
	{
		Mp::FunctionJumpVa(GetTimeZoneInformation,  HookGetTimeZoneInformation,  &StubGetTimeZoneInformation),
		Mp::FunctionJumpVa(GetLocaleInfoW,          HookGetLocaleInfoW,          &StubGetLocaleInfoW),
		Mp::FunctionJumpVa(GetFileVersionInfoSizeW, HookGetFileVersionInfoSizeW, &StubGetFileVersionInfoSizeW),
		Mp::FunctionJumpVa(GetFileVersionInfoW,     HookGetFileVersionInfoW,     &StubGetFileVersionInfoW),
	};

	return NT_SUCCESS(Mp::PatchMemory(p, countof(p)));
}


API_POINTER(FindWindowW) StubFindWindowW = NULL;

HWND
WINAPI
HookFindWindowW(
_In_opt_ LPCWSTR lpClassName,
_In_opt_ LPCWSTR lpWindowName)
{
	//PrintConsoleW(L"%s %s\n", lpClassName, lpWindowName);
	return StubFindWindowW(lpClassName, lpWindowName);
}

Void BypassDebuggerCheck()
{
	Mp::PATCH_MEMORY_DATA p[] =
	{
		Mp::FunctionJumpVa(FindWindowW, HookFindWindowW, &StubFindWindowW)
	};

	Mp::PatchMemory(p, countof(p));
}


API_POINTER(GetUserNameA) StubGetUserNameA = NULL;

BOOL WINAPI HookGetUserNameA(
	_Out_   LPSTR  lpBuffer,
	_Inout_ LPDWORD lpnSize
)
{
	ULONG_PTR  ReturnAddress, OpSize;
	DWORD      OldProtect;

	//PrintConsoleW(L"get user name..........\n");

	INLINE_ASM
	{
		mov eax, [ebp];  
		mov ebx, [eax + 4]; //ret addr
		mov ReturnAddress, ebx;
	}

	//PrintConsoleW(L"%08x\n", ReturnAddress);

	//find the first 'jnz' 
	OpSize = 0;
	for(ULONG_PTR i = 0; i < 0x30;)
	{
		OpSize = GetOpCodeSize32((PBYTE)(ReturnAddress + i));
		if (OpSize == 2 && ((PBYTE)(ReturnAddress + i))[0] == 0x75) //short jump
		{
			VirtualProtect((PBYTE)(ReturnAddress + i), 2, PAGE_EXECUTE_READWRITE, &OldProtect);
			((PBYTE)(ReturnAddress + i))[0] = 0xB0;
			((PBYTE)(ReturnAddress + i))[1] = 0x01;
			VirtualProtect((PBYTE)(ReturnAddress + i), 2, OldProtect, &OldProtect);
			//PrintConsoleW(L"patch..........\n");
			break;
		}
		i += OpSize;
	}

	return StubGetUserNameA(lpBuffer, lpnSize);
}


BOOL BypassDummyCheck()
{
	Mp::PATCH_MEMORY_DATA p[] = 
	{
		Mp::FunctionJumpVa(GetUserNameA, HookGetUserNameA, &StubGetUserNameA)
	};

	return NT_SUCCESS(Mp::PatchMemory(p, countof(p)));
}


API_POINTER(CreateFontIndirectW) StubCreateFontIndirectW = NULL;

HFONT WINAPI HookCreateFontIndirectW(_In_ CONST LOGFONTW *lplf)
{
	LOGFONTW Font;
	
	RtlCopyMemory(&Font, lplf, sizeof(LOGFONTW));
	RtlCopyMemory(Font.lfFaceName, GameFont.c_str(), (GameFont.length() + 1) * 2);
	return StubCreateFontIndirectW(&Font);
}


BOOL FASTCALL Initialize(HMODULE DllModule)
{
	//AllocConsole();
	BypassDebuggerCheck();

	Nt_LoadLibrary(L"ADVAPI32.DLL");

	Mp::PATCH_MEMORY_DATA p[] =
	{
		Mp::FunctionJumpVa(CreateFileW,         HookCreateFileW,         &StubCreateFileW),
		Mp::FunctionJumpVa(CreateFontIndirectW, HookCreateFontIndirectW, &StubCreateFontIndirectW)
	};
	
	BypassAlphaRom(DllModule);
	BypassDummyCheck();

	return NT_SUCCESS(Mp::PatchMemory(p, countof(p)));
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD Reason, LPVOID lpReserved)
{
	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		ml::MlInitialize();
		LoadConfig();
		LoadFontConfig();
		return Initialize(hModule);

	case DLL_PROCESS_DETACH:
		ml::MlUnInitialize();
		break;
	}
	return TRUE;
}

