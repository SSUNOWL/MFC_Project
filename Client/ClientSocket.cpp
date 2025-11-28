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

void CClientSocket::OnReceive(int nErrorCode)
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
                ProcessExtractedMessage(utf8_data); 
                size_t total_processed_size = sizeof(int) + m_nextMessageSize;
                m_recvBuffer.erase(m_recvBuffer.begin(),
                m_recvBuffer.begin() + total_processed_size);

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
    }
    CAsyncSocket::OnClose(nErrorCode);
}


void CClientSocket::ProcessExtractedMessage(const std::string& utf8_data)
{

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
            // [251127] ClientDlg에 구현한 함수 호출
            m_pClientDlg->ProcessPublicBoardUpdate(strMessage);
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
            CString strMsg;
            strMsg.Format(_T("%d %d"), nTileid,  m_pClientDlg->m_private_tile[nX][nY].tileId);
            m_pClientDlg->DisplayMessage(0, strMsg, 1);
            //개인 타일판을 시각화하는 함수
            m_pClientDlg->Invalidate(TRUE);
        }
        else if (strType == _T("StartTurn")) {
            // [251127] 내 턴이 시작될 때 현재 상태를 'Old'에 백업해둬야 기준점이 생김
            m_pClientDlg->CopyBoards();
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
            m_pClientDlg->AddPlayerToList(name, 0, 0);
        }
        else if (strType == _T("AddPlayer")) {
            CString strName, strTileNum, strID;
            int nTileNum = 0;
            DWORD_PTR nID = 0;

            if (messageMap.Lookup(_T("name"), strName));
            if (messageMap.Lookup(_T("tilenum"), strTileNum)) {
                nTileNum = _ttoi(strTileNum);
            }
            if (messageMap.Lookup(_T("id"), strID)) {
                nID = (DWORD_PTR)_ttoi64(strID);
            }

            // ID를 포함하여 리스트에 추가
            m_pClientDlg->AddPlayerToList(strName, nTileNum, nID);
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
            if (m_pClientDlg->m_bCurrentTurn)  
                m_pClientDlg->m_bCurrentTurn = false;
            m_pClientDlg->Invalidate(TRUE);
            m_pClientDlg->UpdateSelfTileNum();
        }
        else if (strType == _T("Backup")) {
            if (m_pClientDlg->m_bCurrentTurn) { // 개인판은 내 턴일때만 복사해두면 됨
                for (int i = 1; i <= 3; i++)
                    for (int j = 1; j <= 17; j++)
                        m_pClientDlg->m_old_private_tile[i][j] = m_pClientDlg->m_private_tile[i][j];
            }
            for (int i = 1; i <= 13; i++) // 공용판은 누구의 턴이던 모두에게 적용되므로 복사해야함
                for (int j = 1; j <= 27; j++)
                    m_pClientDlg->m_old_public_tile[i][j] = m_pClientDlg->m_public_tile[i][j];
        }
        else if (strType == _T("Setback")) {
            if (m_pClientDlg->m_bCurrentTurn) { // 자기 차례면 개인판도 Setback
                for (int i = 1; i <= 3; i++)
                    for (int j = 1; j <= 17; j++)
                        m_pClientDlg->m_private_tile[i][j] = m_pClientDlg->m_old_private_tile[i][j];
            }
            for (int i = 1; i <= 13; i++) // 공용판은 항상 Setback
                for (int j = 1; j <= 27; j++)
                    m_pClientDlg->m_public_tile[i][j] = m_pClientDlg->m_old_public_tile[i][j];
            m_pClientDlg->Invalidate(TRUE);
        }
        else if (strType == _T("EndGame")) {
            CString tmpString;
            int isNormalEnd = -1;
            if (messageMap.Lookup(_T("isNormalEnd"), tmpString)) {
				isNormalEnd = _ttoi(tmpString);
            }

            if (isNormalEnd == 0) { // 비정상적으로 종료된 경우
                // 게임 종료
                m_pClientDlg->m_bisGameStarted = FALSE;

                AfxMessageBox(_T("플레이어 탈주로 게임을 종료합니다.", MB_OK));
                // 다이얼로그 닫기
                if (m_pClientDlg->GetSafeHwnd())  // NULL이 아니면 윈도우가 아직 존재
                {
                    m_pClientDlg->PostMessage(WM_CLOSE);
                }
            }
        }
        else if (strType == _T("GameStarted")) {
			m_pClientDlg->m_bisGameStarted = TRUE;

            AfxMessageBox(_T("게임이 시작되었습니다.", MB_OK));
        }

        else if (strType == _T("UpdateTileNum")) {
            int nTilenum;
            CString strTilenum;
            CString strID;
            if (messageMap.Lookup(_T("tilenum"), strTilenum));
            DWORD_PTR nID = 0;

            nTilenum = _ttoi(strTilenum);
            if (messageMap.Lookup(_T("id"), strID)) {
                nID = (DWORD_PTR)_ttoi64(strID);
            }
            m_pClientDlg->UpdatePlayerTileCount(nID, nTilenum);


         }
        else if (strType == _T("UpdatePlayer")) {

            CString strID;
            DWORD_PTR nID = 0;
            CString strName;
            if (messageMap.Lookup(_T("name"), strName));
            if (messageMap.Lookup(_T("id"), strID)) {
                nID = (DWORD_PTR)_ttoi64(strID);
            }
            m_pClientDlg->m_pTurn = nID;

            CString strContent;
            strContent.Format(_T("%s의 턴이 시작되었습니다 %lld"), strName, nID);

            m_pClientDlg->DisplayMessage(strName, strContent, TRUE);

            m_pClientDlg->m_listPlayer.RedrawItems(0, m_pClientDlg->m_listPlayer.GetItemCount() - 1);
            m_pClientDlg->m_listPlayer.UpdateWindow();
        }
        else if (strType == _T("Reject")) {
            m_pClientDlg->MessageBox(_T("방이 가득 찼습니다.", MB_OK));
            Close();

            // 3. UI 상태 업데이트 (연결 끊김 처리)
            m_pClientDlg->m_static_status.SetWindowText(_T("서버 접속이 거부되었습니다."));
        }
    }
}