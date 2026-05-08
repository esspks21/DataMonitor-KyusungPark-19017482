# DataMonitor — 반도체 시료 생산주문관리 모니터링 도구

반도체 시료 생산주문 데이터를 콘솔에서 실시간으로 조회하는 관리자 도구입니다.  
JSON 파일 기반으로 동작하며 외부 라이브러리 없이 순수 C++20으로 작성되었습니다.

---

## 주요 기능

| 기능 | 설명 |
|---|---|
| 전체 주문 목록 | 등록된 모든 생산주문을 테이블로 표시 |
| 상태별 필터 | 대기중 / 진행중 / 완료 / 취소 중 선택 조회 |
| 시료종류별 필터 | Silicon Wafer, SiC Wafer, GaN Substrate 등 종류별 조회 |
| 우선순위 정렬 | HIGH → MEDIUM → LOW 순서로 정렬 출력 |
| 주문 상세 조회 | 주문번호 입력 시 전체 필드 상세 출력 |
| 통계 요약 | 상태·우선순위·시료종류별 집계 및 평균 불량률 |
| 데이터 새로고침 | JSON 파일을 재로드하여 최신 상태 반영 |

---

## 빌드 및 실행

### Visual Studio

1. `DataMonitor-KyusungPark-19017482.slnx` 파일을 Visual Studio 2022에서 열기
2. **솔루션 빌드** (`Ctrl+Shift+B`)
3. **F5** 또는 **디버그 없이 시작** (`Ctrl+F5`)

### 명령줄 실행

```
DataMonitor-KyusungPark-19017482.exe
```

기본 데이터 파일: `data/orders.json` (실행 파일과 같은 디렉터리 기준)

사용자 지정 데이터 파일 지정:
```
DataMonitor-KyusungPark-19017482.exe C:\path\to\custom_orders.json
```

---

## 데이터 파일 구조

`data/orders.json` 파일을 직접 편집하여 주문 데이터를 추가·수정할 수 있습니다.

```json
{
  "orders": [
    {
      "order_id": "ORD-2024-001",
      "sample_type": "Silicon Wafer",
      "grade": "Prime",
      "quantity": 200,
      "unit": "ea",
      "status": "진행중",
      "priority": "HIGH",
      "customer": "삼성전자",
      "created_date": "2024-01-10",
      "deadline": "2024-02-10",
      "process_step": "포토리소그래피",
      "lot_id": "LOT-240110-001",
      "defect_rate": 0.015,
      "operator": "김철수",
      "notes": "DDR5 메모리 공정용 긴급 주문"
    }
  ]
}
```

**status 허용값**: `대기중` / `진행중` / `완료` / `취소`  
**priority 허용값**: `HIGH` / `MEDIUM` / `LOW`  
**defect_rate**: 0.0 ~ 1.0 사이 소수 (예: 0.015 = 1.5%)

---

## 환경 요구사항

- Windows 10/11
- Visual Studio 2022 (MSVC v145)
- C++20 이상

---

## 샘플 데이터

총 10건의 반도체 생산주문 샘플 데이터가 포함되어 있습니다.

| 시료종류 | 고객사 예시 |
|---|---|
| Silicon Wafer | 삼성전자, SK하이닉스 |
| SiC Wafer | 현대모비스, SK하이닉스 |
| Photomask | DB하이텍, 한국전자통신연구원 |
| GaN Substrate | 네패스, KAIST 나노연구소 |
| CMP Slurry | 삼성전자 |
