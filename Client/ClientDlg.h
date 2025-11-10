
// ClientDlg.h: 헤더 파일
//

#pragma once
#include "ClientSocket.h"


// CClientDlg 대화 상자
class CClientDlg : public CDialogEx
{
	// 생성입니다.
public:
	CClientDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CLIENT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다

protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CEdit m_edit_send;
	CListBox m_list_message;
//	CStatic m_static_status;
	afx_msg void OnBnClickedButtonConnect();
	afx_msg void OnBnClickedButtonSend();
	//  통신 클래스 포인터
	CClientSocket* m_pClientSocket;
	//  메시지 출력 함수 (소켓 클래스에서 호출될 함수)
	void DisplayMessage(const CString& strSender, const CString& strMsg, BOOL bReceived);
	CStatic m_static_status;

	//-------------------
	void RequestMessage(CString& strMsg);
	CString m_strName;
};
