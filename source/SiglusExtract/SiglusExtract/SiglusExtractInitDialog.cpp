// SiglusExtractInitDialog.cpp : 实现文件
//

#include "stdafx.h"
#include "SiglusExtractInitDialog.h"
#include "afxdialogex.h"


// CSiglusExtractInitDialog 对话框

IMPLEMENT_DYNAMIC(CSiglusExtractInitDialog, CDialogEx)

CSiglusExtractInitDialog::CSiglusExtractInitDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSiglusExtractInitDialog::IDD, pParent)
{

}

CSiglusExtractInitDialog::~CSiglusExtractInitDialog()
{
}

void CSiglusExtractInitDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSiglusExtractInitDialog, CDialogEx)
END_MESSAGE_MAP()


// CSiglusExtractInitDialog 消息处理程序


BOOL CSiglusExtractInitDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CProgressCtrl* Item = (CProgressCtrl*)GetDlgItem(IDC_PROGRESS1);
	Item->SetMarquee(TRUE, 30);

	return TRUE; 
}
