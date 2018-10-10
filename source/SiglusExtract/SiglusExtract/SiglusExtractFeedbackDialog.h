#pragma once

#include "resource.h"
#include "afxcmn.h"

// CSiglusExtractFeedbackDialog 对话框

class CSiglusExtractFeedbackDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CSiglusExtractFeedbackDialog)

public:
	CSiglusExtractFeedbackDialog(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSiglusExtractFeedbackDialog();

// 对话框数据
	enum { IDD = IDD_FEEDBACK_DIALOG };

	BOOL m_IsInited;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnBnClickedOk();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CRichEditCtrl m_markEditNote;
};
