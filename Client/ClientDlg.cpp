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

std::string CStringToUTF8(const CString& str)
{
	CT2A ascii(str, CP_UTF8);
	return std::string(ascii.m_psz);
}
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
}

BEGIN_MESSAGE_MAP(CClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_LBUTTONDOWN() // [추가] 마우스 클릭 이벤트 연결 1123
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CClientDlg::OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_SEND, &CClientDlg::OnBnClickedButtonSend)
	ON_BN_CLICKED(IDC_BUTTON_PASS, &CClientDlg::OnBnClickedButtonPass)
	ON_BN_CLICKED(IDC_BUTTON_RECEIVE, &CClientDlg::OnBnClickedButtonReceive)
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

	m_intPrivateTileNum = 0;

	// [추가] 선택 정보 초기화
	m_selInfo = { false, false, -1, -1 };
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

		// [추가] 선택된 타일이 있다면 빨간색 테두리 표시
		if (m_selInfo.bSelected)
		{
			int startX, startY;
			if (m_selInfo.bIsPublic) {
				startX = 35; startY = 35;
			}
			else {
				startX = 105; startY = 555;
			}

			int drawX = startX + (m_selInfo.col - 1) * 35;
			int drawY = startY + (m_selInfo.row - 1) * 35;

			// 빨간색 펜으로 테두리 그리기
			CPen pen(PS_SOLID, 3, RGB(255, 0, 0));
			CPen* pOldPen = dc.SelectObject(&pen);
			CBrush* pOldBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH); // 투명 브러시

			dc.Rectangle(drawX, drawY, drawX + 35, drawY + 35);

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

	for (int i = 1; i <= 13; i++) // 공용판의 각 행에 대해 반복
	{
		if (!IsRowValid(i))
		{
			return false;
		}
	}

	return true;
}

bool CClientDlg::IsRowValid(int row)
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

void CClientDlg::OnBnClickedButtonPass()
{
	if (!m_bCurrentTurn) {
		AfxMessageBox(_T("턴이 돌아오지 않았습니다.", MB_OK));

		return;
	}

	//유효성 검증코드
	if (IsPublicTileValid()) // 공용판이 올바른 경우
	{
		// 타일 개수 최신화 요청 전송
		CString requestMsg;
		requestMsg.Format(_T("type:UpdateTileNum|sender:%s|tilenum:%d"), m_strName, m_intPrivateTileNum);
		RequestMessage(requestMsg);


		// 턴 종료
		CString strMsg;
		strMsg.Format(_T("type:EndTurn|sender:%s"), m_strName);
		RequestMessage(strMsg);
		m_bCurrentTurn = false;
	}
	else // 공용판이 올바르지 않은 경우
	{
		AfxMessageBox(_T("공용판이 올바르지 않습니다.", MB_OK));
	}

	Invalidate(FALSE);
}


void CClientDlg::OnBnClickedButtonReceive()
{
	if (m_bCurrentTurn) {
		CString strMsg;
		strMsg.Format(_T("type:Receive|sender:%s"), m_strName);
		RequestMessage(strMsg);
		m_bCurrentTurn = false;
	}

	Invalidate(FALSE);
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


bool CClientDlg::GetBoardIndexFromPoint(CPoint point, bool& bIsPublic, int& row, int& col)
{
	const int CELL_SIZE = 35;

	// 1. 공용판 영역 체크 (35, 35) ~ (980, 490) 근처
	// 행: 1~13, 열: 1~27
	int pubStartX = 35;
	int pubStartY = 35;

	if (point.x >= pubStartX && point.x < pubStartX + 27 * CELL_SIZE &&
		point.y >= pubStartY && point.y < pubStartY + 13 * CELL_SIZE)
	{
		bIsPublic = true;
		col = (point.x - pubStartX) / CELL_SIZE + 1;
		row = (point.y - pubStartY) / CELL_SIZE + 1;
		return true;
	}

	// 2. 개인판 영역 체크 (105, 555) ~ ...
	// 행: 1~3, 열: 1~17
	int privStartX = 105;
	int privStartY = 555;

	if (point.x >= privStartX && point.x < privStartX + 17 * CELL_SIZE &&
		point.y >= privStartY && point.y < privStartY + 3 * CELL_SIZE)
	{
		bIsPublic = false;
		col = (point.x - privStartX) / CELL_SIZE + 1;
		row = (point.y - privStartY) / CELL_SIZE + 1;
		return true;
	}

	return false; // 판 밖을 클릭함
}

void CClientDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// 내 턴이 아니면 조작 불가 (필요시 주석 해제)
	if (!m_bCurrentTurn) {
		return;
	}

	bool bClickedPublic = false;
	int nRow = -1, nCol = -1;

	// 1. 유효한 판 위를 클릭했는지 확인
	if (!GetBoardIndexFromPoint(point, bClickedPublic, nRow, nCol)) {
		// 판 밖을 클릭하면 선택 해제
		m_selInfo.bSelected = false;
		Invalidate(TRUE); // [수정] 배경까지 지워야 테두리가 깔끔하게 사라짐
		CDialogEx::OnLButtonDown(nFlags, point);
		return;
	}

	// 2. 타일 선택/이동 로직
	if (!m_selInfo.bSelected)
	{
		// [선택 모드] 
		// 아직 선택된 타일이 없을 때 -> 클릭한 위치에 타일이 있으면 선택
		Tile t;
		if (bClickedPublic) t = m_public_tile[nRow][nCol];
		else                t = m_private_tile[nRow][nCol];

		// 빈 타일이 아니면 선택 상태로 변경
		bool isEmpty = (t.color == BLACK && t.num == 0 && !t.isJoker);
		if (!isEmpty) {
			m_selInfo.bSelected = true;
			m_selInfo.bIsPublic = bClickedPublic;
			m_selInfo.row = nRow;
			m_selInfo.col = nCol;

			// 선택 시에는 테두리만 그리면 되므로 FALSE여도 되지만, 안전하게 TRUE 권장
			Invalidate(FALSE);
		}
	}
	else
	{
		// [이동 모드] 
		// 이미 타일을 선택한 상태 -> 클릭한 위치로 이동(또는 스왑)

		// 제자리 클릭 시 (선택 취소)
		if (m_selInfo.bIsPublic == bClickedPublic && m_selInfo.row == nRow && m_selInfo.col == nCol) {
			m_selInfo.bSelected = false; // [수정] 선택 해제
		}
		else {
			// 다른 위치 클릭 시 (이동 수행)
			MoveTile(m_selInfo.bIsPublic, m_selInfo.row, m_selInfo.col, bClickedPublic, nRow, nCol);
			m_selInfo.bSelected = false; // [수정] 이동 후 선택 상태 즉시 해제 (빨간 테두리 제거)
		}

		// [중요 수정] Invalidate(TRUE)를 해야 배경을 지우고 다시 그립니다.
		// 배경을 지워야 "이동 전 위치"에 있던 타일 그림이 사라집니다.
		Invalidate(TRUE);
	}

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CClientDlg::MoveTile(bool bSrcPublic, int sRow, int sCol, bool bDestPublic, int dRow, int dCol)
{
	// 1. 타일 스왑 (이동 로직)
	Tile& srcTile = (bSrcPublic) ? m_public_tile[sRow][sCol] : m_private_tile[sRow][sCol];
	Tile& destTile = (bDestPublic) ? m_public_tile[dRow][dCol] : m_private_tile[dRow][dCol];

	Tile temp = destTile;
	destTile = srcTile;
	srcTile = temp;

	// 2. [핵심] 공용판이 변경되었으면 서버에 알림
	if (bSrcPublic || bDestPublic)
	{
		// 현재 공용판 상태를 문자열로 변환
		CString strBoard = SerializePublicBoard();

		// 서버로 전송 (프로토콜: type:UpdateBoard | sender:나 | board:데이터)
		CString strMsg;
		strMsg.Format(_T("type:UpdateBoard|sender:%s|board:%s"), m_strName, strBoard);
		RequestMessage(strMsg);
	}
}

CString CClientDlg::SerializePublicBoard() // 서버는 CServerDlg:: 로 변경
{
	CString strData = _T("");
	CString strTemp;

	for (int r = 1; r <= 13; ++r) {
		for (int c = 1; c <= 27; ++c) {
			int id = -1;
			// 빈 타일인지 확인 (ServerDlg의 경우 구조체 멤버가 다를 수 있으니 확인 필요)
			// ClientDlg는 TileId가 -1인지, 혹은 color==BLACK && num==0인지 확인
			Tile& t = m_public_tile[r][c];

			// 빈 타일 조건 (작성하신 코드 기준)
			if (t.color == BLACK && t.num == 0 && !t.isJoker)
				id = -1;
			else
				id = t.tileId;

			strTemp.Format(_T("%d,"), id);
			strData += strTemp;
		}
	}
	return strData;
}

// 수신받은 문자열을 공용판에 반영
void CClientDlg::DeserializePublicBoard(CString strBoardData) // 서버는 CServerDlg:: 로 변경
{
	int r = 1;
	int c = 1;
	int nPos = 0;

	// 콤마(,)를 기준으로 자름
	CString strToken = strBoardData.Tokenize(_T(","), nPos);

	while (!strToken.IsEmpty())
	{
		int nTileId = _ttoi(strToken);

		// ID를 이용해 타일 정보 복원
		if (nTileId == -1) {
			// 빈 타일 생성 (MakeEmptyTile 함수 활용 권장)
			m_public_tile[r][c] = Tile{ BLACK, 0, false, -1 };
		}
		else {
			// ID로 타일 생성 (서버에도 ParseIdtoTile 함수가 있어야 함)
			m_public_tile[r][c] = ParseIdtoTile(nTileId);
		}

		c++;
		if (c > 27) { // 다음 줄로
			c = 1;
			r++;
			if (r > 13) break;
		}
		strToken = strBoardData.Tokenize(_T(","), nPos);
	}

	Invalidate(TRUE); // 화면 갱신
}

void CClientDlg::ProcessMessage(CString strMsg)
{
	int nPos = 0;
	CString strToken = strMsg.Tokenize(_T("|"), nPos);

	CString strType = _T("");
	CString strSender = _T("");
	CString strContent = _T("");
	CString strBoardData = _T("");

	// 추가 데이터 파싱용
	CString strPos = _T("");
	CString strTileId = _T("");
	CString strName = _T("");
	CString strNum = _T("");

	// 1. 파싱 루프
	while (!strToken.IsEmpty())
	{
		int nSep = strToken.Find(_T(":"));
		if (nSep != -1)
		{
			CString strKey = strToken.Left(nSep);
			CString strValue = strToken.Mid(nSep + 1);

			if (strKey == _T("type")) strType = strValue;
			else if (strKey == _T("sender")) strSender = strValue;
			else if (strKey == _T("content")) strContent = strValue;
			else if (strKey == _T("board")) strBoardData = strValue;

			else if (strKey == _T("pos")) strPos = strValue;
			else if (strKey == _T("tileid")) strTileId = strValue;
			else if (strKey == _T("name")) strName = strValue;
			else if (strKey == _T("num")) strNum = strValue;
		}
		strToken = strMsg.Tokenize(_T("|"), nPos);
	}

	// 2. 로직 실행
	if (strType == _T("UpdateBoard"))
	{
		// 공용판 업데이트 (동기화 문제 해결을 위해 조건 없이 수행 권장)
		DeserializePublicBoard(strBoardData);
	}
	else if (strType == _T("CHAT"))
	{
		DisplayMessage(strSender, strContent, TRUE);
	}
	else if (strType == _T("GetName"))
	{
		CString strResponse;
		strResponse.Format(_T("type:GetName|sender:%s|name:%s"), m_strName, m_strName);
		RequestMessage(strResponse);
	}
	else if (strType == _T("StartTurn"))
	{
		m_bCurrentTurn = true;``
		AfxMessageBox(_T("당신의 턴입니다!"));
	}
	else if (strType == _T("StartTile"))
	{
		int nX = 0, nY = 0, nId = 0;
		int comma_pos = strPos.Find(_T(","));
		if (comma_pos != -1) {
			nX = _ttoi(strPos.Left(comma_pos));
			nY = _ttoi(strPos.Mid(comma_pos + 1));
		}
		nId = _ttoi(strTileId);
		m_private_tile[nX][nY] = ParseIdtoTile(nId);
		Invalidate(FALSE);
	}
	else if (strType == _T("ReceiveTile"))
	{
		int nId = _ttoi(strTileId);
		bool received = false;
		for (int i = 1; i <= 3; i++) {
			for (int j = 1; j <= 17; j++) {
				if (m_private_tile[i][j].tileId == -1) {
					m_private_tile[i][j] = ParseIdtoTile(nId);
					received = true;
					break;
				}
			}
			if (received) break;
		}
		Invalidate(FALSE);
	}
	else if (strType == _T("Accept"))
	{
		int nNum = _ttoi(strNum) + 1;
		CString strSystemMsg;
		strSystemMsg.Format(_T("%s님이 입장하였습니다. 현재 %d명"), strName, nNum);
		DisplayMessage(_T("시스템"), strSystemMsg, TRUE);
	}
}