#pragma once
#include <afxsock.h>

class CServerDlg; // 전방 선언

class CListenSocket : public CAsyncSocket
{
public:
    CListenSocket(CServerDlg* pDlg = nullptr);
    virtual ~CListenSocket();

protected:
    //  클라이언트 연결 요청이 들어오면 자동으로 호출됩니다.
    virtual void OnAccept(int nErrorCode);

private:
    CServerDlg* m_pServerDlg; // 대화 상자 포인터
};