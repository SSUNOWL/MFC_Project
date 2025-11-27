/*
// ServerDlg.h: 헤더 파일
//

#pragma once
#include "ListenSocket.h"
#include "ServiceSocket.h"
#include <array>
#include <list>

#include <algorithm>

// CServerDlg 대화 상자
class CServerDlg : public CDialogEx
{
// 생성입니다.
public:
	CServerDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	virtual ~CServerDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SERVER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButtonSend();
	CListBox m_list_log;
	afx_msg void OnBnClickedButtonStart();
	//  1. 소켓 객체 포인터
	CListenSocket* m_pListenSocket;
//  2. 연결된 클라이언트 소켓 목록
	CList<CServiceSocket*, CServiceSocket*> m_clientSocketList;
//  4. 소켓 콜백에서 호출될 핵심 관리 함수들
	// 클라이언트 연결 수락 처리 (CListenSocket::OnAccept에서 호출)
	void ProcessAccept(CListenSocket* pListenSocket);

	// 클라이언트 연결 종료 처리 (CServiceSocket::OnClose에서 호출)
	void RemoveClient(CServiceSocket* pServiceSocket);

	
	void AddLog(const CString& strMsg);
	void DisplayMessage(const CString& strSender, const CString& strMsg, BOOL bReceived);
	CEdit m_edit_send;
	CListBox m_list_message;
	afx_msg void OnBnClickedButtonReceive();

	// 색상을 정의하는 열거형(enum)
	enum Color {
		RED,
		GREEN,
		BLUE,
		BLACK
	};

	// 타일 구조체
	struct Tile
	{
		Color color;
		int num;
		bool isJoker;
		int tileId=-1; // 나머지 1~106, 비어있는 판으로 표시하기 위해 -1로 초기화

	};
	// === [게임 상태: 서버 권위] ===
   // 전체 106장(색 4 × 1~13 × 2세트 = 104 + 조커 2)
	std::array<Tile, 106> m_tile_list;      // 원본 덱
	std::array<Tile, 106> m_rand_tile_list; // 셔플 덱 --> 우리가 사용할꺼임
	int m_deck_pos = 0; // 다음에 줄 카드 인덱스 (0부터 증가)

	// 공용판 13x27, 개인판 3x17 (인덱스는 1..13/27 을 쓰려면 0행/0열은 패딩)
	Tile m_old_public_tile[14][28]{};
	Tile m_old_private_tile[4][18]{};
	Tile m_public_tile[14][28]{};
	Tile m_private_tile[4][18]{};

	CImage m_tile_image_list[106];


	// === [유틸/로직] ===
	void InitTiles();                 // 106장 생성
	void ShuffleTiles();              // 셔플
	bool DealOneTileTo(class CServiceSocket* pSock); // 한 장 지급
	static CString TileToString(const Tile& t);      // 직렬화
	static Tile    MakeJoker();       // 조커 생성
	static Tile    MakeEmpty();       // 빈칸 생성
	void PlayGame();
	void NextTurn();
	void Receive();

	void LoadImage();
	bool LoadPngFromResource(CImage& img, UINT uResID);


	//======
	//단일 대상한테만 보내기 -> receive버튼, 타일 돌리기
	void ResponseMessage(const CString& strMsg, CServiceSocket* pSender);
	// 메시지 전체 클라이언트에게 전송 (CServiceSocket::OnReceive에서 호출) 
	void BroadcastMessage(const CString& strMsg, CServiceSocket* pSender);


	CString m_strName;
	//	int m_intTurnPos;
	afx_msg void OnBnClickedButtonPlay();
	POSITION m_posTurn;
	bool m_bCurrentTurn;
	bool m_bisGameStarted;
	afx_msg void OnBnClickedButtonPass();
	bool IsPublicTileValid();
	int m_intPrivateTileNum;
private:
	bool IsRowValid(int);
	bool IsRunValid(std::list<Tile> tileChunk);
	bool IsGroupValid(std::list<Tile> tileChunk);
	void DrawMyTiles(CDC& dc);
	int  GetTileImageIndex(const Tile& tile) const;
};
*/

/*
// ServerDlg.h: 헤더 파일
//

#pragma once
#include "ListenSocket.h"
#include "ServiceSocket.h"
#include <array>
#include <list>
#include <algorithm>
#include "afxwin.h" // CImage, CDC 지원

// [추가] 선택된 타일 정보를 저장할 구조체
struct SelectedTileInfo {
	bool bSelected = false;   // 현재 선택된 타일이 있는지 여부
	bool bIsPublic = false;   // 선택된 타일이 공용판(true)인지 개인판(false)인지
	int row = -1;             // 행 인덱스
	int col = -1;             // 열 인덱스
};

// CServerDlg 대화 상자
class CServerDlg : public CDialogEx
{
	// 생성입니다.
public:
	CServerDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	virtual ~CServerDlg();

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SERVER_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

	// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	// [추가] 마우스 클릭 핸들러
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButtonSend();
	CListBox m_list_log;
	afx_msg void OnBnClickedButtonStart();

	// 1. 소켓 객체 포인터
	CListenSocket* m_pListenSocket;
	// 2. 연결된 클라이언트 소켓 목록
	CList<CServiceSocket*, CServiceSocket*> m_clientSocketList;

	// 4. 소켓 콜백 관리 함수
	void ProcessAccept(CListenSocket* pListenSocket);
	void RemoveClient(CServiceSocket* pServiceSocket);

	void AddLog(const CString& strMsg);
	void DisplayMessage(const CString& strSender, const CString& strMsg, BOOL bReceived);
	CEdit m_edit_send;
	CListBox m_list_message;
	afx_msg void OnBnClickedButtonReceive();
	//[추가]
	CString SerializePublicBoard(); // 공용판 -> 문자열
	void DeserializePublicBoard(CString strBoardData); // 문자열 -> 공용판
	// 색상을 정의하는 열거형(enum)
	enum Color {
		RED,
		GREEN,
		BLUE,
		BLACK
	};

	// 타일 구조체
	struct Tile
	{
		Color color;
		int num;
		bool isJoker;
		int tileId = -1;
	};

	// === [게임 상태: 서버 권위] ===
	std::array<Tile, 106> m_tile_list;      // 원본 덱
	std::array<Tile, 106> m_rand_tile_list; // 셔플 덱
	int m_deck_pos = 0;

	// 공용판 13x27, 개인판 3x17
	Tile m_old_public_tile[14][28]{};
	Tile m_old_private_tile[4][18]{};
	Tile m_public_tile[14][28]{};
	Tile m_private_tile[4][18]{};

	CImage m_tile_image_list[106];

	// === [유틸/로직] ===
	void InitTiles();
	void ShuffleTiles();
	bool DealOneTileTo(class CServiceSocket* pSock);
	static CString TileToString(const Tile& t);
	static Tile    MakeJoker();
	static Tile    MakeEmpty();
	void PlayGame();
	void NextTurn();
	void Receive();

	void LoadImage();
	bool LoadPngFromResource(CImage& img, UINT uResID);

	// 통신 관련
	void ResponseMessage(const CString& strMsg, CServiceSocket* pSender);
	void BroadcastMessage(const CString& strMsg, CServiceSocket* pSender);

	CString m_strName;
	afx_msg void OnBnClickedButtonPlay();
	POSITION m_posTurn;
	bool m_bCurrentTurn;
	bool m_bisGameStarted;
	afx_msg void OnBnClickedButtonPass();
	bool IsPublicTileValid();
	int m_intPrivateTileNum;
public:
	// ... (기존 함수들 밑에 추가) ...

	// [추가] 타일 ID로 타일 객체 생성
	Tile ParseIdtoTile(int Tileid);

	// [추가] 공용판 직렬화/역직렬화
	CString SerializePublicBoard();
	void DeserializePublicBoard(CString strBoardData);

	// [추가] 수신된 메시지 처리 함수 (보낸 사람 소켓 포인터 포함)
	void ProcessMessage(CString strMsg, CServiceSocket* pSender);

private:
	bool IsRowValid(int);
	bool IsRunValid(std::list<Tile> tileChunk);
	bool IsGroupValid(std::list<Tile> tileChunk);
	void DrawMyTiles(CDC& dc);
	int  GetTileImageIndex(const Tile& tile) const;

	// [추가] 선택 및 이동 관련 멤버 변수/함수
protected:
	SelectedTileInfo m_selInfo;

	bool GetBoardIndexFromPoint(CPoint point, bool& bIsPublic, int& row, int& col);
	void MoveTile(bool bSrcPublic, int sRow, int sCol, bool bDestPublic, int dRow, int dCol);
};
*/

// ServerDlg.h : 헤더 파일
//

#pragma once
#include "ListenSocket.h"
#include "ServiceSocket.h"
#include <array>
#include <list>
#include <algorithm>
#include "afxwin.h" // CImage, CDC 지원

// [추가] 선택된 타일 정보를 저장할 구조체
struct SelectedTileInfo {
	bool bSelected = false;   // 현재 선택된 타일이 있는지 여부
	bool bIsPublic = false;   // 선택된 타일이 공용판(true)인지 개인판(false)인지
	int row = -1;             // 행 인덱스
	int col = -1;             // 열 인덱스
};

// CServerDlg 대화 상자
class CServerDlg : public CDialogEx
{
	// 생성입니다.
public:
	CServerDlg(CWnd* pParent = nullptr);    // 표준 생성자입니다.
	virtual ~CServerDlg();

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SERVER_DIALOG };
#endif

	// ==========================================
	// 1. 멤버 변수 (Data & UI)
	// ==========================================
public:
	// UI 컨트롤
	CListBox m_list_log;
	CEdit m_edit_send;
	CListBox m_list_message;

	// 소켓 통신
	CListenSocket* m_pListenSocket;
	CList<CServiceSocket*, CServiceSocket*> m_clientSocketList;

	// 게임 상태 변수
	CString m_strName;
	POSITION m_posTurn;
	bool m_bCurrentTurn;
	bool m_bisGameStarted;
	int m_intPrivateTileNum;

	// 타일 관련 정의
	enum Color { RED, GREEN, BLUE, BLACK };
	struct Tile {
		Color color;
		int num;
		bool isJoker;
		int tileId = -1;
	};

	// 타일 데이터
	std::array<Tile, 106> m_tile_list;      // 원본 덱
	std::array<Tile, 106> m_rand_tile_list; // 셔플 덱
	int m_deck_pos = 0;

	// 보드판 데이터
	Tile m_old_public_tile[14][28]{};
	Tile m_old_private_tile[4][18]{};
	Tile m_public_tile[14][28]{};
	Tile m_private_tile[4][18]{};

	// 이미지 리소스
	CImage m_tile_image_list[106];

	// ==========================================
	// 2. 멤버 함수 (Logic & Helper)
	// ==========================================
public:
	// 초기화 및 게임 진행
	void InitTiles();
	void ShuffleTiles();
	void PlayGame();
	void NextTurn();
	void Receive();

	// 유틸리티
	static Tile MakeJoker();
	static Tile MakeEmpty();
	bool DealOneTileTo(class CServiceSocket* pSock);
	static CString TileToString(const Tile& t);
	Tile ParseIdtoTile(int Tileid); // [중요] 타일 ID -> 객체 변환

	// 이미지 로드
	void LoadImage();
	bool LoadPngFromResource(CImage& img, UINT uResID);

	// 통신 관련
	void ProcessAccept(CListenSocket* pListenSocket);
	void RemoveClient(CServiceSocket* pServiceSocket);
	void ResponseMessage(const CString& strMsg, CServiceSocket* pSender);
	void BroadcastMessage(const CString& strMsg, CServiceSocket* pSender);
	void ProcessMessage(CString strMsg, CServiceSocket* pSender); // [중요] 메시지 처리

	// 로그 및 채팅
	void AddLog(const CString& strMsg);
	void DisplayMessage(const CString& strSender, const CString& strMsg, BOOL bReceived);

	// 게임 로직 검증
	bool IsPublicTileValid();

	// 공용판 동기화 [중요] (중복 제거됨)
	CString SerializePublicBoard();
	void DeserializePublicBoard(CString strBoardData);

	// ==========================================
	// 3. 내부 구현 (Protected/Private)
	// ==========================================
protected:
	HICON m_hIcon;
	SelectedTileInfo m_selInfo; // [선택 정보]

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();

	// 로직 헬퍼
	bool IsRowValid(int row);
	bool IsRunValid(std::list<Tile> tileChunk);
	bool IsGroupValid(std::list<Tile> tileChunk);

	// 그리기 및 좌표
	void DrawMyTiles(CDC& dc);
	int  GetTileImageIndex(const Tile& tile) const;
	bool GetBoardIndexFromPoint(CPoint point, bool& bIsPublic, int& row, int& col);
	void MoveTile(bool bSrcPublic, int sRow, int sCol, bool bDestPublic, int dRow, int dCol);

	// ==========================================
	// 4. 메시지 맵 (MFC Event Handlers)
	// ==========================================
protected:
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point); // 마우스 클릭

	// 버튼 이벤트
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButtonSend();
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonReceive();
	afx_msg void OnBnClickedButtonPlay();
	afx_msg void OnBnClickedButtonPass();

	DECLARE_MESSAGE_MAP()
};
