// ListenSocket.cpp

#include "pch.h"
#include "ListenSocket.h"
#include "ServerDlg.h"

CListenSocket::CListenSocket(CServerDlg* pDlg)
    : m_pServerDlg(pDlg)
{
}

CListenSocket::~CListenSocket()
{
}

void CListenSocket::OnAccept(int nErrorCode)
{
    CAsyncSocket::OnAccept(nErrorCode);

        // TODO: ServerDlg에 pNewSocket 객체를 관리 리스트에 추가하는 로직 필요
        // m_pServerDlg->AddServiceSocket(pNewSocket);
    if(nErrorCode == 0 && m_pServerDlg)
    {
        //  연결 수락 처리를 대화 상자에게 위임합니다.
        // CServerDlg에 ProcessAccept 함수를 구현해야 합니다.

        m_pServerDlg->ProcessAccept(this);

    }
 
}