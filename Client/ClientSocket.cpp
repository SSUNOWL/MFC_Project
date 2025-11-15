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
    char buffer[1024];
    int nRecv = Receive(buffer, sizeof(buffer) - 1); // Receive 함수 사용

    if (nRecv > 0)
    {


        std::string utf8_data(buffer, nRecv);

        CString strMessage = UTF8ToCString(utf8_data);
        CStringToStringMap messageMap;
        CString strType, strSender;
        ParseMessageToMap(strMessage, messageMap);
        if (messageMap.Lookup(_T("type"), strType));
        if (messageMap.Lookup(_T("sender"), strSender));
        // 대화 상자 함수를 호출하여 메시지를 UI에 출력
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
                strMsg.Format(_T("%d %d %d"), nX, nY, nTileid);
                m_pClientDlg->DisplayMessage(0, strMsg, 1);*/
                //개인 타일판을 시각화하는 함수
            
            }
        }
    }
    else if (nRecv == 0 || nErrorCode != 0)
    {
        // 연결이 정상적으로 종료되었거나 오류 발생
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
