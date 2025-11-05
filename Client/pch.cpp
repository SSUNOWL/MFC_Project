#include "pch.h"
#include "afxsock.h"
// #include "Utils.h" // 별도의 파일로 분리했을 경우

// CString (유니코드) -> UTF-8 (std::string) 변환
std::string CStringToUTF8(const CString& strInput)
{
    if (strInput.IsEmpty())
        return "";

    // 1. 필요한 버퍼 크기 계산 (널 문자 포함)
    int nLen = WideCharToMultiByte(CP_UTF8, 0, strInput, -1, NULL, 0, NULL, NULL);

    // 2. 버퍼 할당
    std::unique_ptr<char[]> buffer(new char[nLen]);

    // 3. 변환 실행
    WideCharToMultiByte(CP_UTF8, 0, strInput, -1, buffer.get(), nLen, NULL, NULL);

    // 4. std::string 반환 (널 문자를 제외한 길이로 생성)
    return std::string(buffer.get());
}

// UTF-8 (std::string) -> CString (유니코드) 변환
CString UTF8ToCString(const std::string& strInput)
{
    if (strInput.empty())
        return CString();

    // 1. 필요한 버퍼 크기 계산 (널 문자 포함, TCHAR 기준)
    int nLen = MultiByteToWideChar(CP_UTF8, 0, strInput.c_str(), (int)strInput.length(), NULL, 0);

    // 2. CString 버퍼 확보
    CString strOutput;
    LPTSTR psz = strOutput.GetBuffer(nLen);

    // 3. 변환 실행
    MultiByteToWideChar(CP_UTF8, 0, strInput.c_str(), (int)strInput.length(), psz, nLen);

    // 4. 버퍼 해제 및 CString 반환
    strOutput.ReleaseBuffer();
    return strOutput;
}