#pragma once
#include "afxdialogex.h"


// CAddressDlg 대화 상자

class CAddressDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CAddressDlg)

public:
	CAddressDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CAddressDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_ADDRESS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	CString m_strName;
	CIPAddressCtrl m_ipaddress_ip;
	CString m_strIPAddress;
	afx_msg void OnBnClickedOk();
};
