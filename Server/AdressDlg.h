#pragma once
#include "afxdialogex.h"


// CAdressDlg 대화 상자

class CAdressDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CAdressDlg)

public:
	CAdressDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CAdressDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_ADDRESS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:

	CString m_strName;
	CString m_strIPAddress;
	CIPAddressCtrl m_ipaddress_ip;
	afx_msg void OnBnClickedOk();
};
