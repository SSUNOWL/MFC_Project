
// ServerDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "Server.h"
#include "ServerDlg.h"
#include "afxdialogex.h"
#include "ListenSocket.h"
#include "ServiceSocket.h"

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


// CServerDlg 대화 상자



CServerDlg::CServerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SERVER_DIALOG, pParent)
	, m_pListenSocket(nullptr)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
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
	CString strSend;
	m_edit_send.GetWindowText(strSend);
	CString strSender = _T("서버");
	CString strType = _T("CHAT");
	BroadcastMessage(strType, strSender, strSend, 0);

	m_edit_send.SetWindowText(_T(""));
	DisplayMessage(strSender, strSend, FALSE);
}

void CServerDlg::OnBnClickedButtonStart()
{

	if (m_pListenSocket != nullptr)
	{
		AddLog(_T("이미 서버가 실행 중입니다."));
		return;
	}

	//  1. AfxSocketInit()은 CWinApp::InitInstance()에서 이미 호출되었다고 가정합니다.
	UINT nPort = 12345; // 클라이언트와 동일한 포트 사용

	m_pListenSocket = new CListenSocket(this);

	// 2. 소켓 생성 및 바인딩
	if (!m_pListenSocket->Create(nPort))
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


void CServerDlg::ProcessAccept(CListenSocket* pListenSocket)
{
	//  1. 새로운 CServiceSocket 객체를 생성합니다.
	CServiceSocket* pNewSocket = new CServiceSocket(this);

	// 2. 리스닝 소켓의 Accept 함수를 호출하여 연결을 새 소켓에 할당
	if (pListenSocket->Accept(*pNewSocket))
	{
		// 3. 성공 시, 소켓 목록에 추가
		m_clientSocketList.AddTail(pNewSocket);

		CString strLog;
		strLog.Format(_T("INFO: 클라이언트 연결 수락됨 (현재 %d명)"), m_clientSocketList.GetCount());
		AddLog(strLog);
	}
	else
	{
		// 4. 실패 시, 객체 해제
		AddLog(_T("ERROR: 클라이언트 연결 수락 실패!"));
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


void CServerDlg::BroadcastMessage(const CString& strType, const CString& strSender, const CString& strMsg, CServiceSocket* pSender)
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

			//  3. 메시지 전송 (Send 함수 사용)
			// CString을 CT2A 매크로를 이용해 ANSI/멀티바이트 문자열로 변환하여 전송
			 CString strMessageToSend;
			 strMessageToSend.Format(
				 _T("type:%s|sender:%s|content:%s"),
				 strType, strSender, strMsg
			 );
			 
			 std::string utf8_data = CStringToUTF8(strMessageToSend);
			 strLog.Format(_T("BROADCAST: %s"), strMessageToSend);
			 AddLog(strLog);
			 // 2. 소켓을 통해 서버로 데이터 전송
			 // CAsyncSocket::Send 함수는 비동기로 작동하며, 성공 시 보낸 바이트 수를 반환
			 int nBytesSent = pSocket->Send(utf8_data.c_str(), (int)utf8_data.length()); //  수정



			// 전송 오류 처리
			if (nBytesSent == SOCKET_ERROR)
			{
				// 오류가 발생하면 해당 소켓의 연결을 강제로 끊거나 로그를 남길 수 있습니다.
				pSocket->Close();
			}
		}
	}
}