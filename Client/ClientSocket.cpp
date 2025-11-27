// ClientSocket.cpp : 구현 파일

#include "pch.h"
#include "Client.h" // 프로젝트 이름 헤더
#include "ClientSocket.h"
#include "ClientDlg.h" // ★ ClientDlg 멤버 접근용
#include "afxsock.h"

// 생성자
CClientSocket::CClientSocket(CClientDlg* pDlg)
    : m_pClientDlg(pDlg)
    , m_bConnected(FALSE)
    , m_nextMessageSize(0)
{
}

CClientSocket::~CClientSocket()
{
}

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

void CClientSocket::OnClose(int nErrorCode)
{
    m_bConnected = FALSE;
    if (m_pClientDlg)
    {
        CString strLog;
        strLog.Format(_T("INFO: 서버와의 연결이 끊어졌습니다. (에러코드: %d)"), nErrorCode);
        m_pClientDlg->m_static_status.SetWindowText(strLog);
    }
    CAsyncSocket::OnClose(nErrorCode);
}

// 맵 파싱 헬퍼 함수
typedef CMap<CString, LPCTSTR, CString, LPCTSTR> CStringToStringMap;
void ParseMessageToMap(const CString& strMessage, CStringToStringMap& mapResult)
{
    CString strToken;
    int nPos = 0;
    mapResult.RemoveAll();
    strToken = strMessage.Tokenize(_T("|"), nPos);
    while (!strToken.IsEmpty())
    {
        int nColonPos = strToken.Find(_T(':'));
        if (nColonPos != -1)
        {
            CString strKey = strToken.Left(nColonPos);
            CString strValue = strToken.Mid(nColonPos + 1);
            mapResult[strKey] = strValue;
        }
        strToken = strMessage.Tokenize(_T("|"), nPos);
    }
}

// [수정됨] 안전한 OnReceive
void CClientSocket::OnReceive(int nErrorCode)
{
    char tempBuffer[1024];
    int nRecv = Receive(tempBuffer, sizeof(tempBuffer));

    if (nRecv > 0)
    {
        m_recvBuffer.insert(m_recvBuffer.end(), tempBuffer, tempBuffer + nRecv);

        while (true)
        {
            if (m_recvBuffer.size() < sizeof(int)) break;

            if (m_nextMessageSize == 0)
            {
                int nLength = 0;
                std::memcpy(&nLength, m_recvBuffer.data(), sizeof(int));
                m_nextMessageSize = static_cast<size_t>(nLength);

                if (m_nextMessageSize <= 0 || m_nextMessageSize > MAX_MESSAGE_SIZE)
                {
                    m_nextMessageSize = 0;
                    OnClose(nErrorCode);
                    return;
                }
            }

            size_t payload_size = m_recvBuffer.size() - sizeof(int);

            if (payload_size >= m_nextMessageSize)
            {
                const char* message_start = m_recvBuffer.data() + sizeof(int);
                std::string utf8_data(message_start, m_nextMessageSize);

                // 메시지 처리 호출
                ProcessExtractedMessage(utf8_data);

                // [중요] 버퍼 정리 (vector iterator 오류 방지)
                size_t total_processed_size = sizeof(int) + m_nextMessageSize;

                if (total_processed_size <= m_recvBuffer.size()) {
                    m_recvBuffer.erase(m_recvBuffer.begin(), m_recvBuffer.begin() + total_processed_size);
                }
                else {
                    m_recvBuffer.clear(); // 비정상 상황 시 초기화
                }
                m_nextMessageSize = 0;
            }
            else
            {
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

// [수정됨] 소켓에서 로직을 처리하는 함수 (협업 기준)
void CClientSocket::ProcessExtractedMessage(const std::string& utf8_data)
{
    // 1. 변환 (CStringToUTF8가 없어도 동작하도록 직접 변환)
    int nLen = MultiByteToWideChar(CP_UTF8, 0, utf8_data.c_str(), (int)utf8_data.size(), NULL, 0);
    CString strMessage;
    if (nLen > 0) {
        WCHAR* pBuf = strMessage.GetBuffer(nLen);
        MultiByteToWideChar(CP_UTF8, 0, utf8_data.c_str(), (int)utf8_data.size(), pBuf, nLen);
        strMessage.ReleaseBuffer(nLen);
    }

    // 2. 파싱
    CStringToStringMap messageMap;
    ParseMessageToMap(strMessage, messageMap);

    CString strType, strSender;
    messageMap.Lookup(_T("type"), strType);
    messageMap.Lookup(_T("sender"), strSender);

    if (m_pClientDlg)
    {
        // --- [1] 채팅 ---
        if (strType == _T("CHAT")) {
            CString strSend;
            if (messageMap.Lookup(_T("content"), strSend))
                m_pClientDlg->DisplayMessage(strSender, strSend, TRUE);
        }

        // --- [2] 공용판 업데이트 (추가됨) ---
        else if (strType == _T("UpdateBoard"))
        {
            CString strBoardData;
            if (messageMap.Lookup(_T("board"), strBoardData))
            {
                // UI 동기화 함수 호출
                m_pClientDlg->DeserializePublicBoard(strBoardData);
            }
        }

        // --- [3] 턴 시작 알림 ---
        else if (strType == _T("StartTurn")) {
            m_pClientDlg->m_bCurrentTurn = true;
            AfxMessageBox(_T("당신의 턴입니다!")); // 여기서 한 번만 뜹니다.
        }

        // --- [4] 이름 설정 ---
        else if (strType == _T("GetName")) {
            CString strMsg;
            CString Name = m_pClientDlg->m_strName;
            strMsg.Format(_T("type:GetName|sender:%s|name:%s"), Name, Name);
            m_pClientDlg->RequestMessage(strMsg);
        }

        // --- [5] 초기 타일 ---
        else if (strType == _T("StartTile")) {
            CString strPos, strTileid;
            int nX = 0, nY = 0, nTileid = 0;
            if (messageMap.Lookup(_T("pos"), strPos)) {
                int comma_pos = strPos.Find(_T(","));
                CString sub = strPos.Mid(0, comma_pos);
                strPos = strPos.Mid(comma_pos + 1);
                nX = _ttoi(sub);
                nY = _ttoi(strPos);
            }
            if (messageMap.Lookup(_T("tileid"), strTileid)) {
                nTileid = _ttoi(strTileid);
            }
            m_pClientDlg->m_private_tile[nX][nY] = m_pClientDlg->ParseIdtoTile(nTileid);

            // 로그
            CString strMsg;
            strMsg.Format(_T("%d %d"), nTileid, m_pClientDlg->m_private_tile[nX][nY].tileId);
            m_pClientDlg->DisplayMessage(0, strMsg, 1);

            m_pClientDlg->Invalidate(FALSE);
        }

        // --- [6] 입장 알림 ---
        else if (strType == _T("Accept")) {
            CString name, strNum;
            int nNum = 0;
            messageMap.Lookup(_T("name"), name);
            if (messageMap.Lookup(_T("num"), strNum)) {
                nNum = _ttoi(strNum) + 1;
            }
            CString strMsg;
            strMsg.Format(_T("%s님이 입장하였습니다. 현재 %d명"), name, nNum);
            m_pClientDlg->DisplayMessage(_T("시스템"), strMsg, true);
        }

        // --- [7] 타일 받기 ---
        else if (strType == _T("ReceiveTile")) {
            CString strPos, strTileid;
            int nTileid = 0;
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
                if (received == true) break;
            }
            m_pClientDlg->Invalidate(FALSE);
        }
    }
}