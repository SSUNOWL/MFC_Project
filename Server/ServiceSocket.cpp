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
        if (messageMap.Lookup(_T("type"), strType)){}
        if (messageMap.Lookup(_T("sender"), strSender)){}
        if (strType == _T("CHAT")) {
            CString strSend;
            if (messageMap.Lookup(_T("content"), strSend));
            m_pServerDlg->DisplayMessage(strSender, strSend, TRUE);
            m_pServerDlg->BroadcastMessage(strMessage, this);
        }
        else if (strType == _T("PLACE")) {
            // [251127] ServerDlg에 구현한 함수 호출
            // 1. 서버의 공용판 배열 업데이트
            // 2. 다른 클라이언트들에게 브로드캐스트
            m_pServerDlg->ProcessPublicBoardUpdate(strMessage);
        }
        else if (strType == _T("GetName")) {
            messageMap.Lookup(_T("name"), m_strName);


            if (m_pServerDlg)
            {
                // [수정] this 포인터를 함께 전달하여 리스트에 등록
                m_pServerDlg->AddPlayerToList(m_strName, 0, this);
            }

            CString strLog;
            strLog.Format(_T("INFO: 클라이언트 %s 연결 수락됨 (현재 %d명)"), m_strName, m_pServerDlg->m_clientSocketList.GetCount());
            m_pServerDlg->AddLog(strLog);
            CString strMsg;
            
            CString strChat;
            strChat.Format(_T("%s님이 입장하였습니다. 현재 %d명"), this->m_strName, m_pServerDlg->m_clientSocketList.GetCount() + 1);
            m_pServerDlg->DisplayMessage(_T("시스템"), strChat, 1);
            // 다른 플레이어들에게 전달

            if (m_pServerDlg)
            {
                // 서버의 리스트 컨트롤(UI)이 가장 정확한 명단이므로 이를 순회
                int nCount = m_pServerDlg->m_listPlayer.GetItemCount();

                for (int i = 0; i < nCount; i++)
                {
                    CString strExistingName = m_pServerDlg->m_listPlayer.GetItemText(i, 0);
                    CString strExistingNum = m_pServerDlg->m_listPlayer.GetItemText(i, 1);

                    // 방금 들어온 본인(m_strName)은 제외하고 보내거나, 
                    // 클라이언트 로직이 중복을 허용하지 않는다면 다 보내도 됩니다.
                    // 보통은 다 보내서 클라이언트가 리스트를 초기화하고 다시 채우게 하거나
                    // 없는 사람만 추가하게 합니다. 여기서는 'AddPlayer' 메시지를 보냅니다.
                    DWORD_PTR pStoredSocket = m_pServerDlg->m_listPlayer.GetItemData(i);

                    // 메시지에 |id:주소값 추가
                    CString strSyncMsg;
                    strSyncMsg.Format(_T("type:AddPlayer|name:%s|tilenum:%s|id:%llu"),
                        strExistingName, strExistingNum, (unsigned long long)pStoredSocket);

                    m_pServerDlg->ResponseMessage(strSyncMsg, this);
                }
            }
            CString strNewPlayerMsg;
        strNewPlayerMsg.Format(_T("type:AddPlayer|name:%s|tilenum:0|id:%llu"), 
            m_strName, (unsigned long long)this);
        
        // 나를 제외한 모두에게 전송
        m_pServerDlg->BroadcastMessage(strNewPlayerMsg, this);


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
        else if (strType == _T("EndGame")) {
            CString tmpString;
            int isNormalEnd = -1;
            if (messageMap.Lookup(_T("isNormalEnd"), tmpString)) {
                isNormalEnd = _ttoi(tmpString);
            }

            if (isNormalEnd == 0) { // 비정상적으로 종료된 경우
                // 게임 종료
                m_pServerDlg->m_bisGameStarted = false;

				// 송신자를 제외한 모든 클라이언트에게 게임 종료 메시지 전송
                CServiceSocket* pSender = NULL;
                POSITION pos = m_pServerDlg->m_clientSocketList.GetHeadPosition();
                while (pos != NULL) {
                    CServiceSocket* pSocket = m_pServerDlg->m_clientSocketList.GetNext(pos);
                    if (pSocket && (pSocket->m_strName == strSender)) {
                        pSender = pSocket;
                    }
                }

                CString requestMsg;
                requestMsg.Format(_T("type:EndGame|isNormalEnd:0"));
                m_pServerDlg->BroadcastMessage(requestMsg, pSender);
                
                AfxMessageBox(_T("플레이어 탈주로 게임을 종료합니다.", MB_OK));

                // 다이얼로그 닫기
                m_pServerDlg->PostMessage(WM_CLOSE);
                //if (m_pServerDlg->GetSafeHwnd())  // NULL이 아니면 윈도우가 아직 존재
                //{
                //    m_pServerDlg->PostMessage(WM_CLOSE);
                //}
            }
        }
        else if (strType == _T("Backup")) {
            m_pServerDlg->Backup();
        }
     
        else if (strType == _T("SetbackReq")) {
            CString strMsg;
            strMsg.Format(_T("type:Setback|sender:시스템"));
            m_pServerDlg->BroadcastMessage(strMsg, 0);
            m_pServerDlg->Setback();
        }
    }
}