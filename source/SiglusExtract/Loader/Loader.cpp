#include "my.h"

#pragma comment(linker, "/SECTION:.text,ERW /MERGE:.rdata=.text /MERGE:.data=.text")
#pragma comment(linker, "/SECTION:.Xmoe,ERW /MERGE:.text=.Xmoe")

#pragma comment(lib, "ntdll.lib")

typedef
BOOL
(WINAPI
*FuncCreateProcessInternalW)(
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
);

BOOL
(WINAPI
*StubCreateProcessInternalW)(
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
);

BOOL
WINAPI
VMeCreateProcess(
HANDLE                  hToken,
LPCWSTR                 lpApplicationName,
LPWSTR                  lpCommandLine,
LPCWSTR                 lpDllPath,
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
	UNICODE_STRING   FullDllPath;

	RtlInitUnicodeString(&FullDllPath, lpDllPath);

	StubCreateProcessInternalW =
		(FuncCreateProcessInternalW)Nt_GetProcAddress(GetKernel32Handle(),
		"CreateProcessInternalW");

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

	InjectDllToRemoteProcess(
		lpProcessInformation->hProcess,
		lpProcessInformation->hThread,
		&FullDllPath,
		IsSuspended
		);

	NtResumeThread(lpProcessInformation->hThread, NULL);

	return TRUE;
}

LPWSTR NTAPI StringCatW(LPWSTR lpString1, LPCWSTR lpString2)
{
	ULONG_PTR Length;

	Length = StrLengthW(lpString1);
	StrCopyW(&lpString1[Length], lpString2);
	return lpString1;
}

PVOID NTAPI LocalAllocater(ULONG Size)
{
	return AllocateMemoryP(Size);
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


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	NTSTATUS            Status;
	WCHAR               DllPath[MAX_PATH];
	ULONG               CodePage;
	STARTUPINFOW        si;
	PROCESS_INFORMATION pi;
	BOOL                Result;
	PWSTR               CmdLine;
	LONG_PTR            Argc;
	PWSTR*              Args;
	BOOL                IsDumper;
	PBYTE               Buffer;
	ULONG               Size;
	NtFileDisk          File;
	HANDLE              hFile, hEvent, Handle[2];
	DWORD               BytesCount, BytesCurrent;

	ml::MlInitialize();
	SelfPrivilegeUp();

	Buffer = NULL;
	Size   = NULL;
	BytesCurrent = 0;

	RtlZeroMemory(&si, sizeof(si));
	RtlZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	RtlZeroMemory(DllPath, MAX_PATH * 2);
	Nt_SetExeDirectoryAsCurrent();
	Nt_GetCurrentDirectory(MAX_PATH, DllPath);
	StringCatW(DllPath, L"\\");
	StringCatW(DllPath, L"SiglusExtract.dll");

	static WCHAR TargetFile[] = L"SiglusEngine.exe";

	IsDumper = FALSE;

	CmdLine = Ps::GetCommandLineW();
	Args = CmdLineToArgvW(CmdLine, &Argc); 

	if (Argc != 1 && Argc != 2)
		Ps::ExitProcess(0);

	if (Nt_GetFileAttributes(TargetFile) == 0xffffffff)
	{
		if (Argc < 2)
		{
			ReleaseArgv(Args);
			Ps::ExitProcess(STATUS_INVALID_PARAMETER);
		}
		Result = VMeCreateProcess(NULL, NULL, Args[1], DllPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi, NULL);
		
	}
	else
	{
		Result = VMeCreateProcess(NULL, NULL, TargetFile, DllPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi, NULL);
	}

	ReleaseArgv(Args);

	if (Result == FALSE)
	{
		MessageBoxW(NULL, L"SiglusExtract : Failed to Create Process!", L"SiglusExtract", MB_OK | MB_ICONERROR);
		Ps::ExitProcess(0);
	}

	Ps::ExitProcess(0);
	return Result;
}

