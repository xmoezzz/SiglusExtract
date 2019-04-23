#include "SiglusHook.h"
#include "mt64.h"


BOOL Init(HMODULE hModule)
{
	BOOL        Status;
	SiglusHook* Data;
	WCHAR       FullDllPath[MAX_PATH];

	//AllocConsole();
	LOOP_ONCE
	{
		Status = FALSE;

		Data = GetSiglusHook();
		if (!Data)
			break;

		RtlZeroMemory(FullDllPath, sizeof(FullDllPath));
		GetModuleFileNameW(hModule, FullDllPath, countof(FullDllPath));
		Data->DllPath = FullDllPath;

		Data->SetDllModule(hModule);
		Data->SetExeModule((HMODULE)GetModuleHandleW(NULL));
		Status = Data->InitWindow();
	}
	return Status;
}

BOOL UnInit()
{
	return TRUE;
}

/*
extern "C" BOOL NTAPI DllMain(HMODULE hModule, DWORD Reason, LPVOID lpReserved)
{
	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		return Init(hModule);
	case DLL_PROCESS_DETACH:
		return UnInit();
	}
	return TRUE;
}
*/


class CSiglusExtractApp : public CWinApp
{
public:
	CSiglusExtractApp();

public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CSiglusExtractApp, CWinApp)
END_MESSAGE_MAP()


CSiglusExtractApp::CSiglusExtractApp()
{
}

CSiglusExtractApp theApp;

ULONG64 MurmurHash64B(const void * key, int len, ULONG seed = 0xEE6B27EB)
{
	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	unsigned int h1 = seed ^ len;
	unsigned int h2 = 0;

	const unsigned int * data = (const unsigned int *)key;

	while (len >= 8)
	{
		unsigned int k1 = *data++;
		k1 *= m; k1 ^= k1 >> r; k1 *= m;
		h1 *= m; h1 ^= k1;
		len -= 4;

		unsigned int k2 = *data++;
		k2 *= m; k2 ^= k2 >> r; k2 *= m;
		h2 *= m; h2 ^= k2;
		len -= 4;
	}

	if (len >= 4)
	{
		unsigned int k1 = *data++;
		k1 *= m; k1 ^= k1 >> r; k1 *= m;
		h1 *= m; h1 ^= k1;
		len -= 4;
	}

	switch (len)
	{
	case 3: h2 ^= ((unsigned char*)data)[2] << 16;
	case 2: h2 ^= ((unsigned char*)data)[1] << 8;
	case 1: h2 ^= ((unsigned char*)data)[0];
		h2 *= m;
	};

	h1 ^= h2 >> 18; h1 *= m;
	h2 ^= h1 >> 22; h2 *= m;
	h1 ^= h2 >> 17; h1 *= m;
	h2 ^= h1 >> 19; h2 *= m;

	unsigned long long h = h1;

	h = (h << 32) | h2;

	return h;
}

VOID InitRand(HMODULE hModule)
{
	ULONG64  Seeds[4];
	WCHAR    Path[MAX_PATH];

	RtlZeroMemory(Path, countof(Path) * 2);
	Nt_GetExeDirectory(Path, MAX_PATH);

	Seeds[0] = MakeQword(GetCurrentProcessId(), GetCurrentThreadId());
	Seeds[1] = MakeQword(Nt_CurrentPeb()->ProcessHeap, Nt_CurrentTeb()->EnvironmentPointer);
	Seeds[2] = MurmurHash64B(Path, lstrlenW(Path) * 2);
	Seeds[3] = MakeQword(hModule, GetModuleHandleW(NULL));

	init_by_array64(Seeds, countof(Seeds));
}

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

	if (pSarcheck &&
		!IsBadReadPtr(pSarcheck->pDllName, MAX_PATH) &&
		!IsBadReadPtr(pSarcheck->pBuffer, pSarcheck->dwSize) &&
		pSarcheck->pDllName + lstrlenA(pSarcheck->pDllName) + 5 == (PCHAR)pSarcheck->pBuffer&&
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
		!strnicmp(lpProcName, "Sarcheck", 9))
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


BOOL FASTCALL InitializeFuckAlphaRom(HMODULE DllModule)
{

	PIMAGE_DOS_HEADER   DosHeader;
	PIMAGE_NT_HEADERS32 NtHeader;
	PIMAGE_SECTION_HEADER SectionHeader;
	ULONG_PTR FirstSection;
	ULONG_PTR FirstSize;
	ULONG     i;

	DosHeader = (PIMAGE_DOS_HEADER)GetModuleHandleW(NULL);
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
	return TRUE;
}


BOOL LookupImportHasAzure(PVOID ImageBase, PCSTR DllName = "kDays.dll")
{
	BOOL  Success;

	Success       = FALSE;

	WalkImportTableT(ImageBase,
		WalkIATCallbackM(Data)
	{
		if (DllName != nullptr)
		{
			if (lstrcmpA(Data->DllName, DllName) != 0)
			{
				return STATUS_VALIDATE_CONTINUE;
			}
			else
			{
				Success = TRUE;
				return STATUS_SUCCESS;
			}
		}
		return STATUS_SUCCESS;
	},
		0
		);

	return Success;
}


API_POINTER(FindWindowW) StubFindWindowW = NULL;

HWND
WINAPI
HookFindWindowW(
_In_opt_ LPCWSTR lpClassName,
_In_opt_ LPCWSTR lpWindowName)
{
	PrintConsoleW(L"%s %s\n", lpClassName, lpWindowName);
	return StubFindWindowW(lpClassName, lpWindowName);
}

VOID BypassDebuggerCheck()
{
	Mp::PATCH_MEMORY_DATA p[] =
	{
		Mp::FunctionJumpVa(FindWindowW, HookFindWindowW, &StubFindWindowW)
	};

	Mp::PatchMemory(p, countof(p));
}


BOOL CSiglusExtractApp::InitInstance()
{
	CWinApp::InitInstance();
	AFX_MODULE_STATE* State = AfxGetModuleState();
	InitRand(State->m_hCurrentInstanceHandle);

	BypassDebuggerCheck();

	if (Nt_GetModuleHandle(L"kDays.dll") == NULL || Nt_GetModuleHandle(L"SiglusUniversalPatch.dll") == NULL)
		InitializeFuckAlphaRom(State->m_hCurrentInstanceHandle);

	return Init(State->m_hCurrentInstanceHandle);
}
