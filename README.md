# MFC Rummikub - JOKER

> **C++ MFC와 비동기 소켓 통신을 기반으로 구현한 루미큐브**
> 
> 로컬 환경 멀티플레이 루미큐브
> 2~4인 플레이 가능
> C++ MFC 기반 제작
>
## 🚀 시작하기 (Getting Started)

### 저장소 클론 (Repository Clone)
아래 명령어를 통해 프로젝트 소스코드를 로컬 환경으로 가져옵니다.
```bash
git clone [https://github.com/SSUNOWL/MFC_Project.git](https://github.com/SSUNOWL/MFC_Project.git)
```


<img width="1244" height="695" alt="image" src="https://github.com/user-attachments/assets/f59dd1c5-06ad-45cd-9023-bc82a66b865e" />

<img width="1243" height="683" alt="image" src="https://github.com/user-attachments/assets/9e83f7bb-bdb5-4872-b784-bd13f60dcf80" />

### 서버와 클라이언트의 IP를 연동해서 게임을 플레이하세요!
> 
<img width="502" height="225" alt="image" src="https://github.com/user-attachments/assets/273f3bfd-960c-4c77-8ed7-d2c6723cee65" />

> 동일 네트워크내에 있어야 진행할 수 있습니다.
>
> 서버가 IP를 입력하여 방을 파면 클라이언트에서 동일 IP를 입력해서 입장하세요
>
> https://www.vpn.net/
>
> Logmein Hamachi를 통해 원격 접속이 가능합니다.
> 

## 👥 팀원 및 역할 (Team JOKER)

본 프로젝트는 기능 단위의 모듈화 개발을 통해 협업을 진행하였습니다.

| 이름 | 역할 | 상세 구현 내용 |
| :--- | :--- | :--- |
| **김선재** | **네트워크 및 게임 흐름 제어** | • 비동기 소켓 기반 서버-클라이언트 통신 레이어(CListenSocket, CServiceSocket) <br> • 라운드 로빈 방식의 실시간 턴 순환 시스템 및 게임 시작/타일 분배 로직 관리 <br> • 실시간 채팅 시스템 및 시스템 로그 출력 기능 구현|
| **김환희** | **타일 조작 및 예외 처리** | • 마우스 클릭 이벤트 기반 타일 선택, 이동 및 교환 기능 구현 <br> • 기존 공용판 타일의 개인판 회수 금지 등 조작 관련 예외 처리 규칙 적용 <br> • 프로젝트 최종 결과 보고서 정리 및 데이터 관리|
| **이준혁** | **게임 규칙 검증 엔진** | • 공용판 타일 유효성 검사 알고리즘(Run/Group/Chunk) 구현 <br> • 첫 등록 시 30점 이상 제한 및 타일 섞임 방지(IsChunkMixed) 로직 개발 <br> • Pass 버튼 핸들링 및 유효성 기반 턴 종료 제어 시스템 구축|
| **임정빈** | **상태 복구 및 타일 수급** | • 턴 시작 시점 스냅샷 백업 및 Setback(되돌리기) 기능 구현 <br> • 덱에서 타일을 획득하는 Receive 기능 및 개인판 용량 초과 예외 처리 <br> • 프로젝트 발표 PPT 제작 및 시각 자료 정리|
| **차준혁** | **그래픽 렌더링 및 결과 처리** | • `CImage` 활용 타일 이미지 로딩 및 보드 시각화/UI 렌더링 시스템 구축 <br> • 해상도 및 DPI 대응을 위한 디자인 좌표계 기반 스케일링 시스템 설계 지원 <br> • 게임 종료 시 벌점 집계 및 최종 순위 계산/출력 로직 구현|
---

## ✨ 주요 기능 (Key Features)

### 1. 실시간 멀티플레이 시스템 (Network)
* **비동기 소켓 통신**: `CAsyncSocket`을 상속받은 `CListenSocket`과 `CServiceSocket`을 통해 최대 4인의 멀티플레이를 지원합니다.
* **안정적인 메시지 프로토콜**: TCP 스트림의 메시지 쪼개짐 현상을 방지하기 위해 `4바이트 헤더(길이) + UTF-8 페이로드` 구조를 적용했습니다.
* **중앙 관리형 서버**: 모든 게임 상태(덱, 공용판)는 서버가 권위를 가지며, 클라이언트에 실시간으로 브로드캐스트하여 동기화합니다.

### 2. 완벽한 게임 규칙 구현 (Game Logic)
* **엄격한 조합 검사**: 루미큐브의 핵심인 **런(Run)**과 **그룹(Group)** 규칙을 청크(Chunk) 단위로 파싱하여 검증합니다.
* **첫 등록 30점 규칙**: 첫 번째 타일 제출 시 숫자 합이 30 이상이어야 하는 규칙을 강제합니다.
* **조커 타일 처리**: 모든 색상과 숫자를 대체할 수 있는 조커의 특성을 반영하고, 배치 위치에 따른 예외 처리를 완료했습니다.
* **SetBack 및 Receive**: 잘못된 조작 시 턴 시작 상태로 복구하는 기능과 타일 획득 후 자동 턴 넘김 기능을 지원합니다.

### 3. 고도화된 UI/UX (User Interface)
* **DPI 및 해상도 대응**: 모니터 배율이나 해상도에 관계없이 UI가 깨지지 않도록 디자인 좌표(1280x720) 기반의 스케일링 헬퍼 함수를 적용했습니다.
* **커스텀 렌더링**: `OnPaint`와 `DrawMyTiles`를 통해 직접 디자인한 PNG 타일 리소스를 격자에 맞게 렌더링합니다.
* **실시간 채팅 및 로그**: 플레이어 간 소통을 위한 채팅창과 게임 이벤트를 기록하는 시스템 로그를 제공합니다.

---

## 🛠 기술 스택 (Tech Stack)

* **언어**: C++
* **프레임워크**: Microsoft Foundation Class (MFC)
* **네트워크**: WinSock2 (Asynchronous Socket)
* **그래픽**: CImage (PNG 리소스 렌더링)
* **IDE**: Visual Studio 2022

---

## 📂 프로젝트 구조

```text
├── Server/Client Dlg  # 메인 UI 및 이벤트 핸들링
├── Socket Classes     # CAsyncSocket 기반 통신 레이어
├── Tile Structure     # 타일 속성(색상, 숫자, ID) 정의
├── Validation Logic   # 런/그룹 및 점수 계산 엔진
└── Resources (PNG)    # Canva로 디자인된 106장의 타일 이미지
