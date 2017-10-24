// SiglusExtractAboutDialog.cpp : 实现文件
//

#include "stdafx.h"
#include "SiglusExtractAboutDialog.h"
#include "afxdialogex.h"
#include "resource.h"


// CSiglusExtractAboutDialog 对话框

IMPLEMENT_DYNAMIC(CSiglusExtractAboutDialog, CDialogEx)

CSiglusExtractAboutDialog::CSiglusExtractAboutDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSiglusExtractAboutDialog::IDD, pParent)
{
	m_IsInited = FALSE;

}

CSiglusExtractAboutDialog::~CSiglusExtractAboutDialog()
{
}

void CSiglusExtractAboutDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON1, m_CloseBtn);
}


bool CSiglusExtractAboutDialog::LoadImageFromResource(CImage* pImage, UINT nResID, LPCWSTR lpTyp)
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

	// 锁定内存中的指定资源
	LPVOID lpVoid = ::LockResource(hImgData);

	LPSTREAM pStream = NULL;
	DWORD dwSize = ::SizeofResource(AfxGetResourceHandle(), hRsrc);
	HGLOBAL hNew = ::GlobalAlloc(GHND, dwSize);
	LPBYTE lpByte = (LPBYTE)::GlobalLock(hNew);
	::memcpy(lpByte, lpVoid, dwSize);

	// 解除内存中的指定资源
	::GlobalUnlock(hNew);

	// 从指定内存创建流对象
	HRESULT ht = ::CreateStreamOnHGlobal(hNew, TRUE, &pStream);
	if (ht != S_OK)
	{
		GlobalFree(hNew);
	}
	else
	{
		// 加载图片
		pImage->Load(pStream);

		GlobalFree(hNew);
	}

	// 释放资源
	::FreeResource(hImgData);

	// 像素转换，由于CImage 中没有对alpha 进行处理
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


void CSiglusExtractAboutDialog::CreateRgnByImage(CImage* pImage, CRgn &rgn)
{
	rgn.CreateRectRgn(0, 0, 0, 0);
	CRgn tempRgn;
	for (UINT h = 0; h < (UINT)pImage->GetHeight(); ++h)
	{
		UINT w = 0;
		while (w < (UINT)m_ImageBg.GetWidth())
		{
			UINT leftX = w;
			while (w < (UINT)m_ImageBg.GetWidth())
			{
				UCHAR* cr = (UCHAR*)pImage->GetPixelAddress(w, h);
				if (cr[3] != 0)
					break;
				leftX = w;
				w++;
			}
			while (w < (UINT)m_ImageBg.GetWidth())
			{
				UCHAR* cr = (UCHAR*)pImage->GetPixelAddress(w, h);
				if (cr[3] == 0)
					break;
				w++;
			}
			tempRgn.CreateRectRgn(leftX, h, w - 1, h + 1);
			rgn.CombineRgn(&rgn, &tempRgn, RGN_OR);
			tempRgn.DeleteObject();
		}
	}
}

BEGIN_MESSAGE_MAP(CSiglusExtractAboutDialog, CDialogEx)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_CTLCOLOR()
	ON_WM_PAINT()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON1, &CSiglusExtractAboutDialog::OnBnClickedButton1)
END_MESSAGE_MAP()


// CSiglusExtractAboutDialog 消息处理程序


BOOL CSiglusExtractAboutDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	LoadImageFromResource(&m_ImageBg, IDB_PNG1, _TEXT("PNG"));

	int width = m_ImageBg.GetWidth();
	int height = m_ImageBg.GetHeight();

	int x = 230;
	int y = 0;

	CRect bRect(x, y, x + 150, y + 29);

	m_CloseBtn.Create(NULL, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, bRect, this, IDC_BUTTON1);
	m_CloseBtn.LoadBitmaps(IDB_BITMAP1, IDB_BITMAP2, IDB_BITMAP3, IDB_BITMAP4);
	m_CloseBtn.SizeToContent();

	CRgn bRgn;
	bRgn.CreateRectRgn(bRect.left, bRect.top, bRect.right, bRect.bottom);

	CRgn rgn;
	CreateRgnByImage(&m_ImageBg, rgn);

	rgn.CombineRgn(&rgn, &bRgn, RGN_OR);

	CRect rc;
	GetWindowRect(&rc);
	int nScreenWidth = GetSystemMetrics(SM_CXFULLSCREEN);
	int nScreenHeight = GetSystemMetrics(SM_CYFULLSCREEN);
	if (nScreenWidth > width)
		rc.left = (nScreenWidth - width) / 2;
	if (nScreenHeight > height)
		rc.top = (nScreenHeight - height) / 2;
	rc.right = rc.left + width;
	rc.bottom = rc.top + height;

	rc.right += 50;

	MoveWindow(&rc);
	SetWindowRgn(rgn, TRUE);


	m_IsInited = TRUE;

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常:  OCX 属性页应返回 FALSE
}


BOOL CSiglusExtractAboutDialog::OnEraseBkgnd(CDC* pDC)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	//return CDialogEx::OnEraseBkgnd(pDC);
	m_ImageBg.AlphaBlend(pDC->GetSafeHdc(), 0, 0);
	pDC->SetBkMode(TRANSPARENT);


#if 0
	CDC dcCompatible;
	dcCompatible.CreateCompatibleDC(pDC); 
	dcCompatible.SelectObject(&m_ImageBg);
	CRect rc;
	GetClientRect(&rc);
	//m_ImageBg.StretchBlt(dcCompatible.GetSafeHdc(), 0, 0, rc.Width(), rc.Height(), 0, 0, m_ImageBg.GetWidth(), m_ImageBg.GetHeight(), SRCCOPY);
	pDC->StretchBlt(0, 0, rc.Width(), rc.Height(), &dcCompatible, 0, 0, m_ImageBg.GetWidth(), m_ImageBg.GetHeight(), SRCCOPY);
	dcCompatible.DeleteDC();
#endif

	return TRUE;
}


void CSiglusExtractAboutDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnLButtonDown(nFlags, point);
	PostMessageW(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
}


HBRUSH CSiglusExtractAboutDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (nCtlColor == CTLCOLOR_STATIC)
	{
		pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)::GetStockObject(NULL_BRUSH);
	}

	return hbr;
}


void CSiglusExtractAboutDialog::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO:  在此处添加消息处理程序代码
	// 不为绘图消息调用 CDialogEx::OnPaint()
}


void CSiglusExtractAboutDialog::OnClose()
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnClose();
}


void CSiglusExtractAboutDialog::OnBnClickedButton1()
{
	// TODO:  在此添加控件通知处理程序代码
	m_IsInited = FALSE;
	EndDialog(0);
}
