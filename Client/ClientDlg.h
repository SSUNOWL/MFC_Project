/*
// ClientDlg.h: 헤더 파일
//

#pragma once
#include "ClientSocket.h"
#include <vector>
#include <list>
#include "afxwin.h" // CImage, CDC 등을 위해 포함

enum Color {
	RED,
	GREEN,
	BLUE,
	BLACK
};

struct Tile {
	Color color;
	int num;
	bool isJoker;
	int tileId = -1;
};

struct SelectedTileInfo {
	bool bSelected = false;   // 현재 선택된 타일이 있는지 여부
	bool bIsPublic = false;   // 선택된 타일이 공용판(true)인지 개인판(false)인지
	int row = -1;             // 행 인덱스
	int col = -1;             // 열 인덱스
};

// CClientDlg 대화 상자
class CClientDlg : public CDialogEx
{
	// 생성입니다.
public:
	Tile m_tile_list[106]; //전체 타일 리스트
	Tile m_rand_tile_list[106]; //타일 섞은 후 저장용
	Tile m_rand_tile_list_cpy[106]; //타일 섞은 후 저장용


	Tile m_public_tile[14][28]; // 공용판은 13x27. 1~13, 1~27로 좌표 지정
	Tile m_private_tile[4][18]; // 개인판은 3x17. 1~3, 1~17로 좌표 지정

	Tile m_old_public_tile[14][28]; // 되돌리기용.
	Tile m_old_private_tile[4][18]; // 되돌리기용. 

	CImage m_tile_image_list[106];


	void InitTiles();
	void ClearBoards();
	void CopyBoards();
	void CopyBoardsReverse();
	//void ShuffleTiles(int swaps = 300);
	inline Tile MakeEmptyTile() const { return Tile{ BLACK, 0, false }; }
	Tile ParseIdtoTile(int Tileid);

	void LoadImage();
	bool LoadPngFromResource(CImage& img, UINT uResID);

public:
	CClientDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CLIENT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다

protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()


public:
	CEdit m_edit_send;
	CListBox m_list_message;
//	CStatic m_static_status;
	afx_msg void OnBnClickedButtonConnect();
	afx_msg void OnBnClickedButtonSend();
	//  통신 클래스 포인터
	CClientSocket* m_pClientSocket;
	//  메시지 출력 함수 (소켓 클래스에서 호출될 함수)
	void DisplayMessage(const CString& strSender, const CString& strMsg, BOOL bReceived);
	CStatic m_static_status;

	//-------------------
	void RequestMessage(CString& strMsg);
	CString m_strName;
	bool m_bCurrentTurn;
	afx_msg void OnBnClickedButtonPass();
	bool IsPublicTileValid();
	int m_intPrivateTileNum;
	afx_msg void OnBnClickedButtonReceive();
private:
	bool IsRowValid(int);
	bool IsRunValid(std::list<Tile> tileChunk);
	bool IsGroupValid(std::list<Tile> tileChunk);
	void DrawMyTiles(CDC& dc);
	int  GetTileImageIndex(const Tile& tile) const;
/// <summary>
/// 251113
/// </summary>
protected:
	SelectedTileInfo m_selInfo; // [추가] 선택 정보 멤버 변수

	// [추가] 좌표 변환 및 로직 헬퍼 함수
	bool GetBoardIndexFromPoint(CPoint point, bool& bIsPublic, int& row, int& col);
	void MoveTile(bool bSrcPublic, int sRow, int sCol, bool bDestPublic, int dRow, int dCol);

	// ... 생성된 메시지 맵 함수 ...
protected:
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point); // [추가] 마우스 왼쪽 클릭
	DECLARE_MESSAGE_MAP()
};
*/

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// ClientDlg.h : 헤더 파일
//

#pragma once
#include "ClientSocket.h"
#include <vector>
#include <list>
#include "afxwin.h" // CImage, CDC 등을 위해 포함

enum Color {
	RED,
	GREEN,
	BLUE,
	BLACK
};

struct Tile {
	Color color;
	int num;
	bool isJoker;
	int tileId = -1; // [수정] 오타 수정 (=- 1 -> = -1)
};

// [추가] 선택된 타일 정보를 저장할 구조체
struct SelectedTileInfo {
	bool bSelected = false;   // 현재 선택된 타일이 있는지 여부
	bool bIsPublic = false;   // 선택된 타일이 공용판(true)인지 개인판(false)인지
	int row = -1;             // 행 인덱스
	int col = -1;             // 열 인덱스
};

// CClientDlg 대화 상자
class CClientDlg : public CDialogEx
{
	// 생성입니다.
public:
	CClientDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CLIENT_DIALOG };
#endif

	// ==========================================
	// 1. 멤버 변수 (Data & UI)
	// ==========================================
public:
	// 타일 데이터
	Tile m_tile_list[106];           // 전체 타일 리스트
	Tile m_rand_tile_list[106];      // 타일 섞은 후 저장용
	Tile m_rand_tile_list_cpy[106];  // 되돌리기용 복사본

	Tile m_public_tile[14][28];      // 공용판 (13x27 사용)
	Tile m_private_tile[4][18];      // 개인판 (3x17 사용)

	Tile m_old_public_tile[14][28];  // 되돌리기용
	Tile m_old_private_tile[4][18];  // 되돌리기용 

	// 이미지 리소스
	CImage m_tile_image_list[106];

	// 통신 및 상태
	CClientSocket* m_pClientSocket = nullptr;
	CString m_strName;
	bool m_bCurrentTurn;
	int m_intPrivateTileNum;

	// UI 컨트롤
	CEdit m_edit_send;
	CListBox m_list_message;
	CStatic m_static_status;

	// ==========================================
	// 2. 멤버 함수 (Logic & Helper)
	// ==========================================
public:
	void InitTiles();
	void ClearBoards();
	void CopyBoards();
	void CopyBoardsReverse();

	inline Tile MakeEmptyTile() const { return Tile{ BLACK, 0, false }; }
	Tile ParseIdtoTile(int Tileid);

	// 이미지 로드
	void LoadImage();
	bool LoadPngFromResource(CImage& img, UINT uResID);

	// 통신 관련
	void DisplayMessage(const CString& strSender, const CString& strMsg, BOOL bReceived);
	void RequestMessage(CString& strMsg);

	// 게임 로직 검증
	bool IsPublicTileValid();

	// [추가] 공용판 동기화를 위한 함수
	CString SerializePublicBoard(); // 공용판 -> 문자열
	void DeserializePublicBoard(CString strBoardData); // 문자열 -> 공용판
	void ProcessMessage(CString strMsg);
	// ==========================================
	// 3. 내부 구현 (Protected/Private)
	// ==========================================
protected:
	HICON m_hIcon;
	SelectedTileInfo m_selInfo; // [선택 정보]

	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();

	// 로직 헬퍼
	bool IsRowValid(int row);
	bool IsRunValid(std::list<Tile> tileChunk);
	bool IsGroupValid(std::list<Tile> tileChunk);

	// 그리기 및 좌표 계산
	void DrawMyTiles(CDC& dc);
	int GetTileImageIndex(const Tile& tile) const;
	bool GetBoardIndexFromPoint(CPoint point, bool& bIsPublic, int& row, int& col);
	void MoveTile(bool bSrcPublic, int sRow, int sCol, bool bDestPublic, int dRow, int dCol);

	// ==========================================
	// 4. 메시지 맵 (MFC Event Handlers)
	// ==========================================
protected:
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	// 마우스 클릭 이벤트 [중요]
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

	// 버튼 이벤트
	afx_msg void OnBnClickedButtonConnect();
	afx_msg void OnBnClickedButtonSend();
	afx_msg void OnBnClickedButtonPass();
	afx_msg void OnBnClickedButtonReceive();

	DECLARE_MESSAGE_MAP()
};

