// ServiceSocket.cpp

#include "pch.h"
#include "ServiceSocket.h"
#include "ServerDlg.h"

CServiceSocket::CServiceSocket(CServerDlg* pDlg)
    : m_pServerDlg(pDlg)
    , m_bConnected(TRUE) // 연결이 수락되었으므로 초기 상태는 TRUE
{
}

CServiceSocket::~CServiceSocket()
{
}


void CServiceSocket::OnClose(int nErrorCode)
{
    m_bConnected = FALSE; //  상태 업데이트

    if (m_pServerDlg)
    {
        //  연결 해제 처리를 대화 상자에게 요청합니다.
        // CServerDlg에 RemoveClient 함수를 구현해야 합니다.
        m_pServerDlg->RemoveClient(this);
    }

    // 소켓 핸들을 닫고 객체는 나중에 CServerDlg에서 delete될 것입니다.
    CAsyncSocket::OnClose(nErrorCode);
}
typedef CMap<CString, LPCTSTR, CString, LPCTSTR> CStringToStringMap;
void ParseMessageToMap(const CString& strMessage, CStringToStringMap& mapResult)
{
    // CMap 객체 생성 코드를 제거하고, 매개변수 mapResult를 바로 사용합니다.
    CString strToken;
    int nPos = 0;

    // 기존 데이터 정리
    mapResult.RemoveAll();

    strToken = strMessage.Tokenize(_T("|"), nPos);

    while (!strToken.IsEmpty())
    {
        int nColonPos = strToken.Find(_T(':'));
        if (nColonPos != -1)
        {
            CString strKey = strToken.Left(nColonPos);
            CString strValue = strToken.Mid(nColonPos + 1);

            //  매개변수로 받은 mapResult에 삽입
            mapResult[strKey] = strValue;
        }
        strToken = strMessage.Tokenize(_T("|"), nPos);
    }
    // 함수 끝에서 return이 필요 없습니다.
}

void CServiceSocket::OnReceive(int nErrorCode)
{
    char buffer[1024];
    // 버퍼 크기보다 1 작게 받아 NULL 문자를 위한 공간 확보
    int nRecv = Receive(buffer, sizeof(buffer) - 1);

    if (nRecv > 0)
    {
        buffer[nRecv] = '\0';
        std::string utf8_data(buffer, nRecv);

        CString strMessage = UTF8ToCString(utf8_data);
        CStringToStringMap messageMap;
        ParseMessageToMap(strMessage, messageMap);
        //  1. 서버 로그에 메시지 수신 사실 기록
        if (m_pServerDlg)
        {
            m_pServerDlg->AddLog(_T("RECV: ") + strMessage);
            CString strSend, strType, strSender;
            if (messageMap.Lookup(_T("content"), strSend));
            if (messageMap.Lookup(_T("type"), strType));
            if (messageMap.Lookup(_T("sender"), strSender));
            //  2. 대화 상자에 브로드캐스트 요청 (메시지 내용과 이 소켓 객체를 보냄)
            m_pServerDlg->BroadcastMessage(strType, strSender, strMessage, this);
            if (strType == _T("CHAT")) {
                m_pServerDlg->DisplayMessage(strSender, strSend, TRUE);
            }
        }
    }
    else if (nRecv == 0 || nErrorCode != 0)
    {
        // 연결이 정상 종료되거나 오류가 발생하면 OnClose를 호출하여 정리
        OnClose(nErrorCode);
        return;
    }

    CAsyncSocket::OnReceive(nErrorCode);
}