#pragma once



#include "resource.h"

// CSiglusExtractInitDialog 对话框

class CSiglusExtractInitDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CSiglusExtractInitDialog)

public:
	CSiglusExtractInitDialog(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSiglusExtractInitDialog();

// 对话框数据
	enum { IDD = IDD_INIT_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};

