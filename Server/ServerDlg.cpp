
// ServerDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "Server.h"
#include "ServerDlg.h"
#include "afxdialogex.h"
#include "ListenSocket.h"
#include "ServiceSocket.h"
#include "AdressDlg.h"
#include <list>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
public:
//	afx_msg void OnButtonPass();
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
//	ON_COMMAND(IDC_BUTTON_PASS, &CAboutDlg::OnButtonPass)
END_MESSAGE_MAP()


// CServerDlg 대화 상자



CServerDlg::CServerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SERVER_DIALOG, pParent)
	, m_pListenSocket(nullptr)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_strName = _T("");
	m_intPrivateTileNum = 0;
}
CServerDlg::~CServerDlg()
{
	// 리스닝 소켓 정리
	if (m_pListenSocket)
	{
		m_pListenSocket->Close();
		delete m_pListenSocket;
	}

	// 서비스 소켓 목록 정리
	POSITION pos = m_clientSocketList.GetHeadPosition();
	while (pos != NULL)
	{
		CServiceSocket* pSocket = m_clientSocketList.GetNext(pos);
		if (pSocket)
		{
			pSocket->Close();
			delete pSocket;
		}
	}
	m_clientSocketList.RemoveAll();
}

void CServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_LOG, m_list_log);
	DDX_Control(pDX, IDC_EDIT_SEND, m_edit_send);
	//  DDX_Control(pDX, IDC_LIST_MESSAGE, m_list_Message);
	DDX_Control(pDX, IDC_LIST_MESSAGE, m_list_message);
}

BEGIN_MESSAGE_MAP(CServerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()

	ON_BN_CLICKED(IDC_BUTTON_SEND, &CServerDlg::OnBnClickedButtonSend)
	ON_BN_CLICKED(IDC_BUTTON_START, &CServerDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_RECEIVE, &CServerDlg::OnBnClickedButtonReceive)
	ON_BN_CLICKED(IDC_BUTTON_PASS, &CServerDlg::OnClickedButtonPass)
END_MESSAGE_MAP()


// CServerDlg 메시지 처리기

BOOL CServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.

	m_strName = _T("서버");

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);

	}
	else
	{


		CPaintDC dc(this); 

		CPen line_pen(PS_DOT, 1, RGB(80, 80, 80));
		CPen* p_old_pen = dc.SelectObject(&line_pen);
		int i = 0;
		for (i = 1; i < 15; i++) {
			dc.MoveTo(35, 35 * i);
			dc.LineTo(980, 35 * i);
		}
		for (i = 1; i < 29; i++) {
			dc.MoveTo(35 * i, 35);
			dc.LineTo(35 * i, 490);
		}
		dc.SelectObject(p_old_pen);
		line_pen.DeleteObject();
		for (i = 0; i < 4; i++) {
			dc.MoveTo(105, 555 + 35 * i);
			dc.LineTo(700, 555 + 35 * i);

		}
		for (i = 0; i < 18; i++) {
			dc.MoveTo(105 + 35 * i, 555);
			dc.LineTo(105 + 35 * i, 555 + 35 * 3);
		}
		CDialogEx::OnPaint();


	}
}


void CServerDlg::AddLog(const CString& strMsg)
{
	// 현재 시간 추가
	CTime time = CTime::GetCurrentTime();
	CString strTime = time.Format(_T("[%H:%M:%S] "));

	// ListBox에 문자열 추가 및 스크롤 이동
	m_list_log.AddString(strTime + strMsg);
	m_list_log.SetTopIndex(m_list_log.GetCount() - 1);
}
void CServerDlg::DisplayMessage(const CString& strSender, const CString& strMsg, BOOL bReceived)
{
	CString strOutput;

	strOutput.Format(_T("[%s] %s"),strSender, strMsg);
	

	m_list_message.AddString(strOutput);
	m_list_message.SetTopIndex(m_list_message.GetCount() - 1);
}
// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CServerDlg::OnBnClickedButton1()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CServerDlg::OnBnClickedButtonSend()
{
	CString strContent;
	m_edit_send.GetWindowText(strContent);
	
	CString strType = _T("CHAT");
	CString strMsg;
	strMsg.Format(_T("type:%s|sender:%s|content:%s"), strType, m_strName, strContent);
	BroadcastMessage(strMsg, 0);
	// pSocket이 0이니까 전부한테 보낸거임
	m_edit_send.SetWindowText(_T(""));
	DisplayMessage(m_strName, strContent, FALSE);
}

void CServerDlg::OnBnClickedButtonStart()
{
	CAdressDlg pAddressDlg;
	
	INT_PTR nResponse = pAddressDlg.DoModal();

	if (nResponse == IDOK) {
		CString strServerIP = pAddressDlg.m_strIPAddress;
		if (pAddressDlg.m_strName.IsEmpty() ) m_strName = _T("서버");
		else m_strName = pAddressDlg.m_strName;

		//  1. AfxSocketInit()은 CWinApp::InitInstance()에서 이미 호출되었다고 가정합니다.
		UINT nPort = 12345; // 클라이언트와 동일한 포트 사용

		m_pListenSocket = new CListenSocket(this);

		// 2. 소켓 생성 및 바인딩
		if (!m_pListenSocket->Create(nPort, SOCK_STREAM, FD_ACCEPT, strServerIP))
		{
			AddLog(_T("ERROR: 리스닝 소켓 생성 실패!"));
			delete m_pListenSocket;
			m_pListenSocket = nullptr;
			return;
		}

		// 3. 리스닝 시작 (비동기 대기)
		if (!m_pListenSocket->Listen())
		{
			AddLog(_T("ERROR: 리스닝 시작 실패!"));
			m_pListenSocket->Close();
			delete m_pListenSocket;
			m_pListenSocket = nullptr;
			return;
		}

		CString strLog;
		strLog.Format(_T("서버 시작 성공! 포트 %d에서 연결 대기 중..."), nPort);
		AddLog(strLog);
		// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	}
}



void CServerDlg::ProcessAccept(CListenSocket* pListenSocket)
{// 1. 새로운 CServiceSocket 객체를 생성합니다.
	CServiceSocket* pNewSocket = new CServiceSocket(this);

	// NULL 체크 추가: 메모리 할당 실패 대비
	if (pNewSocket == nullptr)
	{
		AddLog(_T("FATAL ERROR: CServiceSocket 메모리 할당 실패!"));
		return;
	}

	// 2. 리스닝 소켓의 Accept 함수를 호출하여 연결을 새 소켓에 할당
	// ⭐️ 여기서 Accept를 호출합니다. (중복 호출 없음)
	if (pListenSocket->Accept(*pNewSocket))
	{
		// 3. 연결 수락 성공! FD_READ 이벤트 등록 (클라이언트 메시지 수신 활성화)
		pNewSocket->AsyncSelect(FD_READ | FD_CLOSE);

		// 4. 소켓 목록에 추가하여 관리
		m_clientSocketList.AddTail(pNewSocket);

		CString strLog;
		strLog.Format(_T("INFO: 클라이언트 연결 수락됨 (현재 %d명)"), m_clientSocketList.GetCount());
		AddLog(strLog);
	}
	else
	{
		// 5. 실패 시, 오류 코드 확인 및 객체 해제
		DWORD dwError = GetLastError();

		CString strError;
		strError.Format(_T("ERROR: 클라이언트 연결 수락 실패! (WSA 오류 코드: %d)"), dwError);
		AddLog(strError);

		delete pNewSocket;
	}
}

void CServerDlg::RemoveClient(CServiceSocket* pServiceSocket)
{
	// 1. 목록에서 해당 포인터를 찾습니다.
	POSITION pos = m_clientSocketList.Find(pServiceSocket);

	if (pos != NULL)
	{
		// 2. 목록에서 제거
		m_clientSocketList.RemoveAt(pos);

		// 3. 메모리 해제
		delete pServiceSocket;

		CString strLog;
		strLog.Format(_T("INFO: 클라이언트 연결 해제됨 (현재 %d명)"), m_clientSocketList.GetCount());
		AddLog(strLog);
	}
}


void CServerDlg::BroadcastMessage(const CString& strMsg, CServiceSocket* pSender)
{
	//  전송을 위한 CString -> LPCSTR 변환 준비
	USES_CONVERSION;

	// 1. 연결된 클라이언트 목록 순회
	POSITION pos = m_clientSocketList.GetHeadPosition();

	// 2. 서버 로그에 브로드캐스트 사실 기록
	CString strLog;
	

	while (pos != NULL)
	{
		CServiceSocket* pSocket = m_clientSocketList.GetNext(pos);

		// 유효한 소켓인지 확인하고, 연결 상태인지 확인합니다.
		if (pSocket && pSocket->m_hSocket != NULL && pSocket->IsConnected())
		{
			// (선택 사항) 메시지를 보낸 클라이언트에게는 다시 보내지 않으려면 아래 주석 해제
			 if (pSocket == pSender) continue;

			//  3. 메시지 전송 (Send 함수 사용
			 std::string utf8_data = CStringToUTF8(strMsg);
			 strLog.Format(_T("BROADCAST: %s"), strMsg);
			 AddLog(strLog);
			 // 2. 소켓을 통해 서버로 데이터 전송
			 // CAsyncSocket::Send 함수는 비동기로 작동하며, 성공 시 보낸 바이트 수를 반환
			 int nBytesSent = pSocket->Send(utf8_data.c_str(), (int)utf8_data.length()); //  
			// 전송 오류 처리
			if (nBytesSent == SOCKET_ERROR)
			{
				// 오류가 발생하면 해당 소켓의 연결을 강제로 끊거나 로그를 남길 수 있습니다.
				pSocket->Close();
			}
		}
	}
}




void CServerDlg::ResponseMessage(const CString& strMsg, CServiceSocket* pSender) {
	//  전송을 위한 CString -> LPCSTR 변환 준비
	USES_CONVERSION;

	// 1. 연결된 클라이언트 목록 순회
	POSITION pos = m_clientSocketList.GetHeadPosition();

	// 2. 서버 로그에 브로드캐스트 사실 기록
	CString strLog;

	// 유효한 소켓인지 확인하고, 연결 상태인지 확인합니다.
	if (pSender && pSender->m_hSocket != NULL && pSender->IsConnected())
	{
		// (선택 사항) 메시지를 보낸 클라이언트에게는 다시 보내지 않으려면 아래 주석 해제

		//  3. 메시지 전송 (Send 함수 사용
		std::string utf8_data = CStringToUTF8(strMsg);
		strLog.Format(_T("BROADCAST: %s"), strMsg);
		AddLog(strLog);
		// 2. 소켓을 통해 서버로 데이터 전송
		// CAsyncSocket::Send 함수는 비동기로 작동하며, 성공 시 보낸 바이트 수를 반환
		int nBytesSent = pSender->Send(utf8_data.c_str(), (int)utf8_data.length()); //  
		// 전송 오류 처리
		if (nBytesSent == SOCKET_ERROR)
		{
			// 오류가 발생하면 해당 소켓의 연결을 강제로 끊거나 로그를 남길 수 있습니다.
			pSender->Close();
		}
	}

}


void CServerDlg::OnBnClickedButtonReceive()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.


}

bool CServerDlg::IsPublicTileValid()
{
	/*
	공용판이 올바른지 공용판의 각 행 별로 검사하는 메소드.

	테스트 케이스 예시)
	유효한 경우:
	- 빨강 1-2-3
	- 파랑 10-11-12-13
	- 빨강7, 파랑7, 검정7
	- 빨강 5-조커-7 (런)
	- 빨강9, 조커, 파랑9 (그룹)
	무효한 경우:
	- 빨강 1-3-5 (연속 안됨)
	- 빨강7, 빨강7, 파랑7 (색 중복)
	- 빨강 1-2 (3개 미만)
	- 빨강 1-2-파랑3 (색 다름)
	*/

	for (int i = 1; i <= 13; i++) // 공용판의 각 행에 대해 반복
	{
		if (!IsRowValid(i))
		{
			return false;
		}
	}

	return true;
}

bool CServerDlg::IsRowValid(int row)
{
	/*
	특정 행의 타일 조합들이 모두 유효한지 검사

	런과 그룹은 다음과 같이 정의함
	- 런(Run): 같은 색의 연속된 숫자 (예: 빨강 3-4-5)
	- 그룹(Group): 같은 숫자의 다른 색 (예: 빨강7, 파랑7, 검정7)

	청크(chunk)는 행에서 인접한 타일들의 묶음으로, 조건 검사의 대상임
	*/
	std::list<Tile> tileChunk; // 현재 검사 중인 타일 그룹

	for (int i = 1; i <= 27; i++)
	{
		Tile currentTile = m_public_tile[row][i];

		// 빈 칸을 만나거나 마지막 칸일 때, chunk가 비어있지 않은 경우(검사 필요)
		if (((currentTile.num == 0) || (i == 27)) && (!tileChunk.empty()))
		{
			// chunk 검증
			if (IsRunValid(tileChunk) || IsGroupValid(tileChunk))
			{
				tileChunk.clear();
				continue;
			}
			else
			{
				return false;
			}
		}
		else
		{
			// 타일을 그룹에 추가
			tileChunk.push_back(currentTile);
		}
	}

	return true;
}

bool CServerDlg::IsRunValid(std::list<Tile> tileChunk)
{
	/*
	런(Run) 검사: 같은 색의 연속된 숫자
	예: 빨강 3-4-5, 파랑 10-11-12-13
	조커는 빠진 숫자를 대체 가능
	*/

	// 최소 3개 필요
	if (tileChunk.size() < 3)
	{
		return false;
	}

	// 조커 개수 세기
	int jokerCount = 0;
	for (const Tile& t : tileChunk)
	{
		if (t.isJoker)
		{
			jokerCount++;
		}
	}

	// 일반 타일의 색상 찾기 (기준 색상)
	Color runColor = BLACK;
	for (const Tile& t : tileChunk)
	{
		if (!t.isJoker)
		{
			runColor = t.color;
			break;
		}
	}

	// 연속된 숫자 검사
	int expectedNum = -1;
	bool firstTileSet = false;

	for (const Tile& t : tileChunk)
	{
		if (t.isJoker)
		{
			// 조커는 다음 숫자로 간주
			if (expectedNum != -1)
			{
				expectedNum++;
			}
			// 첫 타일이 조커면 다음 일반 타일로 숫자 결정(그냥 패스하면 됨)
		}
		else
		{
			// 색상 일치 확인
			if (t.color != runColor)
			{
				return false;
			}

			// 첫 번째 일반 타일
			if (!firstTileSet)
			{
				expectedNum = t.num;
				firstTileSet = true;
			}
			else
			{
				// 숫자 연속성 확인
				if (t.num != expectedNum)
				{
					return false;
				}
			}

			expectedNum++;
		}
	}

	return true;
}

bool CServerDlg::IsGroupValid(std::list<Tile> tileChunk)
{
	/*
	그룹(Group) 검사: 같은 숫자의 다른 색
	예: 빨강7, 파랑7, 검정7
	조커는 부족한 색을 대체 가능
	*/

	// 최소 3개, 최대 4개 (4가지 색상)
	if (tileChunk.size() < 3 || tileChunk.size() > 4)
	{
		return false;
	}

	// 일반 타일의 숫자 찾기 (기준 숫자)
	int groupNum = -1;
	for (const Tile& t : tileChunk)
	{
		if (!t.isJoker)
		{
			groupNum = t.num;
			break;
		}
	}

	// 색상 중복 체크 배열 (RED=0, GREEN=1, BLUE=2, BLACK=3)
	bool usedColors[4] = { false, false, false, false };

	for (const Tile& t : tileChunk)
	{
		// 조커는 색상 체크 건너뜀
		if (t.isJoker)
		{
			continue;
		}

		// 숫자 일치 확인
		if (t.num != groupNum)
		{
			return false;
		}

		// 색상 중복 확인
		if (usedColors[t.color])
		{
			return false; // 같은 색 중복 불가
		}
		usedColors[t.color] = true;
	}

	return true;
}

void CServerDlg::OnClickedButtonPass()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	if (IsPublicTileValid()) // 공용판이 올바른 경우
	{
		// 모든 클라이언트에 타일 개수 최신화 요청 전송
		CString requestMsg;
		requestMsg.Format(_T("type:UpdateTileNum|sender:%s|tilenum:%d"), m_strName, m_intPrivateTileNum);
		BroadcastMessage(requestMsg, 0);

		// 턴 종료

	}
	else // 공용판이 올바르지 않은 경우
	{
		AfxMessageBox(_T("공용판이 올바르지 않습니다.", MB_OK));
	}
}


