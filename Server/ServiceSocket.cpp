// ServiceSocket.cpp

#include "pch.h"
#include "ServiceSocket.h"
#include "ServerDlg.h"

CServiceSocket::CServiceSocket(CServerDlg* pDlg)
    : m_pServerDlg(pDlg)
    , m_bConnected(TRUE) // 연결이 수락되었으므로 초기 상태는 TRUE
{
    m_strName = _T("");
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
            CString strType, strSender;
            if (messageMap.Lookup(_T("type"), strType));
            if (messageMap.Lookup(_T("sender"), strSender));
            //  2. 대화 상자에 브로드캐스트 요청 (메시지 내용과 이 소켓 객체를 보냄)
            if (strType == _T("CHAT")) {
                CString strSend;
                if (messageMap.Lookup(_T("content"), strSend));

                m_pServerDlg->DisplayMessage(strSender, strSend, TRUE);
                CString strMsg;

                m_pServerDlg->BroadcastMessage(strMessage, this);
                
            }
            else if (strType == _T("PLACE")) {

            }
            else if (strType == _T("GetName")) {

                messageMap.Lookup(_T("name"), m_strName );

                CString strLog;
                strLog.Format(_T("INFO: 클라이언트 %s 연결 수락됨 (현재 %d명)"), m_strName, m_pServerDlg->m_clientSocketList.GetCount());
                m_pServerDlg->AddLog(strLog);
                CString strMsg;
                strMsg.Format(_T("type:Accept|sender:서버|name:%s|num:%d"), this->m_strName, m_pServerDlg->m_clientSocketList.GetCount());
                m_pServerDlg->BroadcastMessage(strMsg, 0); 
                CString strChat;
                strChat.Format(_T("%s님이 입장하였습니다. 현재 %d명"), this->m_strName, m_pServerDlg->m_clientSocketList.GetCount() + 1);
                m_pServerDlg->DisplayMessage(_T("시스템"), strChat, 1);
                // 다른 플레이어들에게 전달

            }
            else if (strType == _T("EndTurn")) {
                // pass, receive시 턴을 끝내주세요~~ 라는 뜻
                
                m_pServerDlg->NextTurn();

            }
            else if (strType == _T("Receive")) {
                POSITION m_posTurn = m_pServerDlg->m_clientSocketList.GetHeadPosition();
                CServiceSocket* pTurn = m_pServerDlg->m_clientSocketList.GetNext(m_posTurn);
                CString strMsg; int tileid;
                tileid = m_pServerDlg->m_rand_tile_list[m_pServerDlg->m_deck_pos++].tileId;
                strMsg.Format(_T("type:ReceiveTile|tileid:%d"), tileid);
                m_pServerDlg->ResponseMessage(strMsg,pTurn);
                m_pServerDlg->NextTurn();
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