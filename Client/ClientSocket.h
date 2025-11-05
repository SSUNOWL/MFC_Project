// ClientSocket.h

#pragma once
#include <afxsock.h> // CAsyncSocket을 위해 필수

class CClientDlg; // CClientDlg 전방 선언 (순환 참조 방지)

class CClientSocket : public CAsyncSocket
{
public:
    // 생성자: 대화 상자 포인터를 받아 저장합니다.
    CClientSocket(CClientDlg* pDlg = nullptr);
    virtual ~CClientSocket();
    BOOL IsConnected() const { return m_bConnected; };

protected:
    // CAsyncSocket의 핵심 콜백 함수 재정의
    virtual void OnConnect(int nErrorCode); // 연결 완료 시
    virtual void OnReceive(int nErrorCode); // 데이터 수신 시
    virtual void OnClose(int nErrorCode);   // 연결 종료 시
    

private:
    CClientDlg* m_pClientDlg; // 대화 상자 포인터
    BOOL m_bConnected;
};