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
    char tempBuffer[1024];
    // 현재 소켓 버퍼에서 읽을 수 있는 만큼 최대 1024 바이트를 읽습니다.
    int nRecv = Receive(tempBuffer, sizeof(tempBuffer));

    if (nRecv > 0)
    {
        // 1. 받은 데이터를 누적 버퍼(m_recvBuffer)에 추가합니다.
        m_recvBuffer.insert(m_recvBuffer.end(), tempBuffer, tempBuffer + nRecv);

        // 2. 누적 버퍼에서 완전한 메시지를 추출하는 루프를 시작합니다.
        while (true)
        {
            // A. 아직 길이 헤더(4바이트)를 읽을 수 없는 상태라면 루프 종료
            if (m_recvBuffer.size() < sizeof(int))
            {
                break;
            }

            // B. 메시지 길이(m_nextMessageSize)가 아직 결정되지 않았다면 헤더를 읽습니다.
            if (m_nextMessageSize == 0)
            {
                // 버퍼의 앞 4바이트를 int(메시지 길이)로 해석합니다.
                // reinterpret_cast<int*>를 사용할 때는 정렬(alignment) 문제가 발생할 수 있으므로, 
                // 안전하게 memcpy를 사용하거나, char 배열을 int로 복사하는 것이 좋습니다.
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
                    return; // 함수 종료
                }
            }

            // C. 메시지 본문(Payload) 데이터가 충분히 모였는지 확인합니다.
            // (누적된 전체 크기 - 헤더 크기) >= 메시지 본문 길이
            size_t payload_size = m_recvBuffer.size() - sizeof(int);

            if (payload_size >= m_nextMessageSize)
            {
                // **완전한 하나의 메시지가 도착했습니다!**

                // 헤더(4바이트)를 건너뛰고 메시지 본문을 추출합니다.
                const char* message_start = m_recvBuffer.data() + sizeof(int);

                // 추출한 메시지 본문을 string으로 변환 후 처리 함수 호출
                std::string utf8_data(message_start, m_nextMessageSize);
                ProcessExtractedMessage(utf8_data);

                // 처리한 헤더와 메시지 본문만큼 누적 버퍼에서 제거합니다. (핵심!)
                size_t total_processed_size = sizeof(int) + m_nextMessageSize;
                m_recvBuffer.erase(m_recvBuffer.begin(),
                    m_recvBuffer.begin() + total_processed_size);

                // 다음 메시지 처리를 위해 상태 초기화
                m_nextMessageSize = 0;
            }
            else
            {
                // 아직 메시지 본문이 완전히 도착하지 않았습니다. 다음 OnReceive를 기다립니다.
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
    // 1. 기존 OnReceive에서 메시지를 처리하던 로직을 이곳으로 가져옵니다.
    // 참고: std::string to CString 변환 및 Map 파싱 함수가 필요합니다.

    // CString strMessage = UTF8ToCString(utf8_data); // 가정: 이 함수는 정의되어 있음

    // **주의: UTF-8 to CString 변환 로직은 직접 정의하셔야 합니다.
    // 여기서는 예시로 기존 로직을 따릅니다.**

    //---------------------------------------------------------
    // 아래 코드는 기존 OnReceive의 핵심 로직입니다.
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

        // 2. 대화 상자에 브로드캐스트 요청 (메시지 내용과 이 소켓 객체를 보냄)
        // ... (CHAT, PLACE, GetName, EndTurn, Receive 등 모든 타입별 분기 로직) ...

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

            POSITION m_posTurn = m_pServerDlg->m_clientSocketList.GetHeadPosition();

            CServiceSocket* pTurn = m_pServerDlg->m_clientSocketList.GetNext(m_posTurn);

            CString strMsg; int tileid;

            tileid = m_pServerDlg->m_rand_tile_list[m_pServerDlg->m_deck_pos++].tileId;

            strMsg.Format(_T("type:ReceiveTile|tileid:%d"), tileid);

            m_pServerDlg->ResponseMessage(strMsg, pTurn);

            m_pServerDlg->NextTurn();

        }
    }
}