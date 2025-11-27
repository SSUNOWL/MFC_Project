
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
#include <algorithm>
#include <chrono>
#include <random>
#include <atlimage.h>
#include <comdef.h>
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
	//  m_intTurnPos = 0;
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
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &CServerDlg::OnBnClickedButtonPlay)
	ON_BN_CLICKED(IDC_BUTTON_PASS, &CServerDlg::OnBnClickedButtonPass)
	ON_BN_CLICKED(IDC_BUTTON_SetBack, &CServerDlg::OnClickedButtonSetback)
	ON_WM_DESTROY()
	// [251127] 마우스 클릭 메시지 등록
	ON_WM_LBUTTONDOWN()
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
	InitTiles();
	m_bisGameStarted = FALSE;
	LoadImage();
	ShuffleTiles();
	m_intPrivateTileNum = 0;

	// [251127] 선택 상태 변수 초기화
	m_bIsSelected = false;
	m_nSelectedRow = -1;
	m_nSelectedCol = -1;
	m_bSelectedFromPublic = false;

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

		DrawMyTiles(dc);

		// [251127] 선택된 타일 빨간 테두리 표시
		// ==========================================
		if (m_bIsSelected)
		{
			CPen redPen(PS_SOLID, 3, RGB(255, 0, 0));
			CPen* pOldPen = dc.SelectObject(&redPen);
			CBrush* pOldBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);

			int startX = 0, startY = 0;

			// 좌표 계산 (서버와 클라이언트 좌표계 동일)
			if (m_bSelectedFromPublic)
			{
				// 공용판: (35, 35) 시작
				startX = 35 + (m_nSelectedCol - 1) * 35;
				startY = 35 + (m_nSelectedRow - 1) * 35;
			}
			else
			{
				// 개인판: (105, 555) 시작
				startX = 105 + (m_nSelectedCol - 1) * 35;
				startY = 555 + (m_nSelectedRow - 1) * 35;
			}

			// 테두리 그리기 (35x35)
			dc.Rectangle(startX, startY, startX + 35, startY + 35);

			dc.SelectObject(pOldPen);
			dc.SelectObject(pOldBrush);
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
	// 여기서 Accept를 호출합니다. (중복 호출 없음)
	if (pListenSocket->Accept(*pNewSocket))
	{
		// 3. 연결 수락 성공! FD_READ 이벤트 등록 (클라이언트 메시지 수신 활성화)
		pNewSocket->AsyncSelect(FD_READ | FD_CLOSE);
		CString strMsg;
		strMsg.Format(_T("type:GetName|sender:%s"), m_strName);
		ResponseMessage(strMsg, pNewSocket);
		// 4. 소켓 목록에 추가하여 관리 
		m_clientSocketList.AddTail(pNewSocket);

		/*CString strLog;
		strLog.Format(_T("INFO: 클라이언트 연결 수락됨 (현재 %d명)"), m_clientSocketList.GetCount());
		AddLog(strLog);*/
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
	USES_CONVERSION;

	POSITION pos = m_clientSocketList.GetHeadPosition();

	std::string utf8_data = CStringToUTF8(strMsg);
	// 1. 메시지 길이를 4바이트 정수형으로 준비합니다. (루프 밖에서 한 번만 계산)
	int nLength = (int)utf8_data.length();

	// 2. 서버 로그 기록
	CString strLog;
	strLog.Format(_T("BROADCAST: %s"), strMsg);
	AddLog(strLog);

	while (pos != NULL)
	{
		CServiceSocket* pSocket = m_clientSocketList.GetNext(pos);

		if (pSocket && pSocket->m_hSocket != NULL && pSocket->IsConnected())
		{
			// 메시지를 보낸 클라이언트 제외
			if (pSocket == pSender) continue;

			// 3. **[수정]** 길이 헤더 (4바이트)를 먼저 전송합니다.
			int nHeaderSent = pSocket->Send(&nLength, sizeof(nLength));

			// 4. **[수정]** 실제 메시지 본문 (Payload)을 전송합니다.
			int nDataSent = pSocket->Send(utf8_data.c_str(), nLength);

			// 전송 오류 처리
			if (nHeaderSent != sizeof(nLength) || nDataSent != nLength)
			{
				// 오류가 발생하면 해당 소켓의 연결을 강제로 끊거나 로그를 남길 수 있습니다.
				// AddLog(_T("ERROR: Broadcast 전송 실패."));
				pSocket->Close();
			}
		}
	}
}


void CServerDlg::ResponseMessage(const CString& strMsg, CServiceSocket* pSender) {
	USES_CONVERSION;

	if (pSender && pSender->m_hSocket != NULL && pSender->IsConnected())
	{
		std::string utf8_data = CStringToUTF8(strMsg);

		// 1. 메시지 길이를 4바이트 정수형으로 준비합니다.
		int nLength = (int)utf8_data.length();

		// 2. 서버 로그 기록
		CString strLog;
		strLog.Format(_T("RESPONSE: %s (To: %s)"), strMsg, pSender->m_strName);
		AddLog(strLog);

		// 3. **[수정]** 길이 헤더 (4바이트)를 먼저 전송합니다.
		int nHeaderSent = pSender->Send(&nLength, sizeof(nLength));

		// 4. **[수정]** 실제 메시지 본문 (Payload)을 전송합니다.
		int nDataSent = pSender->Send(utf8_data.c_str(), nLength);

		// 전송 오류 처리
		if (nHeaderSent != sizeof(nLength) || nDataSent != nLength)
		{
			// 오류가 발생하면 해당 소켓의 연결을 강제로 끊거나 로그를 남길 수 있습니다.
			// AddLog(_T("ERROR: Response 전송 실패."));
			pSender->Close();
		}
	}
}



void CServerDlg::InitTiles() {
	int k = 0;
	int nTileid = 1;
	// 4색 × (1~13 각 2장) = 104장
	for (int c = RED; c <= BLACK; ++c) {
		for (int n = 1; n <= 13; ++n) {
			m_tile_list[k++] = Tile{ static_cast<Color>(c), n, false, nTileid++};
		}
		for (int n = 1; n <= 13; ++n) {
			m_tile_list[k++] = Tile{ static_cast<Color>(c), n, false, nTileid++ };
		}
	}
	
	// 조커 2장(관례: BLACK, num=0)
	m_tile_list[k++] = Tile{ BLACK, 0, true , nTileid++};
	m_tile_list[k++] = Tile{ BLACK, 0, true , nTileid++};

	/*for (Tile c : m_tile_list) {
		CString s;
		s.Format(_T("%d, %d, %d"), c.num, c.color, c.tileId);

		AddLog(s);

	}*/
}

void CServerDlg::ShuffleTiles() {

	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	m_rand_tile_list = m_tile_list; // 깊은 복사가 구현되어있다고 함
	std::shuffle(m_rand_tile_list.begin(), m_rand_tile_list.end(), std::default_random_engine(seed));

	//확인절차
	/*for (Tile c : m_rand_tile_list) {
		CString s;
		s.Format(_T("%d, %d, %d"), c.num, c.color, c.tileId);

		AddLog(s);
		
	}*/   


}


void CServerDlg::PlayGame() { 
	// 유저 정보 요청해서 다 받음.
	//게임 시작할때 필요되는 초기화 과정 전부 여기서 진행
	
	ShuffleTiles();
	//서버의 턴 시작
	m_posTurn = NULL;
	m_bCurrentTurn = FALSE;
	NextTurn();

	for (int i = 1; i <= 14; i++) {
		m_private_tile[1][i] = m_rand_tile_list[m_deck_pos];
		CString strLog;
		strLog.Format(_T("%d %d %d 타일이 %d %d 개인판에 들어감"), m_private_tile[1][i].color, m_private_tile[1][i].num, m_private_tile[1][i].tileId, 1, i);
		AddLog(strLog);
		m_deck_pos++;
	}

	POSITION pos = m_clientSocketList.GetHeadPosition();

	while (pos != NULL)
	{
		CServiceSocket* pSocket = m_clientSocketList.GetNext(pos);
		for (int i = 1; i <= 14; i++) {
			CString strMsg;
			strMsg.Format(_T("type:StartTile|sender:%s|pos:%d,%d|tileid:%d"), m_strName, 1, i, m_rand_tile_list[m_deck_pos].tileId);
			ResponseMessage(strMsg, pSocket);
			m_deck_pos++;
		}
	}
	//개인 타일판을 시각화하는 함수


}

void CServerDlg::NextTurn() {
	CString strMsg;
	CString strNext;
	CString strBackup;
	if (m_bCurrentTurn == TRUE) {
		m_bCurrentTurn = FALSE;
		m_posTurn = m_clientSocketList.GetHeadPosition();
		CServiceSocket* pTurn = m_clientSocketList.GetNext(m_posTurn);

		strMsg.Format(_T("type:CHAT|sender:시스템|content:%s의 턴이 시작되었습니다"), pTurn->m_strName);
			
		strNext.Format(_T("type:StartTurn|sender:시스템"));
		ResponseMessage(strNext, pTurn); // 턴 넘긴 후
		strBackup.Format(_T("type:Backup|sender:시스템")); // 모두에게 Backup 메시지 발신 -> 현재 턴인 사람의 개인판과 공용판만 백업됨
		BroadcastMessage(strBackup, 0);
	}
	else {
		if (m_posTurn == NULL) {
			m_bCurrentTurn = TRUE;

			// [251127] 내 턴 시작 시점의 판 상태 백업] (이게 있어야 회수 규칙 작동)
			CopyBoards();
			strMsg.Format(_T("type:CHAT|sender:시스템|content:%s의 턴이 시작되었습니다"), m_strName);
			//서버는 보낼필요 없음
			Backup(); // 서버의 개인판, 공용판 백업 (Setback을 위해)
		}
		else {
			CServiceSocket* pTurn = m_clientSocketList.GetNext(m_posTurn);
			strMsg.Format(_T("type:CHAT|sender:시스템|content:%s의 턴이 시작되었습니다"), pTurn->m_strName);

			strNext.Format(_T("type:StartTurn|sender:시스템"));
			ResponseMessage(strNext, pTurn);
		}
	}
	BroadcastMessage(strMsg, 0); //현재 턴에 대한 정보는 모두에게 공유되어야함
	
	DisplayMessage(_T("시스템"), strMsg, 1);

}
void CServerDlg::Receive() {
	bool received = false;
	for (int i = 1; i <= 3; i++) {
		for (int j = 1; j <= 17; j++)
			if (m_private_tile[i][j].tileId == -1) { // 비어있는 공간에 패를 넣기 위해서 조건 검사
				m_private_tile[i][j] = m_rand_tile_list[m_deck_pos++];
				CString strLog;
				strLog.Format(_T("%d %d %d 타일이 %d %d 개인판에 들어감"), m_private_tile[i][j].color, m_private_tile[i][j].num, m_private_tile[i][j].tileId, i, j);
				AddLog(strLog);
				received = true;
				break;
			}
		if (received == true)
			break;
	}
}

void CServerDlg::OnBnClickedButtonReceive() {
	
	if (m_bCurrentTurn == true) {
		//Setback(); // 추후 Setback 구현되면 Setback -> 패 받기 -> 턴 넘기기로 진행
		Receive(); // 패 한장 받기
		NextTurn(); // 다음 차례로 넘기기
	}
	Invalidate(FALSE);
}

bool CServerDlg::IsPublicTileValid()
{
	/*
	공용판이 올바른지 공용판의 각 행 별로 검사하는 메소드.
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
	런(Run): 같은 색의 연속된 숫자 (예: 빨강 3-4-5)
	그룹(Group): 같은 숫자의 다른 색 (예: 빨강7, 파랑7, 검정7)

	청크(chunk)는 행에서 인접한 타일들의 묶음으로, 조건 검사의 단위임
	*/
	std::list<Tile> tileChunk; // 현재 검사 중인 타일 그룹

	for (int i = 1; i <= 27; i++)
	{
		Tile currentTile = m_public_tile[row][i];

		// 빈 칸을 만나거나 마지막 칸일 때, chunk가 비어있지 않은 경우(검사 필요)
		if (((currentTile.num == 0) && (!currentTile.isJoker)) || (i == 27))
		{
			if (tileChunk.empty())
			{
				continue;
			}

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

void CServerDlg::OnBnClickedButtonPass()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	if (!m_bCurrentTurn) {
		AfxMessageBox(_T("턴이 돌아오지 않았습니다.", MB_OK));

		return;
	}

	if (IsPublicTileValid()) // 공용판이 올바른 경우
	{
		// 모든 클라이언트에 타일 개수 최신화 요청 전송
		CString requestMsg;
		requestMsg.Format(_T("type:UpdateTileNum|sender:%s|tilenum:%d"), m_strName, m_intPrivateTileNum);
		BroadcastMessage(requestMsg, 0);

		// 턴 종료
		NextTurn();
	}
	else // 공용판이 올바르지 않은 경우
	{
		AfxMessageBox(_T("공용판이 올바르지 않습니다.", MB_OK));
	}
	Invalidate(FALSE);
}



void CServerDlg::OnBnClickedButtonPlay()
{
	if (m_clientSocketList.GetCount() < 1) {
		AfxMessageBox(_T("다른 플레이어를 기다려야합니다."), MB_OK | MB_ICONWARNING);
	}
	else {
		if (!m_bisGameStarted) {
			PlayGame();
			m_bisGameStarted = TRUE;

			CString requestMsg;
			requestMsg.Format(_T("type:GameStarted"));
			BroadcastMessage(requestMsg, 0);

			Invalidate(FALSE);

			AfxMessageBox(_T("게임이 시작되었습니다.", MB_OK));
		}
	}
}


void CServerDlg::LoadImage()
{
	int j = 0;

	// 1) 빨강 타일 (0~12, 13~25)
	for (int i = 0; i < 13; i++, j++) {
		LoadPngFromResource(m_tile_image_list[i], IDB_PNG54 - j);
		LoadPngFromResource(m_tile_image_list[i + 13], IDB_PNG54 - j);
	}

	// 2) 파랑 타일 (26~38, 39~51)
	for (int i = 26; i < 39; i++, j++) {
		LoadPngFromResource(m_tile_image_list[i], IDB_PNG54 - j);
		LoadPngFromResource(m_tile_image_list[i + 13], IDB_PNG54 - j);
	}

	// 3) 노랑 타일 (52~64, 65~77)
	for (int i = 52; i < 65; i++, j++) {
		LoadPngFromResource(m_tile_image_list[i], IDB_PNG54 - j);
		LoadPngFromResource(m_tile_image_list[i + 13], IDB_PNG54 - j);
	}

	// 4) 초록 타일 (78~90, 91~103)
	for (int i = 78; i < 91; i++, j++) {
		LoadPngFromResource(m_tile_image_list[i], IDB_PNG54 - j);
		LoadPngFromResource(m_tile_image_list[i + 13], IDB_PNG54 - j);
	}

	// 조커 2장 (104, 105)
	LoadPngFromResource(m_tile_image_list[104], IDB_PNG2);
	LoadPngFromResource(m_tile_image_list[105], IDB_PNG1);
}

bool CServerDlg::LoadPngFromResource(CImage& img, UINT uResID)
{
	HINSTANCE hInst = AfxGetInstanceHandle();

	// 리소스 찾기 (타입 "PNG")
	HRSRC hRes = ::FindResource(hInst, MAKEINTRESOURCE(uResID), L"PNG");
	if (!hRes) return false;

	DWORD size = ::SizeofResource(hInst, hRes);
	if (size == 0) return false;

	HGLOBAL hGlobal = ::LoadResource(hInst, hRes);
	if (!hGlobal) return false;

	void* pData = ::LockResource(hGlobal);
	if (!pData) return false;

	// 메모리 복사해서 IStream 만들기
	HGLOBAL hBuffer = ::GlobalAlloc(GMEM_MOVEABLE, size);
	if (!hBuffer) return false;

	void* pBuffer = ::GlobalLock(hBuffer);
	memcpy(pBuffer, pData, size);
	::GlobalUnlock(hBuffer);

	IStream* pStream = nullptr;
	if (FAILED(::CreateStreamOnHGlobal(hBuffer, TRUE, &pStream)))
	{
		::GlobalFree(hBuffer);
		return false;
	}

	img.Destroy(); // 혹시 이전 이미지 있으면 정리
	HRESULT hr = img.Load(pStream);

	pStream->Release(); // hBuffer도 같이 해제됨

	return SUCCEEDED(hr);
}

int CServerDlg::GetTileImageIndex(const Tile& tile) const
{
	// 조커
	if (tile.isJoker)
	{
		// 두 장이 모양 같으면 그냥 104로 통일해도 되고,
		// tileId 짝/홀로 104/105 번갈아 써도 됨
		return (tile.tileId % 2 == 0) ? 104 : 105;
	}

	// RED ~ BLACK 이 연속 enum이라고 가정
	int colorIndex = static_cast<int>(tile.color) - static_cast<int>(RED);
	if (colorIndex < 0 || colorIndex > 3)
		return -1; // BLACK 일반 타일 같은 건 없음

	int numIndex = tile.num - 1;   // 1~13 -> 0~12
	if (numIndex < 0 || numIndex >= 13)
		return -1;

	// 각 색당 26칸(같은 숫자 2장)이니까 첫 번째 장만 사용
	// RED : 0~25, BLUE : 26~51, ... 순서로 LoadImage에서 채워놨다고 가정
	int base = colorIndex * 26;

	return base + numIndex;   // 첫 번째 세트만 사용 (0~90 범위)
}

void CServerDlg::DrawMyTiles(CDC& dc)
{
    // 공통 상수
    const int CELL_SIZE      = 35;
    const int TILE_DRAW_SIZE = 32;
    const int OFFSET         = (CELL_SIZE - TILE_DRAW_SIZE) / 2 + 1;

    // 실제 타일 내용 영역 크기 (여백 제외하고 쓰고 싶은 크기)
    const int CONTENT_W = 68;
    const int CONTENT_H = 68;

    // === 공용판 그리기 ===
    const int PUBLIC_START_X = 35;
    const int PUBLIC_START_Y = 35;

    for (int row = 1; row <= 13; ++row)
    {
        for (int col = 1; col <= 27; ++col)
        {
            const Tile& t = m_public_tile[row][col];

            // 빈 타일 스킵 (BLACK, 0, false 기준)
            if (t.color == BLACK && t.num == 0 && !t.isJoker)
                continue;

            int imgIndex = GetTileImageIndex(t);
            if (imgIndex < 0) continue;
            if (m_tile_image_list[imgIndex].IsNull()) continue;

            // 원본 이미지 크기
            int imgW = m_tile_image_list[imgIndex].GetWidth();
            int imgH = m_tile_image_list[imgIndex].GetHeight();

            // 원본 중앙에서 68x68만 잘라오기
            int srcW = min(CONTENT_W, imgW);
            int srcH = min(CONTENT_H, imgH);
            int srcX = max(0, (imgW - srcW) / 2 - 1);
            int srcY = max(0, (imgH - srcH) / 2 - 1);

            int drawX = PUBLIC_START_X + (col - 1) * CELL_SIZE + OFFSET;
            int drawY = PUBLIC_START_Y + (row - 1) * CELL_SIZE + OFFSET;

            // 잘라낸 영역(srcX,srcY,srcW,srcH)을 30x30으로 축소해서 셀 안에 그림
            m_tile_image_list[imgIndex].Draw(
                dc,
                drawX, drawY, TILE_DRAW_SIZE, TILE_DRAW_SIZE,  // 목적지(보드) 영역
                srcX,  srcY,  srcW,            srcH            // 원본에서 자를 영역
            );
        }
    }

    // === 개인판 그리기 ===
    const int PRIVATE_START_X = 105;
    const int PRIVATE_START_Y = 555;

    for (int row = 1; row <= 3; ++row)
    {
        for (int col = 1; col <= 17; ++col)
        {
            const Tile& t = m_private_tile[row][col];

            // 빈 타일 스킵
            if (t.color == BLACK && t.num == 0 && !t.isJoker)
                continue;

            int imgIndex = GetTileImageIndex(t);
            if (imgIndex < 0) continue;
            if (m_tile_image_list[imgIndex].IsNull()) continue;

            int imgW = m_tile_image_list[imgIndex].GetWidth();
            int imgH = m_tile_image_list[imgIndex].GetHeight();

            int srcW = min(CONTENT_W, imgW);
            int srcH = min(CONTENT_H, imgH);
            int srcX = max(0, (imgW - srcW) / 2);
            int srcY = max(0, (imgH - srcH) / 2);

            int drawX = PRIVATE_START_X + (col - 1) * CELL_SIZE + OFFSET;
            int drawY = PRIVATE_START_Y + (row - 1) * CELL_SIZE + OFFSET;

            m_tile_image_list[imgIndex].Draw(
                dc,
                drawX, drawY, TILE_DRAW_SIZE, TILE_DRAW_SIZE,
                srcX,  srcY,  srcW,            srcH
            );
        }
    }
}

void CServerDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	if (m_bisGameStarted) // 게임 진행 중에 서버가 탈주한 경우
	{
		CString requestMsg;
		requestMsg.Format(_T("type:EndGame|isNormalEnd:0"));
		BroadcastMessage(requestMsg, 0);
	}
}

// ServerDlg.cpp - OnLButtonDown 함수 전체 수정

void CServerDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// 1. 클릭된 위치 확인 (기존 로직)
	bool bClickedPublic = false;
	bool bClickedPrivate = false;
	int nRow = -1, nCol = -1;

	// 공용판 범위
	if (point.x >= 35 && point.x < 35 + 27 * 35 &&
		point.y >= 35 && point.y < 35 + 13 * 35)
	{
		bClickedPublic = true;
		nCol = (point.x - 35) / 35 + 1;
		nRow = (point.y - 35) / 35 + 1;
	}
	// 개인판 범위
	else if (point.x >= 105 && point.x < 105 + 17 * 35 &&
		point.y >= 555 && point.y < 555 + 3 * 35)
	{
		bClickedPrivate = true;
		nCol = (point.x - 105) / 35 + 1;
		nRow = (point.y - 555) / 35 + 1;
	}
	else
	{
		if (m_bIsSelected) {
			m_bIsSelected = false; Invalidate(TRUE);
		}
		CDialogEx::OnLButtonDown(nFlags, point);
		return;
	}

	// 2. 타일 선택 또는 이동 로직
	if (!m_bIsSelected)
	{
		// [선택 시도]

		// [규칙 1] 내 턴이 아닐 때 공용판 조작 금지
		if (!m_bCurrentTurn && bClickedPublic)
		{
			// AfxMessageBox(_T("내 턴이 아닙니다.")); 
			return;
		}

		Tile t;
		if (bClickedPublic) t = m_public_tile[nRow][nCol];
		else t = m_private_tile[nRow][nCol];

		if (!(t.color == BLACK && t.num == 0 && !t.isJoker))
		{
			m_bIsSelected = true;
			m_bSelectedFromPublic = bClickedPublic;
			m_nSelectedRow = nRow;
			m_nSelectedCol = nCol;
			Invalidate(TRUE);
		}
	}
	else
	{
		// [이동 시도]

		// 제자리 클릭 시 취소
		if (m_bSelectedFromPublic == bClickedPublic &&
			m_nSelectedRow == nRow && m_nSelectedCol == nCol)
		{
			m_bIsSelected = false; Invalidate(TRUE); return;
		}

		// [규칙 1] 내 턴이 아닐 때 제출 금지
		if (!m_bCurrentTurn && bClickedPublic)
		{
			AfxMessageBox(_T("내 턴이 아닐 때는 타일을 제출할 수 없습니다."));
			m_bIsSelected = false; Invalidate(TRUE); return;
		}

		// [규칙 2] 공용판 -> 개인판 이동 시 '기존 타일'인지 검사
		if (m_bSelectedFromPublic && bClickedPrivate)
		{
			Tile& selectedTile = m_public_tile[m_nSelectedRow][m_nSelectedCol];
			if (IsExistingPublicTile(selectedTile.tileId))
			{
				AfxMessageBox(_T("기존에 있던 타일은 가져올 수 없습니다.\n(이번 턴에 낸 타일만 회수 가능)"));
				m_bIsSelected = false; Invalidate(TRUE); return;
			}
		}

		// --- 데이터 교환 (Swap) ---
		Tile* pSourceTile = m_bSelectedFromPublic
			? &m_public_tile[m_nSelectedRow][m_nSelectedCol]
			: &m_private_tile[m_nSelectedRow][m_nSelectedCol];

		Tile* pTargetTile = bClickedPublic
			? &m_public_tile[nRow][nCol]
			: &m_private_tile[nRow][nCol];

		Tile temp = *pTargetTile;
		*pTargetTile = *pSourceTile;
		*pSourceTile = temp;

		// --- 변경 사항 브로드캐스트 (서버 -> 클라) ---
		if (m_bSelectedFromPublic) SendUpdatePublicTile(m_nSelectedRow, m_nSelectedCol);
		if (bClickedPublic) SendUpdatePublicTile(nRow, nCol);

		m_bIsSelected = false;
		Invalidate(TRUE);
	}

	CDialogEx::OnLButtonDown(nFlags, point);
}

//  [251127] ID를 기반으로 Tile 객체를 반환하는 헬퍼 함수
CServerDlg::Tile CServerDlg::GetTileFromId(int tileId)
{
	if (tileId == -1) return MakeEmpty(); // 빈 타일

	Tile t;
	t.tileId = tileId;
	t.isJoker = false;

	if (tileId >= 105) // 조커
	{
		t.color = BLACK;
		t.num = 0;
		t.isJoker = true;
	}
	else // 일반 타일 (1~104)
	{
		int adjustedId = tileId - 1;
		int colorIdx = adjustedId / 26; // 0:RED, 1:GREEN, 2:BLUE, 3:BLACK
		t.color = static_cast<Color>(colorIdx);
		t.num = (adjustedId % 13) + 1;
	}
	return t;
}

// [251127] 특정 좌표(row, col)의 공용판 타일 정보를 모든 클라이언트에게 전송
void CServerDlg::SendUpdatePublicTile(int row, int col)
{
	Tile t = m_public_tile[row][col];

	// 빈 타일일 경우 ID를 -1로 설정하여 전송
	int id = (t.color == BLACK && t.num == 0 && !t.isJoker) ? -1 : t.tileId;

	// 메시지 프로토콜: type:PLACE|pos:row,col|tileid:id
	CString strMsg;
	strMsg.Format(_T("type:PLACE|pos:%d,%d|tileid:%d"), row, col, id);

	// 0(nullptr)을 넣으면 연결된 '모든' 클라이언트에게 전송함
	BroadcastMessage(strMsg, 0);
}

// [251127] 클라이언트로부터 받은 공용판 수정 메시지(PLACE)를 처리
void CServerDlg::ProcessPublicBoardUpdate(CString strMsg)
{
	// 메시지 파싱 예: "type:PLACE|pos:3,4|tileid:15"

	int nPosStart = strMsg.Find(_T("pos:"));
	int nIdStart = strMsg.Find(_T("tileid:"));

	if (nPosStart == -1 || nIdStart == -1) return;

	// 1. 좌표 파싱
	CString strPos = strMsg.Mid(nPosStart + 4, nIdStart - (nPosStart + 4) - 1); // "3,4"
	int nComma = strPos.Find(_T(","));
	int nRow = _ttoi(strPos.Left(nComma));
	int nCol = _ttoi(strPos.Mid(nComma + 1));

	// 2. TileID 파싱
	CString strId = strMsg.Mid(nIdStart + 7);
	int nId = _ttoi(strId);

	// 3. 서버 공용판 업데이트
	m_public_tile[nRow][nCol] = GetTileFromId(nId);

	// 4. 화면 갱신
	Invalidate(TRUE);

	// 5. 다른 모든 클라이언트에게도 이 변경사항을 전파 (동기화)
	// (서버가 받았으니, 다른 클라이언트들도 알 수 있게 그대로 브로드캐스트)
	BroadcastMessage(strMsg, 0);
}

// [251127]
CServerDlg::Tile CServerDlg::MakeEmpty()
{
	// 빈 타일 반환 (검정, 숫자0, 조커아님, ID -1)
	Tile t;
	t.color = BLACK;
	t.num = 0;
	t.isJoker = false;
	t.tileId = -1;
	return t;
}

void CServerDlg::CopyBoards()
{
	// 현재 공용판 상태를 Old(백업) 배열에 복사
	for (int i = 0; i < 14; i++)
		for (int j = 0; j < 28; j++)
			m_old_public_tile[i][j] = m_public_tile[i][j];

	// 개인판도 백업 (필요 시)
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 18; j++)
			m_old_private_tile[i][j] = m_private_tile[i][j];
}

bool CServerDlg::IsExistingPublicTile(int tileId)
{
	if (tileId <= 0) return false;

	// 백업해둔 공용판을 뒤져서 해당 ID가 있는지 확인
	for (int i = 0; i < 14; ++i)
	{
		for (int j = 0; j < 28; ++j)
		{
			if (m_old_public_tile[i][j].tileId == tileId)
			{
				// 원래부터 있던 타일임 (회수 불가)
				return true;
			}
		}
	}
	// 이번 턴에 내가 올린 타일임 (회수 가능)
	return false;

    
}

void CServerDlg::Backup() { // 매 턴 시작시마다 백업 예정
	if (m_bCurrentTurn) { // 자기 차례면 개인판도 백업
		for (int i = 1; i <= 3; i++)
			for (int j = 1; j <= 17; j++)
				m_old_private_tile[i][j] = m_private_tile[i][j];
	}
		for (int i = 1; i <= 13; i++) // 공용판은 항상 백업
			for (int j = 1; j <= 27; j++)
				m_old_public_tile[i][j] = m_public_tile[i][j];
	
}

void CServerDlg::OnClickedButtonSetback()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	if (m_bCurrentTurn) {
		CString strMsg;
		strMsg.Format(_T("type:Setback|sender:시스템"));
		BroadcastMessage(strMsg, 0); // 전체 공용판 setback
		Invalidate(FALSE);
	}
}
