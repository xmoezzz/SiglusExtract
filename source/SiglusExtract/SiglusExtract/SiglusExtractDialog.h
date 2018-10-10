#pragma once


#include <afx.h>
#include <afxdialogex.h>
#include <afxwin.h>
#include "SiglusExtractAboutDialog.h"
#include "SiglusExtractFeedbackDialog.h"
#include "resource.h"
#include <vector>
#include "iUnpackObject.h"
#include "DecodeControl.h"


#define szApplicationName   L"[X'moe]Welcome to SiglusExtract(version : %s, built on : %s)"
#define szExtractVersion    L"Ver 0.1.0.2"
// CSiglusExtractDialog 对话框

class CSiglusExtractDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CSiglusExtractDialog)

public:
	CSiglusExtractDialog(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSiglusExtractDialog();

	virtual void OnFinalRelease();

// 对话框数据
	enum { IDD = IDD_MAIN_DIALOG };


	std::vector<iUnpackObject*> WorkerList;
	G00Decode  m_G00Flag;
	NWADecode  m_NWAFlag;
	OGGDecode  m_OGGFlag;
	OGVDecode  m_OGVFlag;
	SSDecode   m_SSFlag;

	BOOL       m_InheritIcon;
	HANDLE     WorkerThreadHandle;
	BYTE       m_PrivateKey[16];
	BOOL       m_PckNeedKey;
	BOOL       m_DatNeedKey;
	BOOL       m_ForbiddenPrivateKey;
	CMenu      m_Menu;
	CImage     m_ImageBg;
	CSiglusExtractAboutDialog m_AboutDlg;
	CSiglusExtractFeedbackDialog m_FeedbackDlg;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);   

public:
	void EnableAll();
	void DisableAll();
	void InternalReset();
	void SetProgress(ULONG Value);
	void SetPrivateKey(PBYTE Buffer);
	void PckAndDatStatus(BOOL Pck, BOOL Dat);
	void ForbiddenPrivateKey();


	bool LoadImageFromResource(CImage* pImage, UINT nResID, LPCWSTR lpTyp);

	DECLARE_MESSAGE_MAP()
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
public:
	afx_msg void OnBnClickedGooBmp();
	afx_msg void OnBnClickedG00Png();
	afx_msg void OnBnClickedG00Jpg();
	afx_msg void OnBnClickedNwaWav();
	afx_msg void OnBnClickedNwaFlac();
	afx_msg void OnBnClickedNwaOgg();
	afx_msg void OnBnClickedOggDec();
	afx_msg void OnBnClickedOggRaw();
	afx_msg void OnBnClickedOmvOgv();
	afx_msg void OnBnClickedOmvPng();
	afx_msg void OnEnChangeSceEdit();
	afx_msg void OnBnClickedTextV1();
	afx_msg void OnBnClickedTextV2();
	afx_msg void OnBnClickedIconCheck();
	afx_msg void OnBnClickedSceSel();
	afx_msg void OnBnClickedScePack();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnBnClickedGameexeSel();
	afx_msg void OnBnClickedPatchButton();
	afx_msg void OnFeedbackDeveloperWeibo();
	afx_msg void OnFeedbackDeveloperTwitter();
	afx_msg void OnAbout();
	afx_msg void OnFeedbackSendfeedback();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
