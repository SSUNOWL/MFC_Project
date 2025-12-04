// ServerDlg.h: 헤더 파일
//

#pragma once
#include "ListenSocket.h"
#include "ServiceSocket.h"
#include <array>
#include <list>
#include <map>
#include <algorithm>

// CServerDlg 대화 상자
class CServerDlg : public CDialogEx
{
	// 생성입니다.
public:
	CServerDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	virtual ~CServerDlg();
	const int DESIGN_WIDTH = 1920;
	const int DESIGN_HEIGHT = 1080;
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
	afx_msg void OnNMCustomdrawListPlayer(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()

public:
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
		int tileId = -1; // 나머지 1~106, 비어있는 판으로 표시하기 위해 -1로 초기화

	};
	// [GAMEOVER] 점수 계산용 구조체
	struct PlayerResult
	{
		CString         name;
		CServiceSocket* pSocket;
		int             tileCount;    // 남은 타일 개수
		int             sumNumbers;   // 남은 타일 숫자 합
		int             jokerCount;   // 남은 조커 개수
		int             finalScore;   // 최종 점수 (+면 이김, -면 패)
		bool            isWinner;
	};
	struct PlayerScoreStat
	{
		int sumNumbers = 0;
		int jokerCount = 0;
	};
	std::map<CServiceSocket*, PlayerScoreStat> m_playerScore;

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
	void Backup();
	void Setback();

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

	afx_msg void OnDestroy();

	// [251127] 타일 선택 및 이동 관련 변수
	bool m_bIsSelected;         // 현재 타일이 선택되었는지 여부
	bool m_bSelectedFromPublic; // 선택된 타일이 공용판(true)인지 개인판(false)인지
	int m_nSelectedRow;         // 선택된 타일의 행
	int m_nSelectedCol;         // 선택된 타일의 열

	// [251127] 공용판 동기화 관련 함수
	void SendUpdatePublicTile(int row, int col); // 특정 좌표의 타일 변경사항 전송
	Tile GetTileFromId(int tileId);              // ID로 타일 객체 생성
	void ProcessPublicBoardUpdate(CString strMsg); // 클라이언트의 요청 처리

	// [251127] 규칙 검사 및 백업 함수
	bool IsExistingPublicTile(int tileId);
	void CopyBoards(); // 턴 시작 시 현재 상태 백업

	// [251204] 게임 종료 기능 변수 및 함수
	bool m_bGameOver; //게임 종료 여부
	void HandleGameOver(CServiceSocket* pWinnerSocket);
	void GetPlayerTileStat(CServiceSocket* pSocket, int& outSum, int& outJoker);
	void SetPlayerScoreStat(CServiceSocket* pSocket, int sum, int joker);

private:
	bool IsRowValid(int row, int* sum);
	bool IsRunValid(std::list<Tile> tileChunk);
	bool IsGroupValid(std::list<Tile> tileChunk);
	void DrawMyTiles(CDC& dc);
	int  GetTileImageIndex(const Tile& tile) const;

	// [251127] 마우스 왼쪽 클릭 메시지 함수
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

	// 첫 번째 제출 검증 관련 함수
	bool IsChunkMixed(const std::list<Tile>& tileChunk, bool* isAllNew);
	int CalculateChunkValue(const std::list<Tile>& tileChunk, bool isRun);

public:
	afx_msg void OnClickedButtonSetback();
	void UpdatePlayerTileCount(CServiceSocket* pSocket, int nTileNum);
	CListCtrl m_listPlayer;
	void UpdateSelfTileNum();
	void AddPlayerToList(CString strName, int nTileCount, CServiceSocket* pSocket = nullptr);
	CServiceSocket* m_pTurn;
	int m_nSubmitTileNum;
	bool m_bFirstSubmit; // 첫 번째 제출 여부
	void InitControls();
	double GetDPIScale();
	CPoint GetScaledPoint(CPoint designPt);
	CPoint ScreenToDesign(CPoint screenPt);
	double GetScaleX();
	double GetScaleY();
	int GetScaledSize(int designSize);

};


