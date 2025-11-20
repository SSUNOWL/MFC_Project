// ClientSocket.cpp

#include "pch.h"
#include "ClientSocket.h"
#include "ClientDlg.h" // CClientDlg의 실제 정의를 위해 포함
#include "afxsock.h"
// 생성자 구현
CClientSocket::CClientSocket(CClientDlg* pDlg)
    : m_pClientDlg(pDlg) // 대화 상자 포인터 초기화
    , m_bConnected(FALSE)
{
}

CClientSocket::~CClientSocket()
{
}

// 다음 단계에서 구현 예정
void CClientSocket::OnConnect(int nErrorCode)
{
    m_bConnected = TRUE;
    if (m_pClientDlg)
    {
        if (nErrorCode == 0)
            m_pClientDlg->m_static_status.SetWindowText(_T("서버 연결 성공!"));
        else {
            CString strStatus;
            strStatus.Format(_T("연결 실패! (에러코드: %d)"), nErrorCode);
            m_pClientDlg->m_static_status.SetWindowText(strStatus);
        }
    }
    CAsyncSocket::OnConnect(nErrorCode);
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

void CClientSocket::OnReceive(int nErrorCode)
{
    char tempBuffer[1024];
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
                int nLength = 0;
                // 버퍼의 앞 4바이트를 int로 해석합니다.
                std::memcpy(&nLength, m_recvBuffer.data(), sizeof(int));
                m_nextMessageSize = static_cast<size_t>(nLength);

                if (m_nextMessageSize <= 0 || m_nextMessageSize > MAX_MESSAGE_SIZE)
                {
                    // 비정상적인 길이: 연결 종료 처리 후 루프 종료
                    m_nextMessageSize = 0;
                    OnClose(nErrorCode);
                    return;
                }
            }

            // C. 메시지 본문(Payload) 데이터가 충분히 모였는지 확인합니다.
            size_t payload_size = m_recvBuffer.size() - sizeof(int);

            if (payload_size >= m_nextMessageSize)
            {
                // **완전한 하나의 메시지가 도착했습니다!**
                const char* message_start = m_recvBuffer.data() + sizeof(int);

                std::string utf8_data(message_start, m_nextMessageSize);
                ProcessExtractedMessage(utf8_data); // 추출된 메시지 처리 함수 호출

                // 처리한 헤더와 메시지 본문만큼 누적 버퍼에서 제거합니다.
                size_t total_processed_size = sizeof(int) + m_nextMessageSize;
                m_recvBuffer.erase(m_recvBuffer.begin(),
                    m_recvBuffer.begin() + total_processed_size);

                // 다음 메시지 처리를 위해 상태 초기화
                m_nextMessageSize = 0;
            }
            else
            {
                // 아직 메시지 본문이 완전히 도착하지 않았습니다. 다음 Receive를 기다립니다.
                break;
            }
        }
    }
    else if (nRecv == 0 || nErrorCode != 0)
    {
        OnClose(nErrorCode);
        return;
    }

    CAsyncSocket::OnReceive(nErrorCode);
}

void CClientSocket::OnClose(int nErrorCode)
{
    m_bConnected = FALSE; //  연결 종료 시 상태 업데이트
    // ... (UI 메시지 출력 로직) ...

    if (m_pClientDlg)
    {
        //  UI 상태 업데이트
        CString strLog;
        strLog.Format(_T("INFO: 서버와의 연결이 끊어졌습니다. (에러코드: %d)"), nErrorCode);
        m_pClientDlg->m_static_status.SetWindowText(strLog);

        // CClientDlg의 소켓 포인터를 정리할 수 있도록 요청
        // CClientDlg::OnCloseSocket(this) 같은 함수를 호출하여 처리합니다.
        // 현재는 CClientDlg::OnBnClickedButtonConnect에서 정리하도록 가정합니다.
    }


    CAsyncSocket::OnClose(nErrorCode);
}


void CClientSocket::ProcessExtractedMessage(const std::string& utf8_data)
{
    // CString strMessage = UTF8ToCString(utf8_data); // 가정: 이 함수는 정의되어 있음

    //---------------------------------------------------------
    // 기존 OnReceive의 핵심 로직을 이곳으로 옮깁니다.
    CString strMessage = UTF8ToCString(utf8_data);
    CStringToStringMap messageMap;
    ParseMessageToMap(strMessage, messageMap);

    CString strType, strSender;
    if (messageMap.Lookup(_T("type"), strType));
    if (messageMap.Lookup(_T("sender"), strSender));

    if (m_pClientDlg)
    {
        if (strType == _T("CHAT")) {
            CString strSend;
            if (messageMap.Lookup(_T("content"), strSend));
            m_pClientDlg->DisplayMessage(strSender, strSend, TRUE);
        }
        else if (strType == _T("PLACE")) {
        }
        else if (strType == _T("GetName")) {
            CString strMsg;
            CString Name = m_pClientDlg->m_strName;
            strMsg.Format(_T("type:GetName|sender:%s|name:%s"), Name, Name);
            m_pClientDlg->RequestMessage(strMsg);
        }
        else if (strType == _T("StartTile")) {
            CString strPos, strTileid;
            int nX, nY, nTileid;
            if (messageMap.Lookup(_T("pos"), strPos)) {
                int comma_pos = strPos.Find(_T(","));
                CString sub;
                sub = strPos.Mid(0, comma_pos);
                strPos = strPos.Mid(comma_pos + 1);
                nX = _ttoi(sub);
                nY = _ttoi(strPos);
            }
            if (messageMap.Lookup(_T("tileid"), strTileid)) {
                nTileid = _ttoi(strTileid);
            }

            m_pClientDlg->m_private_tile[nX][nY] = m_pClientDlg->ParseIdtoTile(nTileid);
            // 로그 출력용
            /*CString strMsg;
            strMsg.Format(_T("%d %d %d"), m_pClientDlg->m_private_tile[nX][nY].num, m_pClientDlg->m_private_tile[nX][nY], m_pClientDlg->m_private_tile[nX][nY].tileId);
            m_pClientDlg->DisplayMessage(0, strMsg, 1);*/
            //개인 타일판을 시각화하는 함수
            m_pClientDlg->Invalidate(FALSE);
        }
        else if (strType == _T("StartTurn")) {
            m_pClientDlg->m_bCurrentTurn = true;
        }
        else if (strType == _T("Accept")) {
            CString name, strNum;
            int nNum;
            if (messageMap.Lookup(_T("name"), name));
            if (messageMap.Lookup(_T("num"), strNum)) {
                nNum = _ttoi(strNum) + 1;
            }
            CString strMsg;
            strMsg.Format(_T("%s님이 입장하였습니다. 현재 %d명"), name, nNum);
            m_pClientDlg->DisplayMessage(_T("시스템"), strMsg, true);
        }
        else if (strType == _T("ReceiveTile")) {
            CString strPos, strTileid;
            int nTileid;

            if (messageMap.Lookup(_T("tileid"), strTileid)) {
                nTileid = _ttoi(strTileid);
            }
            bool received = false;
            for (int i = 1; i <= 3; i++) {
                for (int j = 1; j <= 17; j++) {
                    if (m_pClientDlg->m_private_tile[i][j].tileId == -1) {
                        m_pClientDlg->m_private_tile[i][j] = m_pClientDlg->ParseIdtoTile(nTileid);
                        received = true;
                        break;
                    }
                }
                if (received == true)
                    break;
            }
            m_pClientDlg->Invalidate(FALSE);
        }
    }
}