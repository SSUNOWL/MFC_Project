/*
// ClientSocket.cpp : 구현 파일

#include "pch.h"
#include "Client.h"        // 프로젝트 이름에 맞는 헤더 (예: Client.h)
#include "ClientSocket.h"
#include "ClientDlg.h"     // ★ ClientDlg 멤버 접근을 위해 필수
#include "afxsock.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// [헬퍼 함수] 이 파일 내부에서만 사용하도록 static 선언 (링크 에러 방지)
static void Helper_ParseMessageToMap(const CString& strMessage, CMap<CString, LPCTSTR, CString, LPCTSTR>& mapResult)
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

// [수정됨] 안전한 데이터 수신부 (vector 오류 해결)
void CClientSocket::OnReceive(int nErrorCode)
{
	char tempBuffer[1024];
	int nRecv = Receive(tempBuffer, sizeof(tempBuffer));

	if (nRecv > 0)
	{
		// 버퍼 뒤에 데이터 추가
		m_recvBuffer.insert(m_recvBuffer.end(), tempBuffer, tempBuffer + nRecv);

		while (true)
		{
			// 1. 헤더(길이 4바이트) 확인
			if (m_recvBuffer.size() < sizeof(int))
				break;

			// 다음 메시지 크기를 아직 모른다면 헤더 읽기
			if (m_nextMessageSize == 0)
			{
				int nLength = 0;
				std::memcpy(&nLength, m_recvBuffer.data(), sizeof(int));

				// 유효성 검사 (너무 크거나 0이하인 경우)
				if (nLength <= 0 || nLength > (10 * 1024 * 1024)) // 10MB 제한
				{
					// 에러: 버퍼 초기화 후 종료
					m_recvBuffer.clear();
					m_nextMessageSize = 0;
					return;
				}
				m_nextMessageSize = static_cast<size_t>(nLength);
			}

			// 2. 본문(Payload) 확인
			size_t header_size = sizeof(int);
			size_t total_packet_size = header_size + m_nextMessageSize;

			if (m_recvBuffer.size() >= total_packet_size)
			{
				// 데이터가 모두 도착함 -> 추출
				const char* message_start = m_recvBuffer.data() + header_size;
				std::string utf8_data(message_start, m_nextMessageSize);

				// 메시지 처리
				ProcessExtractedMessage(utf8_data);

				// [중요] 처리한 데이터 삭제 (Iterator 오류 방지)
				if (total_packet_size <= m_recvBuffer.size()) {
					m_recvBuffer.erase(m_recvBuffer.begin(), m_recvBuffer.begin() + total_packet_size);
				}
				else {
					m_recvBuffer.clear(); // 이상 상황 시 초기화
				}

				m_nextMessageSize = 0; // 다음 메시지 대기
			}
			else
			{
				// 데이터가 아직 덜 왔음 -> 대기
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

// [수정됨] 공용판 업데이트 로직이 포함된 메시지 처리 함수
void CClientSocket::ProcessExtractedMessage(const std::string& utf8_data)
{
	// 1. UTF-8 -> Unicode(CString) 변환
	int nLen = MultiByteToWideChar(CP_UTF8, 0, utf8_data.c_str(), (int)utf8_data.size(), NULL, 0);
	CString strMessage;
	if (nLen > 0) {
		WCHAR* pBuf = strMessage.GetBuffer(nLen);
		MultiByteToWideChar(CP_UTF8, 0, utf8_data.c_str(), (int)utf8_data.size(), pBuf, nLen);
		strMessage.ReleaseBuffer(nLen);
	}

	// 2. 맵 파싱
	CMap<CString, LPCTSTR, CString, LPCTSTR> messageMap;
	Helper_ParseMessageToMap(strMessage, messageMap);

	CString strType, strSender;
	messageMap.Lookup(_T("type"), strType);
	messageMap.Lookup(_T("sender"), strSender);

	// 3. 다이얼로그 제어
	if (m_pClientDlg)
	{
		// [공용판 업데이트] ★★★ 여기가 핵심입니다 ★★★
		if (strType == _T("UpdateBoard"))
		{
			CString strBoardData;
			if (messageMap.Lookup(_T("board"), strBoardData))
			{
				// 충돌 방지를 위해 내 턴이 아닐 때만 갱신 (혹은 강제 갱신)
				// if (!m_pClientDlg->m_bCurrentTurn) 
				{
					m_pClientDlg->DeserializePublicBoard(strBoardData);
				}
			}
		}
		// [기존 채팅]
		else if (strType == _T("CHAT")) {
			CString strSend;
			if (messageMap.Lookup(_T("content"), strSend))
				m_pClientDlg->DisplayMessage(strSender, strSend, TRUE);
		}
		// [턴 시작 알림]
		else if (strType == _T("StartTurn")) {
			m_pClientDlg->m_bCurrentTurn = true;
			AfxMessageBox(_T("당신의 턴입니다!"));
		}
		// [초기 이름 설정]
		else if (strType == _T("GetName")) {
			CString strMsg;
			CString Name = m_pClientDlg->m_strName;
			strMsg.Format(_T("type:GetName|sender:%s|name:%s"), Name, Name);
			m_pClientDlg->RequestMessage(strMsg);
		}
		// [초기 타일 받기]
		else if (strType == _T("StartTile")) {
			CString strPos, strTileid;
			int nX = 0, nY = 0, nTileid = 0;

			if (messageMap.Lookup(_T("pos"), strPos)) {
				int comma_pos = strPos.Find(_T(","));
				if (comma_pos != -1) {
					nX = _ttoi(strPos.Left(comma_pos));
					nY = _ttoi(strPos.Mid(comma_pos + 1));
				}
			}
			if (messageMap.Lookup(_T("tileid"), strTileid)) {
				nTileid = _ttoi(strTileid);
			}

			// ClientDlg의 public 멤버/함수 사용
			m_pClientDlg->m_private_tile[nX][nY] = m_pClientDlg->ParseIdtoTile(nTileid);
			m_pClientDlg->Invalidate(FALSE);
		}
		// [입장 알림]
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
		// [타일 한 장 받기]
		else if (strType == _T("ReceiveTile")) {
			CString strTileid;
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
				if (received) break;
			}
			m_pClientDlg->Invalidate(FALSE);
		}
	}
}*/

// ClientSocket.h : 헤더 파일

#pragma once
#include <afxsock.h>
#include <vector>
#include <string>

// [핵심 수정] 헤더를 include하지 않고 클래스가 있다고만 알려줌 (순환 참조 방지)
class CClientDlg;

class CClientSocket : public CAsyncSocket
{
public:
	CClientSocket(CClientDlg* pDlg);
	virtual ~CClientSocket();

	// 멤버 변수
	CClientDlg* m_pClientDlg;
	BOOL m_bConnected;

	// 수신 버퍼 관련
	std::vector<char> m_recvBuffer;
	size_t m_nextMessageSize;
	const size_t MAX_MESSAGE_SIZE = 1024 * 1024; // 1MB

	// 오버라이드 함수
	virtual void OnConnect(int nErrorCode);
	virtual void OnReceive(int nErrorCode);
	virtual void OnClose(int nErrorCode);

	// 메시지 처리 함수
	void ProcessExtractedMessage(const std::string& utf8_data);
};