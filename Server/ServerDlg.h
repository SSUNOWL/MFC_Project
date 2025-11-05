
// ServerDlg.h: 헤더 파일
//

#pragma once
#include "ListenSocket.h"
#include "ServiceSocket.h"

// CServerDlg 대화 상자
class CServerDlg : public CDialogEx
{
// 생성입니다.
public:
	CServerDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	virtual ~CServerDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SERVER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButtonSend();
	CListBox m_list_log;
	afx_msg void OnBnClickedButtonStart();
	//  1. 소켓 객체 포인터
	CListenSocket* m_pListenSocket;
//  2. 연결된 클라이언트 소켓 목록
	CList<CServiceSocket*, CServiceSocket*> m_clientSocketList;
//  4. 소켓 콜백에서 호출될 핵심 관리 함수들
	// 클라이언트 연결 수락 처리 (CListenSocket::OnAccept에서 호출)
	void ProcessAccept(CListenSocket* pListenSocket);

	// 클라이언트 연결 종료 처리 (CServiceSocket::OnClose에서 호출)
	void RemoveClient(CServiceSocket* pServiceSocket);

	// 메시지 전체 클라이언트에게 전송 (CServiceSocket::OnReceive에서 호출)
	void BroadcastMessage(const CString& strType, const CString& strSender, const CString& strMsg, CServiceSocket* pSender);

	void AddLog(const CString& strMsg);
	void DisplayMessage(const CString& strSender, const CString& strMsg, BOOL bReceived);
	CEdit m_edit_send;
//	CListBox m_list_Message;
	CListBox m_list_message;
};
