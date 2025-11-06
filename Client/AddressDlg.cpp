// AddressDlg.cpp: 구현 파일
//

#include "pch.h"
#include "Client.h"
#include "afxdialogex.h"
#include "AddressDlg.h"


// CAddressDlg 대화 상자

IMPLEMENT_DYNAMIC(CAddressDlg, CDialogEx)

CAddressDlg::CAddressDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_ADDRESS, pParent)
	, m_strName(_T(""))
{

}

CAddressDlg::~CAddressDlg()
{
}

void CAddressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDX_Control(pDX, IDC_IPADDRESS_IP, m_ipaddress_ip);
}


BEGIN_MESSAGE_MAP(CAddressDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CAddressDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CAddressDlg 메시지 처리기

void CAddressDlg::OnBnClickedOk()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

    // 1. IP 주소 컨트롤에서 4개의 BYTE 값 가져오기
    BYTE field0, field1, field2, field3;
    int nFields = m_ipaddress_ip.GetAddress(field0, field1, field2, field3);

    // 2. 입력이 완료되었는지 확인
    if (nFields == 4)
    {
        // 3. 공개 멤버 변수에 IP 주소 문자열 저장
        m_strIPAddress.Format(_T("%u.%u.%u.%u"), field0, field1, field2, field3);

        // 4. CDialogEx::OnOK() 호출하여 대화상자 종료 (반환값 IDOK)
        CDialogEx::OnOK();
    }
    else
    {
        // IP 주소가 완전히 입력되지 않은 경우
        AfxMessageBox(_T("IP 주소를 완전히 입력해 주세요."));
        // 종료하지 않고 대화상자에 남아 있도록 CDialogEx::OnOK()를 호출하지 않음
    }
    CDialogEx::OnOK();
}
