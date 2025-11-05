// ServiceSocket.h

#pragma once
#include <afxsock.h>

class CServerDlg; // 전방 선언

class CServiceSocket : public CAsyncSocket
{
public:
    CServiceSocket(CServerDlg* pDlg = nullptr);
    virtual ~CServiceSocket();

    //  연결 상태를 외부에 노출하는 함수 (클라이언트와 동일)
    BOOL IsConnected() const { return m_bConnected; }

protected:
    //  클라이언트로부터 데이터가 들어오면 호출됩니다.
    virtual void OnReceive(int nErrorCode);

    //  클라이언트 연결이 끊어지면 호출됩니다.
    virtual void OnClose(int nErrorCode);

private:
    CServerDlg* m_pServerDlg;
    BOOL m_bConnected; // 연결 상태 저장
};