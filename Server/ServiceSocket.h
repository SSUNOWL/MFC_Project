// ServiceSocket.h

#pragma once
#include <afxsock.h>
#include <vector> // std::vector를 사용하기 위해 추가
#include <string> // std::string을 사용하기 위해 추가

// 최대 메시지 길이를 정의합니다. (예: 1MB) - 필요에 따라 조정하세요.
#define MAX_MESSAGE_SIZE (1024 * 1024)
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
    std::vector<char> m_recvBuffer;
    size_t m_nextMessageSize = 0;
    void ProcessExtractedMessage(const std::string& utf8_data);
public:
//    CString Name;
    CString m_strName;
};