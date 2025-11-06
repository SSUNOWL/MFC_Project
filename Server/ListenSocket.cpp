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
    if (nErrorCode == 0 && m_pServerDlg)
    {
        //  연결 수락 처리를 대화 상자에게 위임합니다.
        // CServerDlg에 ProcessAccept 함수를 구현해야 합니다.
        m_pServerDlg->ProcessAccept(this);
    }

    CAsyncSocket::OnAccept(nErrorCode);
}

