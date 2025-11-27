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

        m_pServerDlg->RemoveClient(this);
    }

    
    CAsyncSocket::OnClose(nErrorCode);
}
typedef CMap<CString, LPCTSTR, CString, LPCTSTR> CStringToStringMap;
void ParseMessageToMap(const CString& strMessage, CStringToStringMap& mapResult)
{
    
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
    
}
void CServiceSocket::OnReceive(int nErrorCode)
{
    char tempBuffer[1024];

    int nRecv = Receive(tempBuffer, sizeof(tempBuffer));

    if (nRecv > 0)
    {
        
        m_recvBuffer.insert(m_recvBuffer.end(), tempBuffer, tempBuffer + nRecv);
        while (true)
        {

            if (m_recvBuffer.size() < sizeof(int))
            {
                break;
            }
            if (m_nextMessageSize == 0)
            {
            
                int nLength = 0;
                std::memcpy(&nLength, m_recvBuffer.data(), sizeof(int));
                m_nextMessageSize = static_cast<size_t>(nLength);

                // 길이 값의 유효성 검증
                if (m_nextMessageSize <= 0 || m_nextMessageSize > MAX_MESSAGE_SIZE)
                {
                    // 비정상적인 길이: 연결 종료 처리 후 루프 종료
                    m_nextMessageSize = 0;
                    // m_pServerDlg->AddLog(_T("ERROR: Invalid message length. Closing connection."));
                    OnClose(nErrorCode);
                    return; 
                }
            }

            
            size_t payload_size = m_recvBuffer.size() - sizeof(int);

            if (payload_size >= m_nextMessageSize)
            {

                const char* message_start = m_recvBuffer.data() + sizeof(int);

                std::string utf8_data(message_start, m_nextMessageSize);
                ProcessExtractedMessage(utf8_data);

                size_t total_processed_size = sizeof(int) + m_nextMessageSize;
                m_recvBuffer.erase(m_recvBuffer.begin(),
                    m_recvBuffer.begin() + total_processed_size);

                m_nextMessageSize = 0;
            }
            else
            {
                // 다음 OnReceive를 기다립니다.
                break;
            }
        }
    }
    else if (nRecv == 0 || nErrorCode != 0)
    {
        // 연결 종료 처리
        OnClose(nErrorCode);
        return;
    }

    CAsyncSocket::OnReceive(nErrorCode);
}




void CServiceSocket::ProcessExtractedMessage(const std::string& utf8_data)
{

    CString strMessage = UTF8ToCString(utf8_data);
    CStringToStringMap messageMap;
    ParseMessageToMap(strMessage, messageMap);

    // 1. 서버 로그에 메시지 수신 사실 기록
    if (m_pServerDlg)
    {
        m_pServerDlg->AddLog(_T("RECV: ") + strMessage);
        CString strType, strSender;
        if (messageMap.Lookup(_T("type"), strType));
        if (messageMap.Lookup(_T("sender"), strSender));
        if (strType == _T("CHAT")) {
            CString strSend;
            if (messageMap.Lookup(_T("content"), strSend));
            m_pServerDlg->DisplayMessage(strSender, strSend, TRUE);
            m_pServerDlg->BroadcastMessage(strMessage, this);
        }
        else if (strType == _T("PLACE")) {
        }
        else if (strType == _T("GetName")) {
            messageMap.Lookup(_T("name"), m_strName);
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
            // pass, receive시 턴을 끝내주세요~~ 라는 
            m_pServerDlg->NextTurn();
        }

        else if (strType == _T("Receive")) {
            if (m_pServerDlg->m_deck_pos < 106) {
                CString strMsg; int tileid;
                tileid = m_pServerDlg->m_rand_tile_list[m_pServerDlg->m_deck_pos++].tileId;
                strMsg.Format(_T("type:ReceiveTile|tileid:%d"), tileid);
                m_pServerDlg->ResponseMessage(strMsg, this);
                m_pServerDlg->NextTurn();
            }
            else {
                CString strMsg;
                strMsg.Format(_T("type:CHAT|sender:시스템|content:현재 남은 타일이 없습니다."));

                m_pServerDlg->ResponseMessage(strMsg, this);

                m_pServerDlg->NextTurn();
            }
        }
    }
}