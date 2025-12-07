
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
#include <vector>
#include <map>


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

	m_bGameOver = FALSE;
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
	//  DDX_Control(pDX, IDC_LIST_PLAYER, m_listPlayer);
	DDX_Control(pDX, IDC_LIST_PLAYER, m_listPlayer);
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

	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_PLAYER, &CServerDlg::OnNMCustomdrawListPlayer)

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
	m_bFirstSubmit = true;

	// [251127] 선택 상태 변수 초기화
	m_bIsSelected = false;
	m_nSelectedRow = -1;
	m_nSelectedCol = -1;
	m_bSelectedFromPublic = false;

	m_pTurn = (CServiceSocket*) - 1;

	m_listPlayer.ModifyStyle(0, LVS_REPORT | LVS_NOCOLUMNHEADER);
	m_listPlayer.InsertColumn(0, _T("이름"), LVCFMT_LEFT, 100);
	m_listPlayer.InsertColumn(1, _T("TileNum"), LVCFMT_CENTER, 80);
	m_listPlayer.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	//InitControls();

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
	static ULONGLONG mLastClickTime = 0;

	ULONGLONG currentTime = GetTickCount64(); // 현재 시스템 시간 (ms 단위)

	// 마지막 클릭으로부터 1초(1000ms)가 지나지 않았으면 함수 종료
	if (currentTime - mLastClickTime < 1000)
	{
		return; // 아무것도 안 하고 무시
	}

	// 시간 갱신

	mLastClickTime = currentTime;
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
	static ULONGLONG mLastClickTime = 0;

	ULONGLONG currentTime = GetTickCount64(); // 현재 시스템 시간 (ms 단위)

	// 마지막 클릭으로부터 1초(1000ms)가 지나지 않았으면 함수 종료
	if (currentTime - mLastClickTime < 1000)
	{
		return; // 아무것도 안 하고 무시
	}

	// 시간 갱신

	mLastClickTime = currentTime;
	if (m_bisGameStarted) {
		CString strTmpLog;
		strTmpLog.Format(_T("[INFO] 게임이 이미 진행 중입니다."));
		m_list_message.AddString(strTmpLog);
		m_list_message.SetTopIndex(m_list_message.GetCount() - 1);
		Invalidate(TRUE);
		return;
	}

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

		m_listPlayer.DeleteAllItems();
		
		// 2) 결정된 이름(m_strName)으로 리스트에 추가
		AddPlayerToList(m_strName, 0);

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


		if (!m_bisGameStarted) { // 아직 게임이 시작되지 않은 경우
			// IDC_LIST_PLAYER에서 strSender 이름을 가진 항목 제거
			int nCount = m_listPlayer.GetItemCount();
			for (int i = 0; i < nCount; i++)
			{	
				CServiceSocket* pItemSocket = (CServiceSocket*)m_listPlayer.GetItemData(i);

				CString strPlayerName = m_listPlayer.GetItemText(i, 0);
				if (pItemSocket == pServiceSocket)
				{
					m_listPlayer.DeleteItem(i);
					break;
				}
			}

			Invalidate(TRUE);
		}
		else { // 게임이 진행 중이던 경우
			// 게임 종료
			m_bisGameStarted = false;

			// 송신자를 제외한 모든 클라이언트에게 게임 종료 메시지 전송
			CServiceSocket* pSender = NULL;
			POSITION pos = m_clientSocketList.GetHeadPosition();
			/*while (pos != NULL) {
				CServiceSocket* pSocket = m_clientSocketList.GetNext(pos);
				if (pSocket && (pSocket->m_strName == pServiceSocket->m_strName)) {
					pSender = pSocket;
				}
			}*/

			CString requestMsg;
			requestMsg.Format(_T("type:EndGame|isNormalEnd:0"));
			BroadcastMessage(requestMsg, 0);

			for (int i = 0; i < 3; i++) {
				CString strTmpLog;
				strTmpLog.Format(_T("[INFO] 플레이어 탈주로 게임을 종료합니다. (%d초 이후 종료)"), (3 - i));
				m_list_message.AddString(strTmpLog);
				m_list_message.SetTopIndex(m_list_message.GetCount() - 1);
				//m_pServerDlg->Invalidate(TRUE);
				UpdateWindow();

				Sleep(1000);
			}

			// 다이얼로그 닫기
			PostMessage(WM_CLOSE);
			if (GetSafeHwnd())  // NULL이 아니면 윈도우가 아직 존재
			{
				PostMessage(WM_CLOSE);
			}
		}

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
	m_bGameOver = FALSE;

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
	Backup();
	UpdatePlayerTileCount(0, 14);

	CString strUpdateTilenum;
	strUpdateTilenum.Format(_T("type:UpdateTileNum|sender:시스템|name:%s|tilenum:%d|id:%llu"),
		m_strName, 14, (unsigned long long)0);
	// 나를 제외한 모두에게 전송
	BroadcastMessage(strUpdateTilenum, 0);

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
		UpdatePlayerTileCount(pSocket, 14);


		CString strUpdateTilenum;

		strUpdateTilenum.Format(_T("type:UpdateTileNum|sender:시스템|name:%s|tilenum:%d|id:%llu"),
			m_strName, 14, (unsigned long long)pSocket);

		// 나를 제외한 모두에게 전송
		BroadcastMessage(strUpdateTilenum, 0);


	}
	//개인 타일판을 시각화하는 함수


}

void CServerDlg::NextTurn() {
	CString strMsg;
	CString strNext;
	CString strBackup;
	CString strName;
	if (m_bCurrentTurn == TRUE) { // 서버 -> 플레이어 1로 넘길 때
		m_bCurrentTurn = FALSE;
		m_posTurn = m_clientSocketList.GetHeadPosition();
		m_pTurn = m_clientSocketList.GetNext(m_posTurn);

		strName = m_pTurn->m_strName;
			
		strNext.Format(_T("type:StartTurn|sender:시스템"));
		ResponseMessage(strNext, m_pTurn); // 턴 넘긴 후
		
	}
	else {
		if (m_posTurn == NULL) { // 서버로 턴이 넘어올 때
			m_bCurrentTurn = TRUE;
			strName = m_strName;
			//서버는 보낼필요 없음
			m_pTurn = 0;
			m_nSubmitTileNum = 0;
		}
		else { // 클라 -> 클라
			m_pTurn = m_clientSocketList.GetNext(m_posTurn);
			strName = m_pTurn->m_strName;

			strNext.Format(_T("type:StartTurn|sender:시스템"));
			ResponseMessage(strNext, m_pTurn);
		}
	}

	m_listPlayer.RedrawItems(0, m_listPlayer.GetItemCount() - 1);
	m_listPlayer.UpdateWindow();

	//현재 턴에 대한 정보를 모두에게 전달 (이름, 소켓 주소)
	strMsg.Format(_T("type:UpdatePlayer|sender:시스템|name:%s|id:%llu"),
		strName, (unsigned long long)m_pTurn);
	BroadcastMessage(strMsg, 0); //현재 턴에 대한 정보는 모두에게 공유되어야함
	//-----------
	CString strLog;
	strLog.Format(_T("%s의 턴이 시작되었습니다"), strName);
	DisplayMessage(_T("시스템"), strLog, 1);

	strBackup.Format(_T("type:Backup|sender:시스템")); // 모두에게 Backup 메시지 발신 -> 현재 턴인 사람의 개인판과 공용판만 백업됨
	BroadcastMessage(strBackup, 0);
	Backup();

}
void CServerDlg::Receive() {
	if (m_deck_pos >= 106) {
		CString strMsg;
		strMsg.Format(_T("type:CHAT|sender:시스템|content:현재 남은 타일이 없습니다."));

		DisplayMessage(_T("시스템"), _T("현재 남은 타일이 없습니다."), 1);

		return;
	}
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
	static ULONGLONG mLastClickTime = 0;

	ULONGLONG currentTime = GetTickCount64(); // 현재 시스템 시간 (ms 단위)

	// 마지막 클릭으로부터 1초(1000ms)가 지나지 않았으면 함수 종료
	if (currentTime - mLastClickTime < 1000)
	{
		return; // 아무것도 안 하고 무시
	}

	// 시간 갱신

	mLastClickTime = currentTime;
	if (!m_bisGameStarted) {
		CString strTmpLog;
		strTmpLog.Format(_T("[INFO] 게임이 시작되지 않았습니다."));
		m_list_message.AddString(strTmpLog);
		m_list_message.SetTopIndex(m_list_message.GetCount() - 1);
		Invalidate(TRUE);
		return;
	}
	
	if (m_bCurrentTurn == true && m_intPrivateTileNum!=51) {
			CString strMsg;
			strMsg.Format(_T("type:Setback|sender:시스템"));
			Setback();
			BroadcastMessage(strMsg, 0); // 전체 공용판 setback
			Receive(); // 패 한장 받기
			// 턴 종료
			UpdateSelfTileNum();

			NextTurn();
			Invalidate(TRUE);
			return;
	}
}


bool CServerDlg::IsPublicTileValid()
{
	/*
	공용판이 올바른지 공용판의 각 행 별로 검사하는 메소드.
	*/

	int sum = 0;

	for (int i = 1; i <= 13; i++) // 공용판의 각 행에 대해 반복
	{
		if (!IsRowValid(i, &sum))
		{
			return false;
		}
	}

	// 첫 번째 제출 시, 총합이 30 미만인지 검사
	if (m_bFirstSubmit && (sum < 30))
	{
		CString strTmpLog;
		strTmpLog.Format(_T("[INFO] 첫 번째 제출 시, 타일의 총합이 30 이상이어야 합니다."));
		m_list_message.AddString(strTmpLog);
		m_list_message.SetTopIndex(m_list_message.GetCount() - 1);
		Invalidate(TRUE);

		return false;
	}

	return true;
}

bool CServerDlg::IsRowValid(int row, int *sum)
{
	/*
	특정 행의 타일 조합들이 모두 유효한지 검사

	런과 그룹은 다음과 같이 정의함
	런(Run): 같은 색의 연속된 숫자 (예: 빨강 3-4-5)
	그룹(Group): 같은 숫자의 다른 색 (예: 빨강7, 파랑7, 검정7)

	청크(chunk)는 행에서 인접한 타일들의 묶음으로, 조건 검사의 단위임
	*/
	std::list<Tile> tileChunk; // 현재 검사 중인 타일 그룹
	bool isAllNew = false;

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

			// 첫 번째 제출 시, 기존 타일과 새 타일이 섞여있는지 검사
			if (m_bFirstSubmit && IsChunkMixed(tileChunk, &isAllNew))
			{
				CString strTmpLog;
				strTmpLog.Format(_T("[INFO] 첫 번째 제출 시, 기존 타일과 새 타일이 섞여 있을 수 없습니다."));
				m_list_message.AddString(strTmpLog);
				m_list_message.SetTopIndex(m_list_message.GetCount() - 1);
				Invalidate(TRUE);

				return false;
			}

			// chunk 검사
			if (IsRunValid(tileChunk)) // run인 경우
			{
				if (m_bFirstSubmit && isAllNew)
				{
					*sum += CalculateChunkValue(tileChunk, true);
				}

				tileChunk.clear();
				continue;
			}

			if (IsGroupValid(tileChunk)) // group인 경우
			{
				if (m_bFirstSubmit && isAllNew)
				{
					*sum += CalculateChunkValue(tileChunk, false);
				}

				tileChunk.clear();
				continue;
			}

			return false;
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
	bool isJokerFirst = false;

	for (const Tile& t : tileChunk)
	{
		if (t.isJoker)
		{
			if (expectedNum >= 14)
			{
				return false; // 조커가 13 오른쪽에 올 수 없음
			}

			if (expectedNum != -1) // 조커가 첫 번째 이후 타일인 경우
			{
				// 다음 숫자로 넘어감
				expectedNum++;
			}
			else // 조커가 첫 번째 타일인 경우
			{
				isJokerFirst = true;
			}
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

				if ((expectedNum == 1) && isJokerFirst)
				{
					return false; // 조커가 1 왼쪽에 올 수 없음
				}

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

// 첫 번째 제출 시 chunk에 기존 타일과 새 타일이 섞여있는지 검사
bool CServerDlg::IsChunkMixed(const std::list<Tile>& tileChunk, bool *isAllNew)
{
	bool hasOldTile = false;
	bool hasNewTile = false;

	for (const Tile& t : tileChunk)
	{
		if (IsExistingPublicTile(t.tileId))
		{
			hasOldTile = true;
		}
		else
		{
			hasNewTile = true;
		}

		// 둘 다 있으면 섞여 있는 것
		if (hasOldTile && hasNewTile)
		{
			return true;
		}
	}

	// 섞여 있지 않은 경우에, 전부 새로운 타일인지 여부 설정
	if (hasNewTile)
	{
		*isAllNew = true;
	}
	else
	{
		*isAllNew = false;
	}

	return false;
}

// Run 또는 Group의 타일 값 합계 계산 (조커 포함)
int CServerDlg::CalculateChunkValue(const std::list<Tile>& tileChunk, bool isRun)
{
	int totalValue = 0;

	if (isRun)
	{
		// Run의 경우: 같은 색의 연속된 숫자
		int startNum = -1;
		bool firstFound = false;

		for (const Tile& t : tileChunk)
		{
			if (!t.isJoker && !firstFound)
			{
				startNum = t.num;
				firstFound = true;
				break;
			}
		}

		// 각 위치의 숫자 결정하여 합산
		int expectedNum = startNum - 1;
		for (const Tile& t : tileChunk)
		{
			if (t.isJoker)
			{
				// 조커는 expectedNum 값을 가짐
				totalValue += expectedNum;
				expectedNum = expectedNum + 1;
			}
			else
			{
				totalValue += t.num;
				expectedNum = t.num + 1;
			}
		}
	}
	else
	{
		// Group의 경우: 같은 숫자의 다른 색
		// 일반 타일의 숫자 찾기
		int groupNum = -1;
		for (const Tile& t : tileChunk)
		{
			if (!t.isJoker)
			{
				groupNum = t.num;
				break;
			}
		}

		// 모든 타일이 같은 숫자 값을 가짐
		for (const Tile& t : tileChunk)
		{
			if (t.isJoker)
			{
				totalValue += groupNum;
			}
			else
			{
				totalValue += t.num;
			}
		}
	}

	return totalValue;
}

void CServerDlg::OnBnClickedButtonPass()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	static ULONGLONG mLastClickTime = 0;

	ULONGLONG currentTime = GetTickCount64(); // 현재 시스템 시간 (ms 단위)

	// 마지막 클릭으로부터 1초(1000ms)가 지나지 않았으면 함수 종료
	if (currentTime - mLastClickTime < 1000)
	{
		return; // 아무것도 안 하고 무시
	}

	// 시간 갱신

	mLastClickTime = currentTime;

	if (!m_bisGameStarted) {
		CString strTmpLog;
		strTmpLog.Format(_T("[INFO] 게임이 시작되지 않았습니다."));
		m_list_message.AddString(strTmpLog);
		m_list_message.SetTopIndex(m_list_message.GetCount() - 1);
		Invalidate(TRUE); 
		return;
	}

	if (!m_bCurrentTurn) {
		CString strTmpLog;
		strTmpLog.Format(_T("[INFO] 턴이 돌아오지 않았습니다."));
		m_list_message.AddString(strTmpLog);
		m_list_message.SetTopIndex(m_list_message.GetCount() - 1);
		Invalidate(TRUE);

		return;
	}

	if (m_nSubmitTileNum == 0) {
		CString strTmpLog;
		strTmpLog.Format(_T("[INFO] 제출한 타일이 없습니다."));
		m_list_message.AddString(strTmpLog);
		m_list_message.SetTopIndex(m_list_message.GetCount() - 1);
		Invalidate(TRUE);
		
		return;
	}

	if (IsPublicTileValid()) // 공용판이 올바른 경우
	{	
		if (m_bFirstSubmit)
		{
			m_bFirstSubmit = false; // 첫 제출 완료
		}

		UpdateSelfTileNum();
		// 턴 종료
		NextTurn();
	}
	else // 공용판이 올바르지 않은 경우
	{
		CString strTmpLog;
		strTmpLog.Format(_T("[INFO] 공용판이 올바르지 않습니다."));
		m_list_message.AddString(strTmpLog);
		m_list_message.SetTopIndex(m_list_message.GetCount() - 1);
		Invalidate(TRUE);
	}
	Invalidate(TRUE);
}



void CServerDlg::OnBnClickedButtonPlay()
{
	static ULONGLONG mLastClickTime = 0;

	ULONGLONG currentTime = GetTickCount64(); // 현재 시스템 시간 (ms 단위)

	// 마지막 클릭으로부터 1초(1000ms)가 지나지 않았으면 함수 종료
	if (currentTime - mLastClickTime < 1000)
	{
		return; // 아무것도 안 하고 무시
	}

	// 시간 갱신

	mLastClickTime = currentTime;
	if (m_clientSocketList.GetCount() < 1) {
		CString strTmpLog;
		strTmpLog.Format(_T("[INFO] 다른 플레이어를 기다려야합니다."));
		m_list_message.AddString(strTmpLog);
		m_list_message.SetTopIndex(m_list_message.GetCount() - 1);
		Invalidate(TRUE);
	}
	else {
		if (!m_bisGameStarted) {
			PlayGame();
			m_bisGameStarted = TRUE;

			CString requestMsg;
			requestMsg.Format(_T("type:GameStarted"));
			BroadcastMessage(requestMsg, 0);

			CString strTmpLog;
			strTmpLog.Format(_T("[INFO] 게임이 시작되었습니다."));
			m_list_message.AddString(strTmpLog);
			m_list_message.SetTopIndex(m_list_message.GetCount() - 1);

			Invalidate(TRUE);
		}
		else {
			CString strTmpLog;
			strTmpLog.Format(_T("[INFO] 게임이 이미 진행 중입니다."));
			m_list_message.AddString(strTmpLog);
			m_list_message.SetTopIndex(m_list_message.GetCount() - 1);

			Invalidate(TRUE);
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

	if (m_clientSocketList.GetCount() > 0) // 게임 진행 중에 서버가 탈주한 경우
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
			CString strTmpLog;
			strTmpLog.Format(_T("[INFO] 내 턴이 아닐 때는 타일을 제출할 수 없습니다."));
			m_list_message.AddString(strTmpLog);
			m_list_message.SetTopIndex(m_list_message.GetCount() - 1);

			m_bIsSelected = false; Invalidate(TRUE); return;
		}

		// =========================================================================
				// [규칙 2 수정] 기존 타일이 개인판으로 들어오는 것을 "양방향"으로 차단
				// =========================================================================

				// Case A: 공용판(선택) -> 개인판(클릭)으로 이동 시
		if (m_bSelectedFromPublic && bClickedPrivate)
		{
			Tile& selectedTile = m_public_tile[m_nSelectedRow][m_nSelectedCol];
			// 선택한 타일이 기존 타일이라면 회수 금지
			if (IsExistingPublicTile(selectedTile.tileId))
			{
				CString strTmpLog;
				strTmpLog.Format(_T("[INFO] 기존에 있던 타일은 가져올 수 없습니다.\n(이번 턴에 낸 타일만 회수 가능)"));
				m_list_message.AddString(strTmpLog);
				m_list_message.SetTopIndex(m_list_message.GetCount() - 1);

				m_bIsSelected = false; Invalidate(TRUE); return;
			}
		}
		// Case B: 개인판(선택) -> 공용판(클릭)으로 스왑 시 (추가된 부분)
		else if (!m_bSelectedFromPublic && bClickedPublic)
		{
			Tile& targetTile = m_public_tile[nRow][nCol]; // 공용판에 있는 타일(Target)

			// 공용판의 타겟 위치가 빈칸이 아니고(실제 타일이 있고), 그 타일이 기존 타일이라면
			// 스왑 시 기존 타일이 내 개인판으로 들어오게 되므로 막아야 함.
			if (!(targetTile.color == BLACK && targetTile.num == 0 && !targetTile.isJoker) &&
				IsExistingPublicTile(targetTile.tileId))
			{
				CString strTmpLog;
				strTmpLog.Format(_T("[INFO] 기존 타일과는 맞교환할 수 없습니다.\n(기존 타일이 개인판으로 들어올 수 없음)"));
				m_list_message.AddString(strTmpLog);
				m_list_message.SetTopIndex(m_list_message.GetCount() - 1);

				m_bIsSelected = false; Invalidate(TRUE); return;
			}
		}
		// =========================================================================

		// --- 데이터 교환 (Swap) ---
		Tile* pSourceTile = m_bSelectedFromPublic
			? &m_public_tile[m_nSelectedRow][m_nSelectedCol]
			: &m_private_tile[m_nSelectedRow][m_nSelectedCol];

		Tile* pTargetTile = bClickedPublic
			? &m_public_tile[nRow][nCol]
			: &m_private_tile[nRow][nCol];

		// 타일 이동 방향에 따른 m_nSubmitTileNum 증감
		// 개인판 -> 공용판 이동 (타일 제출)
		if (!m_bSelectedFromPublic && bClickedPublic)
		{
			// 빈 칸이 아닌 실제 타일을 제출하는 경우만 카운트
			if (!(pSourceTile->color == BLACK && pSourceTile->num == 0 && !pSourceTile->isJoker))
			{
				m_nSubmitTileNum++;
			}
		}
		// 공용판 -> 개인판 이동 (타일 회수)
		else if (m_bSelectedFromPublic && !bClickedPublic)
		{
			// 빈 칸이 아닌 실제 타일을 회수하는 경우만 카운트
			if (!(pSourceTile->color == BLACK && pSourceTile->num == 0 && !pSourceTile->isJoker))
			{
				m_nSubmitTileNum--;
			}
		}

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
	if (!m_bisGameStarted) {
		CString strTmpLog;
		strTmpLog.Format(_T("[INFO] 게임이 시작되지 않았습니다."));
		m_list_message.AddString(strTmpLog);
		m_list_message.SetTopIndex(m_list_message.GetCount() - 1);
		Invalidate(TRUE);
		return;
	}

	if (m_bCurrentTurn) {
		CString strMsg;
		strMsg.Format(_T("type:Setback|sender:시스템"));
		Setback();
		BroadcastMessage(strMsg, 0); // 전체 공용판 setback
		Invalidate(TRUE);
	}
	else {
		CString strTmpLog;
		strTmpLog.Format(_T("[INFO] 턴이 돌아오지 않았습니다."));
		m_list_message.AddString(strTmpLog);
		m_list_message.SetTopIndex(m_list_message.GetCount() - 1);
		Invalidate(TRUE);

		return;
	}
}
void CServerDlg::Setback() {
	if (m_bCurrentTurn) {
		for (int i = 1; i <= 3; i++)
			for (int j = 1; j <= 17; j++)
				m_private_tile[i][j] = m_old_private_tile[i][j];
	}
	for (int i = 1; i <= 13; i++)
		for (int j = 1; j <= 27; j++)
			m_public_tile[i][j] = m_old_public_tile[i][j];
	Invalidate(TRUE);

	m_nSubmitTileNum = 0;
}

void CServerDlg::UpdatePlayerTileCount(CServiceSocket* pSocket, int nTileNum)
{
	int nCount = m_listPlayer.GetItemCount();

	for (int i = 0; i < nCount; i++)
	{
		// 각 행에 숨겨진 데이터(소켓 포인터)를 가져옴
		CServiceSocket* pItemSocket = (CServiceSocket*)m_listPlayer.GetItemData(i);

		// 찾으려는 소켓과 리스트에 저장된 소켓이 같은지 비교
		// (서버 본인의 경우 둘 다 nullptr이므로 여기서 걸러짐)
		if (pItemSocket == pSocket)
		{
			CString strNum;
			strNum.Format(_T("%d"), nTileNum);

			// 1번 컬럼(타일 수) 업데이트
			m_listPlayer.SetItemText(i, 1, strNum);

			if (m_bisGameStarted && !m_bGameOver && nTileNum == 0)
			{
				HandleGameOver(pSocket);  // pSocket == nullptr 이면 서버가 승자
			}

			return; // 찾았으므로 종료

		}
	}
}

void CServerDlg::AddPlayerToList(CString strName, int nTileCount, CServiceSocket* pSocket)
{


	int nIndex = m_listPlayer.GetItemCount();

	// 0번 컬럼: 이름
	m_listPlayer.InsertItem(nIndex, strName);

	// 1번 컬럼: 타일 수
	CString strCount;
	strCount.Format(_T("%d"), nTileCount);
	m_listPlayer.SetItemText(nIndex, 1, strCount);

	// [핵심] 리스트 아이템의 데이터 공간에 소켓 포인터 주소를 저장
	// 서버 본인인 경우 pSocket이 nullptr로 들어옴
	m_listPlayer.SetItemData(nIndex, (DWORD_PTR)pSocket);
}
	

void CServerDlg::UpdateSelfTileNum() {
	int nCount = 0;
	int sum = 0;
	int joker = 0;

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 18; j++)
		{
			const Tile& t = m_private_tile[i][j];
			if (t.tileId == -1) continue;

			nCount++;
			if (t.isJoker) joker++;
			else           sum += t.num;
		}
	}

	m_intPrivateTileNum = nCount;

	SetPlayerScoreStat(nullptr, sum, joker);

	CString strUpdateTilenum;
	strUpdateTilenum.Format(
		_T("type:UpdateTileNum|sender:시스템|name:%s|tilenum:%d|id:%llu"),
		m_strName, m_intPrivateTileNum, 0
	);

	BroadcastMessage(strUpdateTilenum, 0);
	UpdatePlayerTileCount(nullptr, m_intPrivateTileNum);  // pSocket == 0
}


// [GAMEOVER] 클라이언트 쪽에서 보내준 점수 정보 저장
void CServerDlg::SetPlayerScoreStat(CServiceSocket* pSocket, int sum, int joker)
{
	if (!pSocket) return; // 서버 본인은 여기서 안 다룸 (서버는 자신의 보드를 직접 스캔)

	PlayerScoreStat& stat = m_playerScore[pSocket];
	stat.sumNumbers = sum;
	stat.jokerCount = joker;
}

// [GAMEOVER] 플레이어별 남은 숫자 합 / 조커 개수 얻기
void CServerDlg::GetPlayerTileStat(CServiceSocket* pSocket, int& outSum, int& outJoker)
{
	outSum = 0;
	outJoker = 0;

	// 서버(자기 자신)인 경우: m_private_tile 그대로 스캔
	if (pSocket == nullptr)
	{
		for (int row = 1; row <= 3; ++row)
		{
			for (int col = 1; col <= 17; ++col)
			{
				const Tile& t = m_private_tile[row][col];

				// 빈 칸이면 스킵
				if (t.color == BLACK && t.num == 0 && !t.isJoker)
					continue;

				if (t.isJoker)
					++outJoker;
				else
					outSum += t.num;
			}
		}
	}
	else
	{
		// 클라이언트는 m_playerScore에서 가져옴
		auto it = m_playerScore.find(pSocket);
		if (it != m_playerScore.end())
		{
			outSum = it->second.sumNumbers;
			outJoker = it->second.jokerCount;
		}
		// 못 찾으면 0, 0
	}
}
void CServerDlg::HandleGameOver(CServiceSocket* pWinnerSocket)
{
	if (m_bGameOver) return; // 중복 호출 방지

	m_bGameOver = TRUE;
	m_bisGameStarted = FALSE; // 정상 종료

	using std::vector;

	vector<PlayerResult> players;
	int nItems = m_listPlayer.GetItemCount();
	int winnerIndex = -1;

	// 1) ListCtrl에서 플레이어 정보 수집
	for (int i = 0; i < nItems; ++i)
	{
		PlayerResult pr{};
		pr.name = m_listPlayer.GetItemText(i, 0);
		CString strCnt = m_listPlayer.GetItemText(i, 1);
		pr.tileCount = _ttoi(strCnt);
		pr.pSocket = (CServiceSocket*)m_listPlayer.GetItemData(i);
		pr.isWinner = (pr.pSocket == pWinnerSocket);

		// 숫자 합/조커 개수 채우기
		GetPlayerTileStat(pr.pSocket, pr.sumNumbers, pr.jokerCount);
		pr.finalScore = 0;

		if (pr.isWinner) winnerIndex = i;

		players.push_back(pr);
	}

	// 혹시 못 찾았으면 tileCount==0인 사람을 승자로 간주
	if (winnerIndex == -1)
	{
		for (int i = 0; i < (int)players.size(); ++i)
		{
			if (players[i].tileCount == 0)
			{
				players[i].isWinner = true;
				winnerIndex = i;
				break;
			}
		}
	}

	if (winnerIndex == -1)
	{
		AfxMessageBox(_T("게임 종료는 감지했지만 승자를 찾지 못했습니다."),
			MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	// 2) 패자 점수(마이너스) 및 벌점 합산
	int totalLoserPenalty = 0;
	for (int i = 0; i < (int)players.size(); ++i)
	{
		if (i == winnerIndex) continue;

		int penalty = players[i].sumNumbers + players[i].jokerCount * 30;
		players[i].finalScore = -penalty;
		totalLoserPenalty += penalty;
	}

	// 3) 승자 점수(플러스) = 모든 패자의 벌점 총합
	players[winnerIndex].finalScore = totalLoserPenalty;

	// 4) 점수 순으로 정렬
	vector<PlayerResult> sorted = players;
	std::sort(sorted.begin(), sorted.end(),
		[](const PlayerResult& a, const PlayerResult& b)
		{
			if (a.finalScore != b.finalScore)
				return a.finalScore > b.finalScore; // 높은 점수 우선
			return a.name < b.name;                // 점수 같으면 이름순
		});

	// 5) 결과 문자열 구성
	CString result;
	result += _T("게임이 종료되었습니다.\r\n\r\n");

	CString winLine;
	winLine.Format(_T("승리자: %s (점수: %d점)\r\n\r\n"),
		sorted[0].name.GetString(), sorted[0].finalScore);
	result += winLine;

	result += _T("[최종 순위]\r\n");
	for (int i = 0; i < (int)sorted.size(); ++i)
	{
		const PlayerResult& p = sorted[i];
		CString line;
		line.Format(
			_T("%d위: %s  (점수: %d, 남은 숫자합: %d, 조커: %d)\r\n"),
			i + 1,
			p.name.GetString(),
			p.finalScore,
			p.sumNumbers,
			p.jokerCount
		);
		result += line;
	}


	// 7) 클라이언트들에게도 결과 브로드캐스트
	//    result 문자열에는 '|'만 안 쓰면 됨. (\r\n은 그대로 전송해도 무방)
	// 메시지 순서변경 -> client에 알림창을 띄우고 자기 꺼 띄어야 해결됨
	CString broadcast;
	broadcast.Format(_T("type:EndGame|isNormalEnd:1|result:%s"),
		result.GetString());

	BroadcastMessage(broadcast, 0);

	// 6) 서버에서 메시지 박스로 출력
	AfxMessageBox(result, MB_OK | MB_ICONINFORMATION);

}

void CServerDlg::OnNMCustomdrawListPlayer(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

    *pResult = CDRF_DODEFAULT;

    // 1. 그리기 주기 시작 (PrePaint)
    if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
    {
        *pResult = CDRF_NOTIFYITEMDRAW; // 각 아이템(행)을 그릴 때 알림 요청
    }
    // 2. 각 아이템 그리기 전 (ItemPrePaint)
    else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
    {
        // 현재 그리는 행의 ItemData(= CServiceSocket* 또는 0)를 가져옴
        int nItem = (int)pLVCD->nmcd.dwItemSpec;
        CServiceSocket* pRowSocket = (CServiceSocket*)m_listPlayer.GetItemData(nItem);
        // 현재 턴인 소켓과 일치하면 색상 변경
        // (서버의 턴일 경우 둘 다 nullptr이므로 서버 행이 강조됨)
        if (pRowSocket == m_pTurn)
        {
            pLVCD->clrTextBk = RGB(255, 255, 200); // 연한 노란색 배경
            pLVCD->clrText   = RGB(255, 0, 0);     // 빨간색 글씨
        }
        else
        {
            // 기본 색상 (흰 배경, 검은 글씨)
            pLVCD->clrTextBk = RGB(255, 255, 255);
            pLVCD->clrText   = RGB(0, 0, 0);
        }

        *pResult = CDRF_NEWFONT; // 폰트/색상 변경 적용
    }
}

void CServerDlg::InitControls() {
	/// [PLAY / 제출] 버튼
		if (GetDlgItem(IDC_BUTTON_PLAY)) {
			GetDlgItem(IDC_BUTTON_PLAY)->MoveWindow(720, 555, 80, 40);
		}
	// [PASS / 턴넘김] 버튼
	if (GetDlgItem(IDC_BUTTON_PASS)) {
		GetDlgItem(IDC_BUTTON_PASS)->MoveWindow(810, 555, 80, 40);
	}
	// [RECEIVE / 타일받기] 버튼
	if (GetDlgItem(IDC_BUTTON_RECEIVE)) {
		GetDlgItem(IDC_BUTTON_RECEIVE)->MoveWindow(720, 605, 80, 40);
	}
	// [SETBACK / 되돌리기] 버튼
	if (GetDlgItem(IDC_BUTTON_SetBack)) {
		GetDlgItem(IDC_BUTTON_SetBack)->MoveWindow(810, 605, 80, 40);
	}

	// [플레이어 목록] (버튼들 옆에 배치)
	if (GetDlgItem(IDC_LIST_PLAYER)) {
		GetDlgItem(IDC_LIST_PLAYER)->MoveWindow(900, 555, 150, 90);
	}


	// -------------------------------------------------------------
	// 3. [오른쪽 상단] 공용판(width=980) 옆 빈 공간 (x > 1000)
	// -------------------------------------------------------------

	// [시작] 버튼 (가장 잘 보이게 위쪽)
	if (GetDlgItem(IDC_BUTTON_START)) {
		GetDlgItem(IDC_BUTTON_START)->MoveWindow(1000, 35, 250, 40);
	}

	// [채팅/메시지 목록]
	if (GetDlgItem(IDC_LIST_MESSAGE)) {
		GetDlgItem(IDC_LIST_MESSAGE)->MoveWindow(1000, 85, 250, 200);
	}

	// [시스템 로그 목록]
	if (GetDlgItem(IDC_LIST_LOG)) {
		GetDlgItem(IDC_LIST_LOG)->MoveWindow(1000, 295, 250, 150);
	}

	// [채팅 입력창]
	if (GetDlgItem(IDC_EDIT_SEND)) {
		GetDlgItem(IDC_EDIT_SEND)->MoveWindow(1000, 455, 180, 30);
	}
	// [보내기 버튼]
	if (GetDlgItem(IDC_BUTTON_SEND)) {
		GetDlgItem(IDC_BUTTON_SEND)->MoveWindow(1190, 455, 60, 30);
	}
}
BOOL CServerDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN) // ENTER키 눌릴 시
			return TRUE;
		else if (pMsg->wParam == VK_ESCAPE) // ESC키 눌릴 시
			return TRUE;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}
