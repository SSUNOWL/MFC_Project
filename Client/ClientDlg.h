
// ClientDlg.h: 헤더 파일
//

#pragma once
#include "ClientSocket.h"
#include <vector>
#include <list>


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
	afx_msg void OnNMCustomdrawListPlayer(NMHDR* pNMHDR, LRESULT* pResult);
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
	afx_msg void OnDestroy();
	bool m_bisGameStarted;

	// [251127] 타일 선택 및 이동 관련 변수
	bool m_bIsSelected;         // 현재 타일이 선택되었는지 여부
	bool m_bSelectedFromPublic; // 선택된 타일이 공용판(true)인지 개인판(false)인지
	int m_nSelectedRow;         // 선택된 타일의 행 (배열 인덱스)
	int m_nSelectedCol;         // 선택된 타일의 열 (배열 인덱스)

	// [251127] 공용판 동기화 함수
	Tile GetTileFromId(int tileId);              // ID로 타일 복원
	void ProcessPublicBoardUpdate(CString strMsg); // 서버에서 온 변경사항 반영
	void SendUpdatePublicTile(int row, int col);   // 변경사항 서버로 전송

	// [251127] 기존 공용판에 있던 타일인지 검사하는 함수
	bool IsExistingPublicTile(int tileId);
private:
	bool IsRowValid(int row, int* sum);
	bool IsRunValid(std::list<Tile> tileChunk);
	bool IsGroupValid(std::list<Tile> tileChunk);
	void DrawMyTiles(CDC& dc);
	int  GetTileImageIndex(const Tile& tile) const;
public:
	afx_msg void OnBnClickedButtonSetback();
	// [251127] 마우스 클릭 이벤트 처리 함수 선언
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	void AddPlayerToList(CString strName, int nTileCount, DWORD_PTR nID = 0);
	void UpdatePlayerTileCount(DWORD_PTR nID, int nTileNum);
	CListCtrl m_listPlayer;
	void UpdateSelfTileNum();
	DWORD_PTR m_pTurn;
	int m_nSubmitTileNum;

	// 첫 번째 제출 검증 관련 함수
	bool IsChunkMixed(const std::list<Tile>& tileChunk, bool* isAllNew);
	int CalculateChunkValue(const std::list<Tile>& tileChunk, bool isRun);
	bool m_bFirstSubmit; // 첫 번째 제출 여부
};
