#include "pch.h"
#include "framework.h"
#include "Client.h"
#include "ClientDlg.h"
#include "afxdialogex.h"
#include "afxsock.h"
#include "AddressDlg.h"
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
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CClientDlg 대화 상자



CClientDlg::CClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CLIENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_strName = _T("");
}

void CClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_SEND, m_edit_send);
	DDX_Control(pDX, IDC_LIST_MESSAGE, m_list_message);
	DDX_Control(pDX, IDD_STATIC_STATUS, m_static_status);
	DDX_Control(pDX, IDC_LIST_PLAYER, m_listPlayer);
}

BEGIN_MESSAGE_MAP(CClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CClientDlg::OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_SEND, &CClientDlg::OnBnClickedButtonSend)
	ON_BN_CLICKED(IDC_BUTTON_PASS, &CClientDlg::OnBnClickedButtonPass)
	ON_BN_CLICKED(IDC_BUTTON_RECEIVE, &CClientDlg::OnBnClickedButtonReceive)
	ON_WM_GETMINMAXINFO()
	ON_WM_DESTROY()
	// [251127] 마우스 왼쪽 클릭 메시지 연결
	ON_WM_LBUTTONDOWN()
	ON_BN_CLICKED(IDC_BUTTON_SETBACK, &CClientDlg::OnBnClickedButtonSetback)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_PLAYER, &CClientDlg::OnNMCustomdrawListPlayer)
END_MESSAGE_MAP()


// CClientDlg 메시지 처리기

BOOL CClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	InitTiles();
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

	m_strName = _T("익명");
	m_bCurrentTurn = false;
	LoadImage();

	m_bisGameStarted = FALSE;

	m_intPrivateTileNum = 0;
	m_bFirstSubmit = true;

	m_pTurn = (DWORD_PTR)-1;

	m_listPlayer.ModifyStyle(0, LVS_REPORT | LVS_NOCOLUMNHEADER);
	m_listPlayer.InsertColumn(0, _T("이름"), LVCFMT_LEFT, 100);
	m_listPlayer.InsertColumn(1, _T("TileNum"), LVCFMT_CENTER, 80);
	m_listPlayer.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	InitControls();
	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}


void CClientDlg::InitTiles() {
	int k = 0;

	// 4색 × (1~13 각 2장) = 104장
	for (int c = RED; c <= BLACK; ++c) {
		for (int n = 1; n <= 13; ++n) {
			m_tile_list[k++] = Tile{ static_cast<Color>(c), n, false };
			m_tile_list[k++] = Tile{ static_cast<Color>(c), n, false };
		}
	}
	// 조커 2장(관례: BLACK, num=0)
	m_tile_list[k++] = Tile{ BLACK, 0, true };
	m_tile_list[k++] = Tile{ BLACK, 0, true };

}
void CClientDlg::ClearBoards() {
	const Tile empty = MakeEmptyTile();

	// 공용판 (전체 0~끝까지 채우고, 실제 사용은 [1..13][1..27])
	for (int r = 0; r < 14; ++r)
		for (int c = 0; c < 28; ++c)
			m_public_tile[r][c] = empty;

	// 개인판 (전체 0~끝까지 채우고, 실제 사용은 [1..3][1..17])
	for (int r = 0; r < 4; ++r)
		for (int c = 0; c < 18; ++c)
			m_private_tile[r][c] = empty;
}
void CClientDlg::CopyBoards() {
	int i = 0, j = 0;
	for (i = 0; i < 14; i++)
		for (j = 0; j < 28; j++)
			m_old_public_tile[i][j] = m_public_tile[i][j];

	for (i = 0; i < 4; i++)
		for (j = 0; j < 18; j++)
			m_old_private_tile[i][j] = m_private_tile[i][j];
	
	for (i = 0; i < 106; i++)
		m_rand_tile_list_cpy[i] = m_rand_tile_list[i];
}
void CClientDlg::CopyBoardsReverse() {
	int i = 0, j = 0;
	for (i = 0; i < 14; i++)
		for (j = 0; j < 28; j++)
			m_public_tile[i][j] = m_old_public_tile[i][j];

	for (i = 0; i < 4; i++)
		for (j = 0; j < 18; j++)
			m_private_tile[i][j] = m_old_private_tile[i][j];

	for (i = 0; i < 106; i++)
		m_rand_tile_list[i] = m_rand_tile_list_cpy[i];
}
	

void CClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CClientDlg::OnPaint()
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
			// 빨간색 펜 생성 (두께 3)
			CPen redPen(PS_SOLID, 3, RGB(255, 0, 0));
			CPen* pOldPen = dc.SelectObject(&redPen);

			// 속은 비우기 (투명 브러시)
			CBrush* pOldBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);

			int startX = 0, startY = 0;

			// 좌표 계산 (DrawMyTiles의 좌표 로직과 동일해야 함)
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

			// 테두리 그리기 (셀 크기 35x35)
			dc.Rectangle(startX, startY, startX + 35, startY + 35);

			// 리소스 복구
			dc.SelectObject(pOldPen);
			dc.SelectObject(pOldBrush);
		}

		CDialogEx::OnPaint();


	}
	
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


//  ListBox에 메시지를 출력하는 함수 구현
void CClientDlg::DisplayMessage(const CString& strSender, const CString& strMsg, BOOL bReceived)
{
	CString strOutput;

	strOutput.Format(_T("[%s] % s"), strSender, strMsg);

	m_list_message.AddString(strOutput);
	m_list_message.SetTopIndex(m_list_message.GetCount() - 1);
}

void CClientDlg::OnBnClickedButtonConnect()
{
	if (m_bisGameStarted) {
		CString strTmpLog;
		strTmpLog.Format(_T("[INFO] 이미 게임이 진행 중입니다."));
		m_list_message.AddString(strTmpLog);
		m_list_message.SetTopIndex(m_list_message.GetCount() - 1);
		Invalidate(TRUE);
	
		return;
	}

	CAddressDlg pAddressDlg;


	INT_PTR nResponse = pAddressDlg.DoModal();

	if (nResponse == IDOK) {
		CString strServerIP = pAddressDlg.m_strIPAddress;

		if (pAddressDlg.m_strName.IsEmpty()) m_strName = _T("익명");
		else m_strName = pAddressDlg.m_strName;

		// 1. 기존 소켓 정리
		if (m_pClientSocket)
		{
			m_pClientSocket->Close();
			delete m_pClientSocket;
			m_pClientSocket = nullptr;
		}

		// 2. CClientSocket 객체 생성 및 대화 상자 포인터 전달
		m_pClientSocket = new CClientSocket(this);

		// 3. 소켓 생성
		if (!m_pClientSocket->Create()) {
			m_static_status.SetWindowText(_T("소켓 생성 실패!"));
			delete m_pClientSocket;
			m_pClientSocket = nullptr;
			return;
		}

		// 4. 비동기 연결 시도

		if (!m_pClientSocket->Connect(strServerIP, 12345)) //  서버 주소와 포트
		{
			DWORD dwError = m_pClientSocket->GetLastError();

			// WSAEWOULDBLOCK(10035): 비동기 연결이 진행 중임을 나타냄 (정상)
			if (dwError != WSAEWOULDBLOCK)
			{

				CString strStatus;
				strStatus.Format(_T("연결 실패! (에러코드: %d)"), dwError);
				m_static_status.SetWindowText(strStatus);
				delete m_pClientSocket;
				m_pClientSocket = nullptr;
			}
			else {
				m_static_status.SetWindowText(_T("서버 연결 시도 중..."));
			}
		}
		else 
		{
			m_static_status.SetWindowText(_T("서버 연결 성공!"));
		}
	}
}

void CClientDlg::OnBnClickedButtonSend()
{
	// 1. 입력창에서 메시지 가져오기
	CString strSend;
	m_edit_send.GetWindowText(strSend);

	if (strSend.IsEmpty()) return;
	
	CString strMsg;
	strMsg.Format(_T("type:CHAT|sender:%s|content:%s"), m_strName, strSend);
	RequestMessage(strMsg);

	DisplayMessage(m_strName, strSend, FALSE);
	// 소켓 통신 오류 해결을 하려면 request message에서 response받아서 해결해야할듯

	m_edit_send.SetWindowText(_T(""));

	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CClientDlg::RequestMessage(CString& strMsg) {
	if (m_pClientSocket == nullptr || !m_pClientSocket->IsConnected())
	{
		m_static_status.SetWindowText(_T("연결되지 않았습니다."));
		return;
	}
	std::string utf8_data = CStringToUTF8(strMsg);
	//메시지 헤더 길이
	int nLength = (int)utf8_data.length();
	// 길이 헤더 전송
	int nHeaderBytesSent = m_pClientSocket->Send(&nLength, sizeof(nLength));
	if (nHeaderBytesSent != sizeof(nLength))
	{
		// 헤더 전송 실패 또는 일부만 전송됨
		// 실제 구현에서는 재시도 로직이 필요하지만, 여기서는 오류 처리
		CString strStatus;
		strStatus.Format(_T("ERROR: 길이 헤더 전송 실패. (Sent: %d)"), nHeaderBytesSent);
		m_static_status.SetWindowText(strStatus);
		return;
	}

	// 메시지 본문 전송
	int nDataBytesSent = m_pClientSocket->Send(utf8_data.c_str(), nLength);

	if (nDataBytesSent == nLength)
	{
		//정상작동
	}
	else if (nDataBytesSent == SOCKET_ERROR)
	{
		CString strStatus;
		strStatus.Format(_T("ERROR: 데이터 본문 전송 실패! (에러코드: %d)"), m_pClientSocket->GetLastError());
		m_static_status.SetWindowText(strStatus);
	}


}




Tile CClientDlg::ParseIdtoTile(int Tileid) {

	Color c = BLACK;
	Tile newTile = Tile{ BLACK, 0, false, 0 }; // 초기화

	if (Tileid >= 105) { // 조커 타일 (105, 106)
		newTile = Tile{ BLACK, 0, true , Tileid };
	}
	else { // 일반 타일 (1~104)
		int adjustedId = Tileid - 1;
		int color_group = adjustedId / 26;


		switch (color_group) {
		case 0: c = RED; break;
		case 1: c = GREEN; break;
		case 2: c = BLUE; break;
		case 3: c = BLACK; break;
	
		default: break;
		}

		newTile = Tile{ c, (adjustedId) % 13 + 1, false, Tileid};
	}
	return newTile;
}

bool CClientDlg::IsPublicTileValid()
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

bool CClientDlg::IsRowValid(int row, int* sum)
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

bool CClientDlg::IsRunValid(std::list<Tile> tileChunk)
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

bool CClientDlg::IsGroupValid(std::list<Tile> tileChunk)
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
bool CClientDlg::IsChunkMixed(const std::list<Tile>& tileChunk, bool* isAllNew)
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
int CClientDlg::CalculateChunkValue(const std::list<Tile>& tileChunk, bool isRun)
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


void CClientDlg::OnBnClickedButtonPass()
{
	if (!m_bisGameStarted) {
		CString strTmpLog;
		strTmpLog.Format(_T("[INFO] 아직 게임이 시작되지 않았습니다."));
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

	//유효성 검증코드
	if (IsPublicTileValid()) // 공용판이 올바른 경우
	{
		if (m_bFirstSubmit)
		{
			m_bFirstSubmit = false; // 첫 제출 완료
		}

		// 타일 개수 최신화 요청 전송
		UpdateSelfTileNum();
		// 턴 종료
		CString strMsg;
		strMsg.Format(_T("type:EndTurn|sender:%s"), m_strName);
		RequestMessage(strMsg);
		m_bCurrentTurn = false;
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


void CClientDlg::OnBnClickedButtonReceive()
{
	if (!m_bisGameStarted) {
		CString strTmpLog;
		strTmpLog.Format(_T("[INFO] 아직 게임이 시작되지 않았습니다."));
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

	if (m_bCurrentTurn && m_intPrivateTileNum!=51) {
		CString strSetback;
		strSetback.Format(_T("type:SetbackReq|sender:Client"));
		RequestMessage(strSetback);
		CString strMsg;
		strMsg.Format(_T("type:Receive|sender:%s"), m_strName);
		RequestMessage(strMsg);
		Invalidate(TRUE);
		return;
	}

	return;
}


void CClientDlg::LoadImage()
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

bool CClientDlg::LoadPngFromResource(CImage& img, UINT uResID)
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

int CClientDlg::GetTileImageIndex(const Tile& tile) const
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

void CClientDlg::DrawMyTiles(CDC& dc)
{
	// 공통 상수
	const int CELL_SIZE = 35;
	const int TILE_DRAW_SIZE = 32;
	const int OFFSET = (CELL_SIZE - TILE_DRAW_SIZE) / 2 + 1;

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
				srcX, srcY, srcW, srcH            // 원본에서 자를 영역
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
				srcX, srcY, srcW, srcH
			);
		}
	}
}

//[251127] 추가
void CClientDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// 1. 클릭된 위치가 공용판인지 개인판인지 확인
	bool bClickedPublic = false;
	bool bClickedPrivate = false;
	int nRow = -1, nCol = -1;

	// 공용판 범위 체크
	if (point.x >= 35 && point.x < 35 + 27 * 35 &&
		point.y >= 35 && point.y < 35 + 13 * 35)
	{
		bClickedPublic = true;
		nCol = (point.x - 35) / 35 + 1;
		nRow = (point.y - 35) / 35 + 1;
	}
	// 개인판 범위 체크
	else if (point.x >= 105 && point.x < 105 + 17 * 35 &&
		point.y >= 555 && point.y < 555 + 3 * 35)
	{
		bClickedPrivate = true;
		nCol = (point.x - 105) / 35 + 1;
		nRow = (point.y - 555) / 35 + 1;
	}
	else
	{
		// 판 바깥 클릭 -> 선택 해제
		if (m_bIsSelected) {
			m_bIsSelected = false;
			Invalidate(TRUE);
		}
		CDialogEx::OnLButtonDown(nFlags, point);
		return;
	}

	// 2. 타일 선택 또는 이동 로직
	if (!m_bIsSelected)
	{
		// [상태 1: 선택 시도]

		// ========================================================
		// [규칙 추가] 내 턴이 아닐 때는 공용판 타일을 선택(조작)할 수 없음
		// ========================================================
		if (!m_bCurrentTurn && bClickedPublic)
		{
			// (옵션) 너무 자주 뜨면 귀찮을 수 있으니 메시지 박스는 생략하거나 필요시 주석 해제
			// AfxMessageBox(_T("내 턴이 아닐 때는 공용판을 건드릴 수 없습니다."));
			return;
		}

		// 클릭한 곳의 타일 정보 가져오기
		Tile t;
		if (bClickedPublic) t = m_public_tile[nRow][nCol];
		else t = m_private_tile[nRow][nCol];

		// 빈 타일이 아니면 선택
		if (!(t.color == BLACK && t.num == 0 && !t.isJoker))
		{
			m_bIsSelected = true;
			m_bSelectedFromPublic = bClickedPublic;
			m_nSelectedRow = nRow;
			m_nSelectedCol = nCol;

			Invalidate(TRUE); // 선택 표시(빨간 테두리)
		}
	}
	else
	{
		// [상태 2: 이동 시도]

		// 같은 위치 다시 클릭 -> 선택 취소
		if (m_bSelectedFromPublic == bClickedPublic &&
			m_nSelectedRow == nRow && m_nSelectedCol == nCol)
		{
			m_bIsSelected = false;
			Invalidate(TRUE);
			return;
		}

		// ========================================================
		// [규칙 수정] 이동 제한 및 교환 로직 강화 (양방향 체크)
		// ========================================================

		// 1. 내 턴이 아닐 때: 제출 및 조작 불가
		if (!m_bCurrentTurn && bClickedPublic)
		{
			AfxMessageBox(_T("내 턴이 아닐 때는 타일을 제출할 수 없습니다."));
			m_bIsSelected = false; Invalidate(TRUE); return;
		}

		// 검사 대상 타일 찾기
		// (공용판에서 개인판으로 넘어오게 될 타일이 무엇인지 식별)
		Tile* pMovingToPrivate = nullptr;
		bool bIsSwapAttempt = false;

		// CASE A: 공용판 타일을 선택해서 -> 개인판을 클릭한 경우 (직접 회수 시도)
		if (m_bSelectedFromPublic && bClickedPrivate)
		{
			pMovingToPrivate = &m_public_tile[m_nSelectedRow][m_nSelectedCol];

			// 대상(개인판)이 비어있지 않으면 교환(Swap) 시도임
			Tile& target = m_private_tile[nRow][nCol];
			if (!(target.color == BLACK && target.num == 0 && !target.isJoker)) bIsSwapAttempt = true;
		}
		// CASE B: 개인판 타일을 선택해서 -> 공용판 타일을 클릭한 경우 (교환 회수 시도 - 버그 발생 구간)
		else if (!m_bSelectedFromPublic && bClickedPublic)
		{
			// 클릭된 공용판 위치에 타일이 있다면, 그 타일이 개인판으로 오게 됨
			Tile& target = m_public_tile[nRow][nCol];

			// 빈 칸이 아니면 교환 시도
			if (!(target.color == BLACK && target.num == 0 && !target.isJoker))
			{
				pMovingToPrivate = &target;
				bIsSwapAttempt = true;
			}
		}

		// 2. 공용판 -> 개인판으로 이동하려는 타일이 '기존 타일'인지 검사
		if (pMovingToPrivate != nullptr)
		{
			if (IsExistingPublicTile(pMovingToPrivate->tileId))
			{
				// 2-1. 조커인 경우
				if (pMovingToPrivate->isJoker)
				{
					// 조커는 반드시 '교환(Swap)' 방식으로만 가져올 수 있음
					// (빈 칸으로 그냥 가져오는 것은 불가능)
					if (!bIsSwapAttempt)
					{
						AfxMessageBox(_T("공용판의 조커는 타일과 '교환'하는 방식으로만 가져올 수 있습니다."));
						m_bIsSelected = false; Invalidate(TRUE); return;
					}
					// 교환 시도라면 허용 (단, 루미큐브 규칙상 맞는 패인지 검증해야 하지만 여기선 Swap만 허용)
				}
				// 2-2. 일반 타일인 경우 (절대 회수 불가)
				else
				{
					AfxMessageBox(_T("기존에 등록된 타일은 다시 가져올 수 없습니다.\n(이번 턴에 낸 타일만 회수 가능)"));
					m_bIsSelected = false; Invalidate(TRUE); return;
				}
			}
		}
		// ========================================================


		// --- 데이터 교환 (Swap) ---
		Tile* pSourceTile = nullptr;
		if (m_bSelectedFromPublic)
			pSourceTile = &m_public_tile[m_nSelectedRow][m_nSelectedCol];
		else
			pSourceTile = &m_private_tile[m_nSelectedRow][m_nSelectedCol];

		Tile* pTargetTile = nullptr;
		if (bClickedPublic)
			pTargetTile = &m_public_tile[nRow][nCol];
		else
			pTargetTile = &m_private_tile[nRow][nCol];

		// 타일 데이터 Swap
		Tile temp = *pTargetTile;
		*pTargetTile = *pSourceTile;
		*pSourceTile = temp;

		// --- 서버 동기화 전송 ---

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

		// 1. 원래 있던 위치(Source)가 공용판이었다면 업데이트 전송
		if (m_bSelectedFromPublic)
		{
			SendUpdatePublicTile(m_nSelectedRow, m_nSelectedCol);
		}

		// 2. 이동한 위치(Target)가 공용판이었다면 업데이트 전송
		if (bClickedPublic)
		{
			SendUpdatePublicTile(nRow, nCol);
		}

		// 선택 해제 및 화면 갱신
		m_bIsSelected = false;
		Invalidate(TRUE);
	}

	CDialogEx::OnLButtonDown(nFlags, point);
}

// ===========================================
// [251127]
// ID를 기반으로 Tile 객체를 반환 (서버와 동일 로직)
Tile CClientDlg::GetTileFromId(int tileId)
{
	if (tileId == -1) return MakeEmptyTile(); // ClientDlg.h에 정의된 함수 사용

	Tile t;
	t.tileId = tileId;
	t.isJoker = false;

	if (tileId >= 105) // 조커
	{
		t.color = BLACK;
		t.num = 0;
		t.isJoker = true;
	}
	else // 일반 타일
	{
		int adjustedId = tileId - 1;
		int colorIdx = adjustedId / 26;
		t.color = static_cast<Color>(colorIdx);
		t.num = (adjustedId % 13) + 1;
	}
	return t;
}

// 변경된 타일 정보를 서버로 전송
void CClientDlg::SendUpdatePublicTile(int row, int col)
{
	Tile t = m_public_tile[row][col];

	// 빈 타일이면 -1, 아니면 tileId
	int id = (t.color == BLACK && t.num == 0 && !t.isJoker) ? -1 : t.tileId;

	// 메시지 생성: type:PLACE|pos:row,col|tileid:id
	CString strMsg;
	strMsg.Format(_T("type:PLACE|pos:%d,%d|tileid:%d"), row, col, id);

	// 서버로 전송
	RequestMessage(strMsg);
}

// 서버에서 온 공용판 업데이트 메시지 처리
void CClientDlg::ProcessPublicBoardUpdate(CString strMsg)
{
	// 메시지 파싱 (서버 코드와 동일)
	int nPosStart = strMsg.Find(_T("pos:"));
	int nIdStart = strMsg.Find(_T("tileid:"));

	if (nPosStart == -1 || nIdStart == -1) return;

	// 1. 좌표 파싱
	CString strPos = strMsg.Mid(nPosStart + 4, nIdStart - (nPosStart + 4) - 1);
	int nComma = strPos.Find(_T(","));
	int nRow = _ttoi(strPos.Left(nComma));
	int nCol = _ttoi(strPos.Mid(nComma + 1));

	// 2. TileID 파싱
	CString strId = strMsg.Mid(nIdStart + 7);
	int nId = _ttoi(strId);

	// 3. 내 공용판 업데이트
	m_public_tile[nRow][nCol] = GetTileFromId(nId);

	// 4. 화면 갱신 (배경 지우고 다시 그리기)
	Invalidate(TRUE);
}

bool CClientDlg::IsExistingPublicTile(int tileId)
{
	// ID가 -1이거나 유효하지 않으면 검사할 필요 없음
	if (tileId <= 0) return false;

	// 백업해둔 공용판(m_old_public_tile)을 뒤져서 해당 ID가 있는지 확인
	for (int i = 0; i < 14; ++i)
	{
		for (int j = 0; j < 28; ++j)
		{
			if (m_old_public_tile[i][j].tileId == tileId)
			{
				// 찾았다 = 원래부터 공용판에 있던 tile
				return true;
			}
		}
	}
	// 못 찾았다 = 이번 턴에 내가 새로 올려둔 tile
	return false;
}
//==================================================

void CClientDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	if (m_bisGameStarted) // 게임 진행 중에 서버가 탈주한 경우
	{
		CString requestMsg;
		requestMsg.Format(_T("type:EndGame|sender:%s|isNormalEnd:0"), m_strName);
		RequestMessage(requestMsg);
	}
}
//==================================================


void CClientDlg::OnBnClickedButtonSetback()
{
	if (!m_bisGameStarted) {
		CString strTmpLog;
		strTmpLog.Format(_T("[INFO] 아직 게임이 시작되지 않았습니다."));
		m_list_message.AddString(strTmpLog);
		m_list_message.SetTopIndex(m_list_message.GetCount() - 1);
		Invalidate(TRUE);

		return;
	}

	if (m_bCurrentTurn) {
		CString strMsg;
		strMsg.Format(_T("type:SetbackReq|sender:Client"));
		RequestMessage(strMsg);
		Invalidate(TRUE);

		m_nSubmitTileNum = 0;
	}
	else {
		CString strTmpLog;
		strTmpLog.Format(_T("[INFO] 턴이 돌아오지 않았습니다."));
		m_list_message.AddString(strTmpLog);
		m_list_message.SetTopIndex(m_list_message.GetCount() - 1);
		Invalidate(TRUE);
	}
}


void CClientDlg::AddPlayerToList(CString strName, int nTileCount, DWORD_PTR nID)
{
	// 1. ID 중복 체크 (이미 등록된 ID라면 무시)
	// ID가 0이 아닌 경우에만 체크 (서버가 ID를 제대로 보냈다는 가정)
	if (nID != 0)
	{
		for (int i = 0; i < m_listPlayer.GetItemCount(); i++)
		{
			if (m_listPlayer.GetItemData(i) == nID) return;
		}
	}

	int nIndex = m_listPlayer.GetItemCount();

	// 이름 삽입
	m_listPlayer.InsertItem(nIndex, strName);

	// 타일 수 설정
	CString strCount;
	strCount.Format(_T("%d"), nTileCount);
	m_listPlayer.SetItemText(nIndex, 1, strCount);

	// [핵심] 리스트 아이템에 ID(소켓 주소값) 숨겨두기
	m_listPlayer.SetItemData(nIndex, nID);
}

void CClientDlg::UpdatePlayerTileCount(DWORD_PTR nID, int nTileNum)
{
    int nCount = m_listPlayer.GetItemCount();

    for (int i = 0; i < nCount; i++)
    {
        // 숨겨둔 ID 확인
        if (m_listPlayer.GetItemData(i) == nID)
        {
            CString strNum;
            strNum.Format(_T("%d"), nTileNum);
            
            // 1번 컬럼(TileNum) 갱신
            m_listPlayer.SetItemText(i, 1, strNum);
            return; 
        }
    }
}

// 개인 타일수 업데이트 이후 타일수 업데이트 해주세요~ 요청
void CClientDlg::UpdateSelfTileNum() {
	//--타일수 업데이트;
	int nCount = 0;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 18; j++)
			if (m_private_tile[i][j].tileId != -1) nCount++;

	m_intPrivateTileNum = nCount;

	CString requestMsg;
	requestMsg.Format(_T("type:UpdateTileNum|sender:%s|tilenum:%d"), m_strName, m_intPrivateTileNum);
	RequestMessage(requestMsg);

}


// player list 그리기
void CClientDlg::OnNMCustomdrawListPlayer(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

	*pResult = CDRF_DODEFAULT;

	// 1. 그리기 주기 시작
	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYITEMDRAW; // 각 아이템 그릴 때 알림 요청
	}
	// 2. 각 아이템 그리기 전
	else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		// 현재 행의 ItemData(=Player ID)를 가져옴
		int nItem = (int)pLVCD->nmcd.dwItemSpec;
		DWORD_PTR nRowID = m_listPlayer.GetItemData(nItem);

		// 현재 턴인 ID와 일치하면 색상 변경 (노란 배경, 빨간 글씨)
		if (nRowID == m_pTurn)
		{
			pLVCD->clrTextBk = RGB(255, 255, 200); // 연한 노란색 배경
			pLVCD->clrText = RGB(255, 0, 0);       // 빨간색 글씨
		}
		else
		{
			// 기본 색상
			pLVCD->clrTextBk = RGB(255, 255, 255);
			pLVCD->clrText = RGB(0, 0, 0);
		}

		*pResult = CDRF_NEWFONT; // 변경 사항 적용
	}
}

void CClientDlg::InitControls() {
	if (GetDlgItem(IDC_BUTTON_RECEIVE)) {
		GetDlgItem(IDC_BUTTON_RECEIVE)->MoveWindow(720, 555, 80, 40);
	}
	// [PASS / 턴넘김] 버튼
	if (GetDlgItem(IDC_BUTTON_PASS)) {
		GetDlgItem(IDC_BUTTON_PASS)->MoveWindow(810, 555, 80, 40);
	}
	// [SET BACK / 되돌리기] 버튼
	if (GetDlgItem(IDC_BUTTON_SETBACK)) {
		GetDlgItem(IDC_BUTTON_SETBACK)->MoveWindow(720, 605, 80, 40);
	}

	// [플레이어 목록] (버튼들 옆에 배치)
	if (GetDlgItem(IDC_LIST_PLAYER)) {
		GetDlgItem(IDC_LIST_PLAYER)->MoveWindow(900, 555, 150, 90);
	}


	// -------------------------------------------------------------
	// 3. [오른쪽 상단] 공용판(width=980) 옆 빈 공간 (x > 1000)
	// -------------------------------------------------------------

	// [서버 연결] 버튼
	if (GetDlgItem(IDC_BUTTON_CONNECT)) {
		GetDlgItem(IDC_BUTTON_CONNECT)->MoveWindow(1000, 35, 100, 30);
	}

	// [연결 상태 텍스트] (연결 버튼 옆)
	if (GetDlgItem(IDD_STATIC_STATUS)) {
		GetDlgItem(IDD_STATIC_STATUS)->MoveWindow(1110, 40, 150, 20);
	}

	// [채팅/메시지 목록]
	if (GetDlgItem(IDC_LIST_MESSAGE)) {
		GetDlgItem(IDC_LIST_MESSAGE)->MoveWindow(1000, 75, 260, 370);
	}

	// [채팅 입력창]
	if (GetDlgItem(IDC_EDIT_SEND)) {
		GetDlgItem(IDC_EDIT_SEND)->MoveWindow(1000, 455, 190, 30);
	}
	// [보내기 버튼]
	if (GetDlgItem(IDC_BUTTON_SEND)) {
		GetDlgItem(IDC_BUTTON_SEND)->MoveWindow(1200, 455, 60, 30);
	}
}