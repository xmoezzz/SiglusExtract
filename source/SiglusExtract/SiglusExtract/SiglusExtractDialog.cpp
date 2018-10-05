// SiglusExtractDialog.cpp : 实现文件
//

#include "stdafx.h"
#include "SiglusExtractDialog.h"
#include "afxdialogex.h"
#include "Tool.h"
#include "UnpackOGG.h"
#include "UnpackGameexe.h"
#include "UnpackScene.h"
#include "UnpackNwa.h"
#include "UnpackG00.h"
#include "UnpackOmv.h"
#include "UnpackOvk.h"
#include "UnpackNwk.h"
#include "mt64.h"
#include "CreateGameExe.h"
#include "CreateScenePck.h"
#include "resource.h"

// CSiglusExtractDialog 对话框

IMPLEMENT_DYNAMIC(CSiglusExtractDialog, CDialogEx)

CSiglusExtractDialog::CSiglusExtractDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSiglusExtractDialog::IDD, pParent)
{

	EnableAutomation();

	m_InheritIcon = TRUE;
	m_G00Flag = G00Decode::G00_BMP;
	m_NWAFlag = NWADecode::NWA_WAV;
	m_OGGFlag = OGGDecode::OGG_DECODE;
	m_OGVFlag = OGVDecode::OGV_DECODE;
	m_SSFlag  = SSDecode::SS_V2;

	WorkerThreadHandle = INVALID_HANDLE_VALUE;

	m_ForbiddenPrivateKey = FALSE;

	RtlZeroMemory(m_PrivateKey, sizeof(m_PrivateKey));
}

CSiglusExtractDialog::~CSiglusExtractDialog()
{
}

void CSiglusExtractDialog::OnFinalRelease()
{
	// 释放了对自动化对象的最后一个引用后，将调用
	// OnFinalRelease。  基类将自动
	// 删除该对象。  在调用该基类之前，请添加您的
	// 对象所需的附加清理代码。

	CDialogEx::OnFinalRelease();
}

void CSiglusExtractDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSiglusExtractDialog, CDialogEx)
	ON_BN_CLICKED(IDC_GOO_BMP, &CSiglusExtractDialog::OnBnClickedGooBmp)
	ON_BN_CLICKED(IDC_G00_PNG, &CSiglusExtractDialog::OnBnClickedG00Png)
	ON_BN_CLICKED(IDC_G00_JPG, &CSiglusExtractDialog::OnBnClickedG00Jpg)
	ON_BN_CLICKED(IDC_NWA_WAV, &CSiglusExtractDialog::OnBnClickedNwaWav)
	ON_BN_CLICKED(IDC_NWA_FLAC, &CSiglusExtractDialog::OnBnClickedNwaFlac)
	ON_BN_CLICKED(IDC_NWA_OGG, &CSiglusExtractDialog::OnBnClickedNwaOgg)
	ON_BN_CLICKED(IDC_OGG_DEC, &CSiglusExtractDialog::OnBnClickedOggDec)
	ON_BN_CLICKED(IDC_OGG_RAW, &CSiglusExtractDialog::OnBnClickedOggRaw)
	ON_BN_CLICKED(IDC_OMV_OGV, &CSiglusExtractDialog::OnBnClickedOmvOgv)
	ON_BN_CLICKED(IDC_OMV_PNG, &CSiglusExtractDialog::OnBnClickedOmvPng)
	ON_EN_CHANGE(IDC_SCE_EDIT, &CSiglusExtractDialog::OnEnChangeSceEdit)
	ON_BN_CLICKED(IDC_TEXT_V1, &CSiglusExtractDialog::OnBnClickedTextV1)
	ON_BN_CLICKED(IDC_TEXT_V2, &CSiglusExtractDialog::OnBnClickedTextV2)
	ON_BN_CLICKED(IDC_ICON_CHECK, &CSiglusExtractDialog::OnBnClickedIconCheck)
	ON_BN_CLICKED(IDC_SCE_SEL, &CSiglusExtractDialog::OnBnClickedSceSel)
	ON_BN_CLICKED(IDC_SCE_PACK, &CSiglusExtractDialog::OnBnClickedScePack)
	ON_WM_DROPFILES()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_GAMEEXE_SEL, &CSiglusExtractDialog::OnBnClickedGameexeSel)
	ON_BN_CLICKED(IDC_PATCH_BUTTON, &CSiglusExtractDialog::OnBnClickedPatchButton)
	ON_COMMAND(ID_FEEDBACK_DEVELOPER, &CSiglusExtractDialog::OnFeedbackDeveloperWeibo)
	ON_COMMAND(ID_FEEDBACK_DEVELOPER40004, &CSiglusExtractDialog::OnFeedbackDeveloperTwitter)
	ON_COMMAND(ID_ABOUT, &CSiglusExtractDialog::OnAbout)
	ON_COMMAND(ID_FEEDBACK_SENDFEEDBACK, &CSiglusExtractDialog::OnFeedbackSendfeedback)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CSiglusExtractDialog, CDialogEx)
END_DISPATCH_MAP()

// 注意: 我们添加 IID_ISiglusExtractDialog 支持
//  以支持来自 VBA 的类型安全绑定。  此 IID 必须同附加到 .IDL 文件中的
//  调度接口的 GUID 匹配。

// {EB7B8100-AB2B-40D4-A467-F8398D6B44DB}
static const IID IID_ISiglusExtractDialog =
{ 0xEB7B8100, 0xAB2B, 0x40D4, { 0xA4, 0x67, 0xF8, 0x39, 0x8D, 0x6B, 0x44, 0xDB } };

BEGIN_INTERFACE_MAP(CSiglusExtractDialog, CDialogEx)
	INTERFACE_PART(CSiglusExtractDialog, IID_ISiglusExtractDialog, Dispatch)
END_INTERFACE_MAP()


// CSiglusExtractDialog 消息处理程序


void CSiglusExtractDialog::ForbiddenPrivateKey()
{
	m_ForbiddenPrivateKey = TRUE;
}

//GOO 选择bmp
void CSiglusExtractDialog::OnBnClickedGooBmp()
{
	m_G00Flag = G00Decode::G00_BMP;
}

//GOO 选择png
void CSiglusExtractDialog::OnBnClickedG00Png()
{
	m_G00Flag = G00Decode::G00_PNG;
}


void CSiglusExtractDialog::OnBnClickedG00Jpg()
{
	m_G00Flag = G00Decode::G00_JPG;
}

//NWA 选择wav
void CSiglusExtractDialog::OnBnClickedNwaWav()
{
	m_NWAFlag = NWADecode::NWA_WAV;
}


void CSiglusExtractDialog::OnBnClickedNwaFlac()
{
	m_NWAFlag = NWADecode::NWA_FLAC;
}


void CSiglusExtractDialog::OnBnClickedNwaOgg()
{
	m_NWAFlag = NWADecode::NWA_VORBIS;
}


//encrypted ogg
void CSiglusExtractDialog::OnBnClickedOggDec()
{
	m_OGGFlag = OGGDecode::OGG_DECODE;
}


void CSiglusExtractDialog::OnBnClickedOggRaw()
{
	m_OGGFlag = OGGDecode::OGG_RAW;
}

//normal
void CSiglusExtractDialog::OnBnClickedOmvOgv()
{
	m_OGVFlag = OGVDecode::OGV_DECODE;
}


void CSiglusExtractDialog::OnBnClickedOmvPng()
{
	m_OGVFlag = OGVDecode::OGV_PNG;
}


void CSiglusExtractDialog::OnEnChangeSceEdit()
{
	//null
}


void CSiglusExtractDialog::OnBnClickedTextV1()
{
	m_SSFlag = SSDecode::SS_V1;
}


void CSiglusExtractDialog::OnBnClickedTextV2()
{
	m_SSFlag = SSDecode::SS_V2;
}


void CSiglusExtractDialog::OnBnClickedIconCheck()
{
	CButton* Item = (CButton*)GetDlgItem(IDC_ICON_CHECK);
	if (Item)
	{
		m_InheritIcon = Item->GetCheck() == 1;
	}
	else
	{
		m_InheritIcon = FALSE;
	}
}


//repack scene.pck
void CSiglusExtractDialog::OnBnClickedSceSel()
{
	BROWSEINFOW      sInfo;
	WCHAR            FolderPath[MAX_PATH];
	RtlZeroMemory(&sInfo, sizeof(BROWSEINFOW));
	RtlZeroMemory(FolderPath, sizeof(FolderPath));

	sInfo.hwndOwner = GetSafeHwnd();
	sInfo.pidlRoot = 0;
	sInfo.lpszTitle = _T("Please select a folder that contains ss scripts");
	sInfo.ulFlags = BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
	sInfo.lpfn = NULL;
	
	LPITEMIDLIST lpidlBrowse = ::SHBrowseForFolderW(&sInfo);
	if (lpidlBrowse != NULL)
	{
		::SHGetPathFromIDListW(lpidlBrowse, FolderPath);
		CWnd* Item = GetDlgItem(IDC_SCE_EDIT);
		if (Item) Item->SetWindowTextW(FolderPath);
	}

	if (lpidlBrowse != NULL)
	{
		::CoTaskMemFree(lpidlBrowse);
	}
}


struct CreatePckUserData
{
	CSiglusExtractDialog* This;
	std::wstring          Path;
	std::wstring          FileName;
};


DWORD NTAPI CreateWorkerThread(PVOID UserData)
{
	NTSTATUS              Status;
	CreatePckUserData*    Data;

	Data = (CreatePckUserData*)UserData;

	Status = CreateScenePck(Data->This->GetSafeHwnd(), Data->This->m_PrivateKey, Data->Path.c_str(), Data->FileName.c_str(), Data->This->m_PckNeedKey);
	if (NT_SUCCESS(Status))
	{
		Data->This->MessageBoxW(L"Make scene.pck successfully!", L"SiglusExtract", MB_OK);
	}

	return 0;
}


DWORD NTAPI CreateWorkerThreadWarpper(PVOID UserData)
{
	NTSTATUS              Status;
	CreatePckUserData*    Data;
	CProgressCtrl*        Item;

	Data = (CreatePckUserData*)UserData;
	Item = (CProgressCtrl*)Data->This->GetDlgItem(IDC_PROGRESS1);
	if (Item)
	{
		Item->ModifyStyle(0, PBS_MARQUEE);
		Item->SetMarquee(TRUE, 100);
	}

	Data->This->DisableAll();
	Status = Nt_CreateThread(CreateWorkerThread, UserData, FALSE, NtCurrentProcess(), &(Data->This->WorkerThreadHandle));
	if (NT_FAILED(Status))
	{
		Data->This->MessageBoxW(L"Couldn't launch worker thread!", L"SiglusExtract", MB_OK | MB_ICONERROR);
		return STATUS_INVALID_THREAD;
	}

	WaitForSingleObject(Data->This->WorkerThreadHandle, INFINITE);
	NtClose(Data->This->WorkerThreadHandle);
	Data->This->WorkerThreadHandle = INVALID_HANDLE_VALUE;
	Data->This->InternalReset();
	Data->This->EnableAll();

	if (Item)
	{
		Item->SetMarquee(FALSE, 100);
		Item->ModifyStyle(PBS_MARQUEE, 0);
	}

	return 0;
}

static CreatePckUserData Param;

//dopack scene.pck
void CSiglusExtractDialog::OnBnClickedScePack()
{
	NTSTATUS          Status;
	DWORD             ExitCode;
	ULONG             Attribute;
	LARGE_INTEGER     Timeout;
	WCHAR             Path[MAX_PATH], FileName[MAX_PATH];
	
	RtlZeroMemory(Path, sizeof(Path));
	RtlZeroMemory(FileName, sizeof(FileName));

	CWnd* Item = GetDlgItem(IDC_SCE_EDIT);
	if (Item) Item->GetWindowTextW(Path, countof(Path));

	Item = GetDlgItem(IDC_SCE_EDIT2);
	if (Item) Item->GetWindowTextW(FileName, countof(FileName));

	Attribute = Nt_GetFileAttributes(Path);
	if (Attribute == 0xffffffff || !(Attribute & FILE_ATTRIBUTE_DIRECTORY))
	{
		MessageBoxW(L"Invalid folder name!", L"SiglusExtract", MB_OK | MB_ICONERROR);
		return;
	}

	if (StrLengthW(FileName) == 0)
	{
		MessageBoxW(L"Invalid file name for the output file!", L"SiglusExtract", MB_OK | MB_ICONERROR);
		return;
	}

	Attribute = Nt_GetFileAttributes(FileName);
	if (Attribute != 0xffffffff)
	{
		FormatStringW(FileName, L"Scene_%08x.pck", genrand64_int64());
		Item = GetDlgItem(IDC_SCE_EDIT2);
		if (Item) Item->SetWindowTextW(FileName);
	}

	if (WorkerThreadHandle != INVALID_HANDLE_VALUE)
	{
		ExitCode = STILL_ACTIVE;
		GetExitCodeThread(WorkerThreadHandle, &ExitCode);
		if (ExitCode == STILL_ACTIVE)
		{
			Timeout.QuadPart = 500;
			NtWaitForSingleObject(WorkerThreadHandle, TRUE, &Timeout);
			GetExitCodeThread(WorkerThreadHandle, &ExitCode);
			if (ExitCode == STILL_ACTIVE)
				Nt_TerminateThread(WorkerThreadHandle, 0);
		}
	}
	NtClose(WorkerThreadHandle);
	WorkerThreadHandle = INVALID_HANDLE_VALUE;

	Param.This = this;
	Param.Path = Path;
	Param.FileName = FileName;
	

	Status = Nt_CreateThread(CreateWorkerThreadWarpper, &Param);
	if (NT_FAILED(Status))
	{
		MessageBoxW(L"Couldn't launch worker thread[warpper thread (at creating pck)]!", L"SiglusExtract", MB_OK | MB_ICONERROR);
		return;
	}

}


DWORD NTAPI WorkerThread(PVOID UserData);
DWORD NTAPI WorkerThreadWarpper(PVOID UserData);

void CSiglusExtractDialog::OnDropFiles(HDROP hDropInfo)
{
	NTSTATUS        Status;
	ULONG           DropCount, Attribute, Length;
	HANDLE          Handle;
	BOOL            HasSeparator;
	WCHAR           ItemFileName[MAX_PATH], SearchString[MAX_PATH];
	WIN32_FIND_DATA NextInfo;
	DWORD           ExitCode;
	LARGE_INTEGER   Timeout;
	
	auto CreateAddObject = [&](LPCWSTR FileName)
	{
		std::wstring ExtensionFileName = GetFileNameExtension(std::wstring(FileName));
		std::wstring CurrentFileName = GetFileName(std::wstring(FileName));


		if (!StrICompareW(ExtensionFileName.c_str(), L"OWP", StrCmp_ToUpper))
		{
			iUnpackObject* Object = new UnpackOGG();
			Object->SetFile(FileName);
			WorkerList.push_back(Object);
		}
		else if (!StrICompareW(ExtensionFileName.c_str(), L"NWA", StrCmp_ToUpper))
		{
			iUnpackObject* Object = new UnpackNWA();
			Object->SetFile(FileName);
			WorkerList.push_back(Object);
		}
		else if (!StrICompareW(ExtensionFileName.c_str(), L"G00", StrCmp_ToUpper) ||
			     !StrICompareW(ExtensionFileName.c_str(), L"G01", StrCmp_ToUpper))
		{
			iUnpackObject* Object = new UnpackG00();
			Object->SetFile(FileName);
			WorkerList.push_back(Object);
		}
		else if (!StrICompareW(ExtensionFileName.c_str(), L"OMV", StrCmp_ToUpper))
		{
			iUnpackObject* Object = new UnpackOMV();
			Object->SetFile(FileName);
			WorkerList.push_back(Object);
		}
		else if (!StrICompareW(ExtensionFileName.c_str(), L"OVK", StrCmp_ToUpper))
		{
			iUnpackObject* Object = new UnpackOVK();
			Object->SetFile(FileName);
			WorkerList.push_back(Object);
		}
		else if (!StrICompareW(ExtensionFileName.c_str(), L"NWK", StrCmp_ToUpper))
		{
			iUnpackObject* Object = new UnpackNWK();
			Object->SetFile(FileName);
			WorkerList.push_back(Object);
		}
		else if (!StrICompareW(CurrentFileName.c_str(), L"GAMEEXE.DAT", StrCmp_ToUpper) &&
			     !m_ForbiddenPrivateKey)
		{
			iUnpackObject* Object = new UnpackGameexe();
			Object->SetFile(FileName);
			WorkerList.push_back(Object);
		}
		else if (!StrICompareW(CurrentFileName.c_str(), L"SCENE.PCK", StrCmp_ToUpper) &&
			     !m_ForbiddenPrivateKey)
		{
			iUnpackObject* Object = new UnpackScene();
			Object->SetFile(FileName);
			WorkerList.push_back(Object);
		}
	};

	if (WorkerThreadHandle != INVALID_HANDLE_VALUE)
	{
		ExitCode = STILL_ACTIVE;
		GetExitCodeThread(WorkerThreadHandle, &ExitCode);
		if (ExitCode == STILL_ACTIVE)
		{
			Timeout.QuadPart = 500;
			NtWaitForSingleObject(WorkerThreadHandle, TRUE, &Timeout);
			GetExitCodeThread(WorkerThreadHandle, &ExitCode);
			if (ExitCode == STILL_ACTIVE)
				Nt_TerminateThread(WorkerThreadHandle, 0);
		}
	}
	NtClose(WorkerThreadHandle);
	WorkerThreadHandle = INVALID_HANDLE_VALUE;
	EnableAll();

	for (auto& Item : WorkerList)
		delete Item;

	WorkerList.clear();
	
	//UpdateWindow();
	DropCount = DragQueryFileW(hDropInfo, -1, NULL, 0);
	for (ULONG i = 0; i< DropCount; i++)
	{
		WCHAR FileName[MAX_PATH];
		
		RtlZeroMemory(FileName, sizeof(FileName));
		DragQueryFileW(hDropInfo, i, FileName, MAX_PATH);
		Attribute = Nt_GetFileAttributes(FileName);
		
		if (Attribute == 0xffffffff)
			continue;

		if (Attribute & FILE_ATTRIBUTE_DIRECTORY)
		{
			Length = StrLengthW(FileName);
			HasSeparator = FALSE;
			if (FileName[Length - 1] == L'\\' || FileName[Length - 1] == L'/')
				HasSeparator = TRUE;

			if (HasSeparator)
				FormatStringW(SearchString, L"%s%s", FileName, L"*.*");
			else
				FormatStringW(SearchString, L"%s\\%s", FileName, L"*.*");

			Handle = FindFirstFileW(SearchString, &NextInfo);
			if (Handle != INVALID_HANDLE_VALUE)
			{
				while (FindNextFileW(Handle, &NextInfo))
				{
					if (NextInfo.cFileName[0] == '.' || (NextInfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
						continue;

					if (HasSeparator)
						FormatStringW(ItemFileName, L"%s%s", FileName, NextInfo.cFileName);
					else
						FormatStringW(ItemFileName, L"%s\\%s", FileName, NextInfo.cFileName);

					CreateAddObject(ItemFileName);
				}
				FindClose(Handle);
			}
			
		}
		else
		{
			CreateAddObject(FileName);
		}
	}
	//CDialogEx::OnDropFiles(hDropInfo);

	Status = Nt_CreateThread(WorkerThreadWarpper, this);
	if (NT_FAILED(Status))
	{
		MessageBoxW(L"Couldn't launch worker thread[warpper thread]!", L"SiglusExtract", MB_OK | MB_ICONERROR);
		return;
	}
}

void CSiglusExtractDialog::InternalReset()
{
	WCHAR FullApplicationTitle[512];

	for (auto& Item : WorkerList)
		delete Item;

	WorkerList.clear();
	FormatStringW(FullApplicationTitle, szApplicationName, szExtractVersion, MAKE_WSTRING(__DATE__) L" " MAKE_WSTRING(__TIME__));
	this->SetWindowTextW(FullApplicationTitle);

	SetProgress(0);
}

bool CSiglusExtractDialog::LoadImageFromResource(CImage* pImage, UINT nResID, LPCWSTR lpTyp)
{
	if (pImage == NULL)
		return false;

	pImage->Destroy();

	HRSRC hRsrc = ::FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(nResID), lpTyp);
	if (hRsrc == NULL) return false;

	HGLOBAL hImgData = ::LoadResource(AfxGetResourceHandle(), hRsrc);
	if (hImgData == NULL)
	{
		::FreeResource(hImgData);
		return false;
	}

	LPVOID lpVoid = ::LockResource(hImgData);

	LPSTREAM pStream = NULL;
	DWORD dwSize = ::SizeofResource(AfxGetResourceHandle(), hRsrc);
	HGLOBAL hNew = ::GlobalAlloc(GHND, dwSize);
	LPBYTE lpByte = (LPBYTE)::GlobalLock(hNew);
	::memcpy(lpByte, lpVoid, dwSize);

	::GlobalUnlock(hNew);

	HRESULT ht = ::CreateStreamOnHGlobal(hNew, TRUE, &pStream);
	if (ht != S_OK)
	{
		GlobalFree(hNew);
	}
	else
	{
		pImage->Load(pStream);
		GlobalFree(hNew);
	}

	::FreeResource(hImgData);

	for (int row = 0; row < pImage->GetWidth(); row++)
	{
		for (int col = 0; col < pImage->GetHeight(); col++)
		{
			UCHAR* cr = (UCHAR*)pImage->GetPixelAddress(row, col);
			cr[0] = cr[0] * cr[3] / 255;
			cr[1] = cr[1] * cr[3] / 255;
			cr[2] = cr[2] * cr[3] / 255;
		}
	}
	return true;
}


BOOL CSiglusExtractDialog::OnInitDialog()
{
	ULONG Longs;

	CDialogEx::OnInitDialog();

	InternalReset();

	m_Menu.LoadMenuW(IDR_MENU1);
	SetMenu(&m_Menu);


	LoadImageFromResource(&m_ImageBg, IDB_PNG3, _TEXT("PNG"));

	/*
	Longs = GetWindowLongW(GetSafeHwnd(), GWL_EXSTYLE);
	Longs |= WS_EX_LAYERED;
	SetWindowLongW(GetSafeHwnd(), GWL_EXSTYLE, Longs);
	SetLayeredWindowAttributes(RGB(0, 0, 0), 200, LWA_ALPHA);
	*/

	CButton* RadioItem = NULL;

	RadioItem = (CButton*)GetDlgItem(IDC_GOO_BMP);
	if (RadioItem) RadioItem->SetCheck(TRUE);

	RadioItem = (CButton*)GetDlgItem(IDC_NWA_WAV);
	if (RadioItem) RadioItem->SetCheck(TRUE);

	RadioItem = (CButton*)GetDlgItem(IDC_OGG_DEC);
	if (RadioItem) RadioItem->SetCheck(TRUE);

	RadioItem = (CButton*)GetDlgItem(IDC_OMV_OGV);
	if (RadioItem) RadioItem->SetCheck(TRUE);

	RadioItem = (CButton*)GetDlgItem(IDC_TEXT_V2);
	if (RadioItem) RadioItem->SetCheck(TRUE);

	RadioItem = (CButton*)GetDlgItem(IDC_ICON_CHECK);
	if (RadioItem) RadioItem->SetCheck(TRUE);

	CEdit* TextItem = (CEdit*)GetDlgItem(IDC_SCE_EDIT2);
	TextItem->SetWindowTextW(L"Scene.pck.new");


	if (m_ForbiddenPrivateKey)
	{
		CWnd* Item = NULL;

		Item = GetDlgItem(IDC_GAMEEXE_SEL);
		if (Item) Item->EnableWindow(FALSE);

		Item = GetDlgItem(IDC_GAMEEXE_EDIT);
		if (Item) Item->EnableWindow(FALSE);

		Item = GetDlgItem(IDC_GAMEEXE_PACK);
		if (Item) Item->EnableWindow(FALSE);

		Item = GetDlgItem(IDC_TEXT_V1);
		if (Item) Item->EnableWindow(FALSE);

		Item = GetDlgItem(IDC_TEXT_V2);
		if (Item) Item->EnableWindow(FALSE);

		Item = GetDlgItem(IDC_SCE_SEL);
		if (Item) Item->EnableWindow(FALSE);

		Item = GetDlgItem(IDC_SCE_PACK);
		if (Item) Item->EnableWindow(FALSE);

		Item = GetDlgItem(IDC_SCE_EDIT2);
		if (Item) Item->EnableWindow(FALSE);

		Item = GetDlgItem(IDC_SCE_EDIT);
		if (Item) Item->EnableWindow(FALSE);
	}


	BringWindowToTop();
	SetForegroundWindow();
	//ShowWindow(SW_SHOW);

	return TRUE;
}


void CSiglusExtractDialog::DisableAll()
{
	CWnd* Item = NULL;

	Item = GetDlgItem(IDC_GOO_BMP);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_G00_PNG);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_G00_JPG);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_NWA_WAV);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_NWA_FLAC);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_NWA_OGG);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_OGG_DEC);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_OGG_RAW);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_SCE_SEL);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_SCE_PACK);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_SCE_EDIT2);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_SCE_EDIT);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_OMV_OGV);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_OMV_PNG);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_GAMEEXE_SEL);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_GAMEEXE_EDIT);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_GAMEEXE_PACK);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_TEXT_V1);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_TEXT_V2);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_PATCH_BUTTON);
	if (Item) Item->EnableWindow(FALSE);

	Item = GetDlgItem(IDC_ICON_CHECK);
	if (Item) Item->EnableWindow(FALSE);
}


void CSiglusExtractDialog::EnableAll()
{
	CWnd* Item = NULL;

	Item = GetDlgItem(IDC_GOO_BMP);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_G00_PNG);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_G00_JPG);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_NWA_WAV);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_NWA_FLAC);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_NWA_OGG);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_OGG_DEC);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_OGG_RAW);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_SCE_SEL);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_SCE_PACK);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_SCE_EDIT2);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_SCE_EDIT);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_OMV_OGV);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_OMV_PNG);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_GAMEEXE_SEL);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_GAMEEXE_EDIT);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_GAMEEXE_PACK);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_TEXT_V1);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_TEXT_V2);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_PATCH_BUTTON);
	if (Item) Item->EnableWindow(TRUE);

	Item = GetDlgItem(IDC_ICON_CHECK);
	if (Item) Item->EnableWindow(TRUE);

	if (m_ForbiddenPrivateKey)
	{
		CWnd* Item = NULL;

		Item = GetDlgItem(IDC_GAMEEXE_SEL);
		if (Item) Item->EnableWindow(FALSE);

		Item = GetDlgItem(IDC_GAMEEXE_EDIT);
		if (Item) Item->EnableWindow(FALSE);

		Item = GetDlgItem(IDC_GAMEEXE_PACK);
		if (Item) Item->EnableWindow(FALSE);

		Item = GetDlgItem(IDC_TEXT_V1);
		if (Item) Item->EnableWindow(FALSE);

		Item = GetDlgItem(IDC_TEXT_V2);
		if (Item) Item->EnableWindow(FALSE);

		Item = GetDlgItem(IDC_SCE_SEL);
		if (Item) Item->EnableWindow(FALSE);

		Item = GetDlgItem(IDC_SCE_PACK);
		if (Item) Item->EnableWindow(FALSE);

		Item = GetDlgItem(IDC_SCE_EDIT2);
		if (Item) Item->EnableWindow(FALSE);

		Item = GetDlgItem(IDC_SCE_EDIT);
		if (Item) Item->EnableWindow(FALSE);
	}
}

void CSiglusExtractDialog::PckAndDatStatus(BOOL Pck, BOOL Dat)
{
	m_PckNeedKey = Pck;
	m_DatNeedKey = Dat;
}

void CSiglusExtractDialog::SetProgress(ULONG Value)
{
	CProgressCtrl* Item = (CProgressCtrl*)GetDlgItem(IDC_PROGRESS1);

	if (Item)
	{
		Item->SetPos(Value);
	}
}

DWORD NTAPI WorkerThreadWarpper(PVOID UserData)
{
	NTSTATUS              Status;
	CSiglusExtractDialog* Data;

	Data = (CSiglusExtractDialog*)UserData;
	Data->DisableAll();
	Status = Nt_CreateThread(WorkerThread, UserData, FALSE, NtCurrentProcess(), &(Data->WorkerThreadHandle));
	if (NT_FAILED(Status))
	{
		Data->MessageBoxW(L"Couldn't launch worker thread!", L"SiglusExtract", MB_OK | MB_ICONERROR);
		return STATUS_INVALID_THREAD;
	}

	WaitForSingleObject(Data->WorkerThreadHandle, INFINITE);
	NtClose(Data->WorkerThreadHandle);
	Data->WorkerThreadHandle = INVALID_HANDLE_VALUE;
	Data->InternalReset();
	Data->EnableAll();
	//UpdateWindow(Data->GetSafeHwnd());

	return 0;
}

DWORD NTAPI WorkerThread(PVOID UserData)
{
	NTSTATUS              Status;
	CSiglusExtractDialog* Data;
	DecodeControl         Control;
	WCHAR                 WindowText[MAX_PATH];

	Data = (CSiglusExtractDialog*)UserData;
	Control.G00Flag = Data->m_G00Flag;
	Control.NWAFlag = Data->m_NWAFlag;
	Control.OGGFlag = Data->m_OGGFlag;
	Control.OGVFlag = Data->m_OGVFlag;
	Control.SSDecode= Data->m_SSFlag;
	Control.PckNeedKey = Data->m_PckNeedKey;
	Control.DatNeedKey = Data->m_DatNeedKey;

	RtlCopyMemory(Control.PrivateKey, Data->m_PrivateKey, sizeof(Control.PrivateKey));
	
	for (ULONG i = 0; i < Data->WorkerList.size(); i++)
	{
		FormatStringW(WindowText, L"[Unpacking] %d/%d", i + 1, Data->WorkerList.size());
		Data->SetWindowTextW(WindowText);
		Data->SetProgress((ULONG)((float)(i + 1) / (float)(Data->WorkerList.size()) * 100.0));
		Status = Data->WorkerList[i]->Unpack(&Control);
	}

	Data->EnableAll();
	return 0;
}

void CSiglusExtractDialog::OnClose()
{
	//CDialogEx::OnClose();
	Ps::ExitProcess(0);
}

void CSiglusExtractDialog::SetPrivateKey(PBYTE Buffer)
{
	RtlCopyMemory(m_PrivateKey, Buffer, sizeof(m_PrivateKey));
}


//选择txt or ini文件
void CSiglusExtractDialog::OnBnClickedGameexeSel()
{
	NTSTATUS Status;
	CString  FileName;
	WCHAR    OutputInfo[400];
	WCHAR    Filter[] = _T("Text Files(*.txt)|*.txt|All Files(*.*)|*.*||");

	CFileDialog FileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, Filter, this);
	if (FileDlg.DoModal() == IDOK)
	{
		FileName = FileDlg.GetPathName();
	}

	Status = CreateGameexe(FileName, m_PrivateKey, m_DatNeedKey);
	switch (Status)
	{
	case STATUS_NO_SUCH_FILE:
		FormatStringW(OutputInfo, L"Couldn't open configuration file[%s]", FileName.GetBuffer());
		MessageBoxW(OutputInfo, L"SiglusExtract", MB_OK | MB_ICONERROR);
		break;

	case STATUS_INVALID_PARAMETER:
		MessageBoxW(L"Couldn't save file to [Gameexe.new.dat]", L"SiglusExtract", MB_OK | MB_ICONERROR);
		break;

	case STATUS_SUCCESS:
		MessageBoxW(L"Save file to [Gameexe.new.dat] Successfully!", L"SiglusExtract", MB_OK);
		break; 

	default:
		MessageBoxW(L"Unknown error!", L"SiglusExtract", MB_OK | MB_ICONERROR);
		break;
	}
}


EXTC ULONG
InternalCopyUnicodeString(
PUNICODE_STRING Unicode,
PWCHAR          Buffer,
ULONG_PTR       BufferCount,
BOOL            IsDirectory = FALSE
);


IStream* LoadFromResource(UINT nResID, LPCWSTR lpTyp, BOOL Inject, HWND Handle)
{
	WCHAR          ExeFileBaseName[MAX_PATH];


	auto Nt_GetModuleFileBaseName = [](PVOID ModuleBase, LPWSTR Filename, ULONG_PTR BufferCount)->ULONG_PTR
	{
		ULONG_PTR               Length;
		PEB_BASE               *Peb;
		PLDR_DATA_TABLE_ENTRY   LdrModule, FirstLdrModule;

		Peb = Nt_CurrentPeb();
		LdrModule = FIELD_BASE(Peb->Ldr->InLoadOrderModuleList.Flink, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		FirstLdrModule = LdrModule;

		if (ModuleBase == NULL)
			ModuleBase = Peb->ImageBaseAddress;

		LOOP_FOREVER
		{
			if ((ULONG_PTR)ModuleBase >= (ULONG_PTR)LdrModule->DllBase &&
			(ULONG_PTR)ModuleBase < (ULONG_PTR)LdrModule->DllBase + LdrModule->SizeOfImage)
			{
				break;
			}

			LdrModule = (PLDR_DATA_TABLE_ENTRY)LdrModule->InLoadOrderLinks.Flink;
			if (LdrModule == FirstLdrModule)
				return 0;
		}


		Length = LdrModule->BaseDllName.Length;
		RtlCopyMemory(Filename, LdrModule->BaseDllName.Buffer, (LdrModule->BaseDllName.Length + 1) * 2);

		return Length;
	};

	HRSRC hRsrc = ::FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(nResID), lpTyp);
	if (hRsrc == NULL)
		return NULL;

	HGLOBAL hImgData = ::LoadResource(AfxGetResourceHandle(), hRsrc);
	if (hImgData == NULL)
	{
		::FreeResource(hImgData);
		return NULL;
	}

	LPVOID lpVoid = ::LockResource(hImgData);

	LPSTREAM pStream = NULL;
	DWORD dwSize = ::SizeofResource(AfxGetResourceHandle(), hRsrc);
	HGLOBAL hNew = ::GlobalAlloc(GHND, dwSize);
	LPBYTE lpByte = (LPBYTE)::GlobalLock(hNew);
	RtlCopyMemory(lpByte, lpVoid, dwSize);


	if (Inject)
	{
		static CHAR KeyWord[] = "SiglInfoXmoeAnzu";

		PVOID ShellData;

		ShellData = KMP(lpByte, dwSize, KeyWord, CONST_STRLEN(KeyWord));
		if (ShellData == NULL)
		{
			::MessageBoxW(Handle, L"Universal patch : Internal Error", L"SiglusExtract", MB_OK);
			return NULL;
		}
		
		Nt_GetModuleFileBaseName(Nt_GetExeModuleHandle(), ExeFileBaseName, countof(ExeFileBaseName));

		RtlZeroMemory(ShellData, 0x10);
		RtlCopyMemory((PBYTE)ShellData + 0x10, ExeFileBaseName, StrLengthW(ExeFileBaseName) * sizeof(ExeFileBaseName[0]));

	}

	::GlobalUnlock(hNew);

	if (FAILED(CreateStreamOnHGlobal(hNew, TRUE, &pStream)))
	{
		GlobalFree(hNew);
		return NULL;
	}
	::FreeResource(hImgData);

	return pStream;
}


#pragma pack(push, 2)
typedef struct {
	BYTE Width;           // icon width (32)
	BYTE Height;          // icon height (32)
	BYTE Colors;          // colors (0 means more than 8 bits per pixel)
	BYTE Reserved2;       // reserved, must be 0
	WORD Planes;          // color planes
	WORD BitsPerPixel;    // bit depth
	DWORD ImageSize;      // size of structure
	WORD ResourceID;      // resource ID
} GROUPICON;
#pragma pack(pop)


#pragma pack(push, 1)

typedef struct
{
	BYTE    Width;          // Width, in pixels, of the image
	BYTE    Height;         // Height, in pixels, of the image
	BYTE    ColorCount;     // Number of colors in image (0 if >=8bpp)
	BYTE    Reserved;       // Reserved
	WORD    Planes;         // Color Planes
	WORD    BitCount;       // Bits per pixel
	DWORD   ImageSize;     // how many bytes in this resource?
	DWORD   ImageOffset;  

} MY_ICON_ENTRY, *PMY_ICON_ENTRY;

typedef struct
{
	WORD                Reserved;   // Reserved (must be 0)
	WORD                Type;       // Resource type (1 for icons)
	WORD                Count;      // How many images?

} MY_ICON, *PMY_ICON;

#pragma pack(pop)

//Make Universal Patch
void CSiglusExtractDialog::OnBnClickedPatchButton()
{
	NTSTATUS         Status;
	IStream*         Stream;
	NtFileDisk       File;
	WCHAR            ExeFileName[MAX_PATH];
	STATSTG          Stat;
	PBYTE            Buffer;
	ULONG            Size, BytesRead;
	HANDLE           Handle;
	WIN32_FIND_DATAW Data;

	auto GetNameFileName = [](LPCWSTR FileName)->std::wstring
	{
		std::wstring PeFileName(FileName);
		auto Index = PeFileName.find_last_of(L'.');
		if (Index == std::wstring::npos)
			return PeFileName + L"_Patch";
		else
			return PeFileName.substr(0, Index) + L"_Patch.exe";
	};

	Stream = LoadFromResource(IDR_EXE1, L"EXE", TRUE, GetSafeHwnd());
	if (!Stream)
		return;

	Nt_GetModuleFileName(Nt_GetExeModuleHandle(), ExeFileName, countof(ExeFileName));
	Status = File.Create(GetNameFileName(ExeFileName).c_str());
	if (NT_FAILED(Status))
	{
		MessageBoxW(L"Universal patch : Couldn't write file", L"SiglusExtract", MB_OK | MB_ICONERROR);
		Stream->Release();
	}
	Stream->Stat(&Stat, STATFLAG_DEFAULT);
	Size = Stat.cbSize.LowPart;
	Buffer = (PBYTE)AllocateMemoryP(Size);
	Stream->Read(Buffer, Size, &BytesRead);
	File.Write(Buffer, Size);
	File.Close();
	Stream->Release();

	Stream = LoadFromResource(IDR_DLL1, L"DLL", FALSE, GetSafeHwnd());
	if (!Stream)
		return;

	Status = File.Create(L"SiglusUniversalPatch.dll");
	if (NT_FAILED(Status))
	{
		MessageBoxW(L"Universal patch : Couldn't write file", L"SiglusExtract", MB_OK | MB_ICONERROR);
		Stream->Release();
	}
	Stream->Stat(&Stat, STATFLAG_DEFAULT);
	Size = Stat.cbSize.LowPart;
	Buffer = (PBYTE)ReAllocateMemoryP(Buffer, Size);
	Stream->Read(Buffer, Size, &BytesRead);
	File.Write(Buffer, Size);
	File.Close();
	Stream->Release();
	FreeMemoryP(Buffer);

	Handle = Nt_FindFirstFile(L"*.ico", &Data);
	if (m_InheritIcon && (Handle != INVALID_HANDLE_VALUE))
	{
		auto InjectMainIcon = [](LPCWSTR Where, LPCWSTR What)
		{
			HANDLE hWhere = BeginUpdateResourceW(Where, FALSE);

			NTSTATUS   Status;
			PBYTE      buffer;    // buffer to store raw icon data
			ULONG      buffersize; // length of buffer
			NtFileDisk IconFile;       // file handle

			Status = IconFile.Open(What);
			if (NT_FAILED(Status))
				return; // if file doesn't exist, can't be opened etc.

			// calculate buffer length and load file into buffer
			buffersize = IconFile.GetSize32();
			buffer = (PBYTE)AllocateMemoryP(buffersize);
			if (!buffer)
			{
				IconFile.Close();
				return;
			}
			IconFile.Read(buffer, buffersize);
			IconFile.Close();

			PMY_ICON Header = (PMY_ICON)buffer;

			PBYTE UpdateData = (PBYTE)AllocateMemoryP(sizeof(MY_ICON) + sizeof(GROUPICON) * Header->Count);
			PMY_ICON UpdateHeader = (PMY_ICON)UpdateData;

			UpdateHeader->Reserved = 0;
			UpdateHeader->Type = 1;
			UpdateHeader->Count = Header->Count;

			for (ULONG i = 1; i <= Header->Count; i++)
			{
				PMY_ICON_ENTRY Entry = (PMY_ICON_ENTRY)(buffer + sizeof(MY_ICON) + sizeof(MY_ICON_ENTRY) * (i - 1));

				UpdateResourceW(
					hWhere,  // Handle to executable
					RT_ICON, // Resource type - icon
					MAKEINTRESOURCE(i + 114514), // Make the id 1
					MAKELANGID(LANG_ENGLISH,
					SUBLANG_DEFAULT), // Default language
					buffer + Entry->ImageOffset,
					Entry->ImageSize
					);


				GROUPICON* Entry2 = (GROUPICON*)(UpdateData + sizeof(MY_ICON) + sizeof(GROUPICON) * (i - 1));

				Entry2->Width = Entry->Width;        // icon width (32)
				Entry2->Height = Entry->Height;       // icon height (32)
				Entry2->Colors = Entry->ColorCount;        // colors (256)
				Entry2->Reserved2 = 0;     // reserved, must be 0
				Entry2->Planes = Entry->Planes;        // color planes
				Entry2->BitsPerPixel = Entry->BitCount; // bit depth
				Entry2->ImageSize = Entry->ImageSize; // size of image
				Entry2->ResourceID = (WORD)i + 114514;       // resource ID is 1
			}

			UpdateResourceW(
				hWhere,
				RT_GROUP_ICON,
				// RT_GROUP_ICON resources contain information
				// about stored icons
				L"MAINICON",
				// MAINICON contains information about the
				// application's displayed icon
				MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
				UpdateHeader,
				// Pointer to this structure
				sizeof(MY_ICON) + sizeof(GROUPICON) * Header->Count
				);

			// Perform the update, don't discard changes
			EndUpdateResourceW(hWhere, FALSE);

			FreeMemoryP(buffer);
			FreeMemoryP(UpdateData);
			
		};

		Nt_FindClose(Handle);
		InjectMainIcon(GetNameFileName(ExeFileName).c_str(), Data.cFileName);
	}

	MessageBoxW(L"Make universal patch : Success", L"SiglusExtract", MB_OK);
}


void CSiglusExtractDialog::OnFeedbackDeveloperWeibo()
{
	ShellExecuteW(NULL, _T("open"), L"http://weibo.com/xmoeosu", NULL, NULL, SW_SHOW);
}


void CSiglusExtractDialog::OnFeedbackDeveloperTwitter()
{
	ShellExecuteW(NULL, _T("open"), L"https://twitter.com/KaedeXmoe", NULL, NULL, SW_SHOW);
}



void CSiglusExtractDialog::OnAbout()
{
	if (m_AboutDlg.m_IsInited)
	{
		m_AboutDlg.ShowWindow(SW_SHOW);
		m_AboutDlg.BringWindowToTop();
		m_AboutDlg.SetForegroundWindow();
	}
	else
	{
		m_AboutDlg.DoModal();
	}
}


void CSiglusExtractDialog::OnFeedbackSendfeedback()
{
#if 0
	if (m_FeedbackDlg.m_IsInited && m_FeedbackDlg.GetSafeHwnd())
	{
		m_FeedbackDlg.ShowWindow(SW_SHOW);
		m_FeedbackDlg.BringWindowToTop();
		m_FeedbackDlg.SetForegroundWindow();
	}
	else
	{
		m_FeedbackDlg.DoModal();
	}
#else

	MessageBoxW(L"This feature now is disabled.", L"SiglusExtract", MB_OK | MB_ICONWARNING);
#endif
}


BOOL CSiglusExtractDialog::OnEraseBkgnd(CDC* pDC)
{
	return CDialogEx::OnEraseBkgnd(pDC);
}


void CSiglusExtractDialog::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); 

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
	}
	else
	{
		CPaintDC dc(this);

		POINT Pos;
		Pos.x = Pos.y = 0;

#if 0

		//m_ImageBg.SetHasAlphaChannel(TRUE);
		//m_ImageBg.AlphaBlend(GetDC()->GetSafeHdc(), 0, 0, 0x7f, AC_SRC_OVER);
		m_ImageBg.Draw(GetDC()->GetSafeHdc(), Pos);
#else
		CRect      rect;
		this->GetClientRect(&rect);

		int imageW = m_ImageBg.GetWidth();
		int imageH = m_ImageBg.GetHeight();

		CImage memImg;                            
		memImg.Create(rect.Width(), rect.Height(), m_ImageBg.GetBPP());
		HDC tmpdc = memImg.GetDC();
		CDC memDC;
		memDC.Attach(tmpdc);

		memDC.FillSolidRect(rect, RGB(255, 255, 255));
		m_ImageBg.Draw(tmpdc, Pos);

		memImg.Draw(dc.m_hDC, 0, 0, rect.Width(), rect.Height());

		memDC.Detach();    
		memImg.ReleaseDC();
#endif
	}
}


HBRUSH CSiglusExtractDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (nCtlColor == CTLCOLOR_STATIC || nCtlColor == CTLCOLOR_BTN || nCtlColor == CTLCOLOR_LISTBOX)
	{
		pDC->SetBkColor(RGB(0, 0, 0));
		pDC->SetBkMode(TRANSPARENT);
		hbr = (HBRUSH)::GetStockObject(HOLLOW_BRUSH);
	}

	CRect rc;
	pWnd->GetWindowRect(&rc);
	ScreenToClient(&rc);
	CDC* dc = GetDC();
	pDC->BitBlt(0, 0, rc.Width(), rc.Height(), dc, rc.left, rc.top, SRCCOPY);
	ReleaseDC(dc);

	return hbr;
}
