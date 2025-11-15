#include "pch.h"
#include "framework.h"
#include "Client.h"
#include "ClientDlg.h"
#include "afxdialogex.h"
#include "afxsock.h"
#include "AddressDlg.h"
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
}

BEGIN_MESSAGE_MAP(CClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CClientDlg::OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_SEND, &CClientDlg::OnBnClickedButtonSend)
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
		// Connect 호출 후 즉시 함수 종료되며, 결과는 OnConnect 콜백으로 통보됩니다.
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
		else // 이 경우는 매우 드물게 Connect가 바로 성공한 경우입니다.
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

	//  1. 유니코드 -> ANSI 변환을 위해 USES_CONVERSION 매크로를 함수 시작 부분에 추가 (필수)
	USES_CONVERSION;

	//  2. CT2A 매크로를 사용하여 LPCSTR로 안전하게 변환
	// CAsyncSocket::Send는 멀티바이트(ANSI) 문자열을 받습니다.


	std::string utf8_data = CStringToUTF8(strMsg);
	// 2. 소켓을 통해 서버로 데이터 전송
	// CAsyncSocket::Send 함수는 비동기로 작동하며, 성공 시 보낸 바이트 수를 반환
	int nBytesSent = m_pClientSocket->Send(utf8_data.c_str(), (int)utf8_data.length()); //  수정

	if (nBytesSent == (int)utf8_data.length())
	{
		//정상 작동시 
	}
	else if (nBytesSent == SOCKET_ERROR)
	{

		CString strStatus;
		strStatus.Format(_T("연결 실패! (에러코드: %d)"), m_pClientSocket->GetLastError());
		m_static_status.SetWindowText(strStatus);
	}
	// nBytesSent < strSend.GetLength()인 경우: 다음에 다시 보내거나 버퍼링해야 함 (복잡해지므로 단순화)



}




Tile CClientDlg::ParseIdtoTile(int Tileid) {
	// 해결책: 변수 선언과 동시에 기본값을 할당하여
	// 모든 코드 경로에서 초기화 상태를 보장합니다.
	Color c = BLACK;
	Tile newTile = Tile{ BLACK, 0, false, 0 }; // 초기화

	if (Tileid >= 105) { // 조커 타일 (105, 106)
		newTile = Tile{ BLACK, 0, true , Tileid };
	}
	else { // 일반 타일 (1~104)
		int color_group = Tileid / 26;

		// switch-case가 if-else if보다 더 명확합니다.
		switch (color_group) {
		case 0: c = RED; break;
		case 1: c = GREEN; break;
		case 2: c = BLUE; break;
		case 3: c = BLACK; break;
			// default 케이스를 넣으면 범위 외의 값(0, 4 이상)이 들어와도 c는 초기화된 값(BLACK)을 유지합니다.
		default: break;
		}

		newTile = Tile{ c, Tileid % 26 + 1, false, Tileid };
	}
	return newTile;
}