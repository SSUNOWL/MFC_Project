
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
	int tileId=-1;
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
};
