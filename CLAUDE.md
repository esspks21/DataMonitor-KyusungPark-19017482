# DataMonitor-KyusungPark-19017482 — CLAUDE.md

## 프로젝트 개요

반도체 시료 생산주문 현황을 콘솔에서 실시간으로 조회하는 **관리자용 데이터 모니터링 도구** PoC 프로젝트입니다. JSON 파일을 데이터 소스로 삼아 외부 라이브러리 없이 순수 C++20으로 구현되었습니다.

---

## 아키텍처

```
DataMonitor-KyusungPark-19017482/
├── DataMonitor-KyusungPark-19017482/
│   ├── main.cpp          ← 전체 구현 (단일 파일 PoC)
│   └── data/
│       └── orders.json   ← 반도체 생산주문 샘플 데이터
└── DataMonitor-KyusungPark-19017482.slnx
```

### 주요 구성 요소 (main.cpp 내부)

| 구성 요소 | 설명 |
|---|---|
| `JsonValue` / `JsonParser` | 외부 의존성 없는 Recursive-Descent JSON 파서 |
| `Order` | 반도체 생산주문 도메인 모델, `fromJson()` 정적 팩토리 |
| `namespace con` | ANSI 색상·콘솔 제어 유틸리티 |
| `DataMonitor` | 데이터 로드·조회·필터·통계 로직, 메뉴 루프 |

---

## 데이터 모델 (Order)

| 필드 | JSON 키 | 타입 | 설명 |
|---|---|---|---|
| orderId | `order_id` | string | 주문번호 |
| sampleType | `sample_type` | string | 시료 종류 (Silicon Wafer, SiC Wafer 등) |
| grade | `grade` | string | 등급 (Prime / Production / Research / Test) |
| quantity | `quantity` | int | 수량 |
| unit | `unit` | string | 단위 (ea, 장, L) |
| status | `status` | string | 상태: `대기중` / `진행중` / `완료` / `취소` |
| priority | `priority` | string | 우선순위: `HIGH` / `MEDIUM` / `LOW` |
| customer | `customer` | string | 고객사명 |
| createdDate | `created_date` | string | 등록일 (YYYY-MM-DD) |
| deadline | `deadline` | string | 마감일 (YYYY-MM-DD) |
| processStep | `process_step` | string | 현재 공정 단계 |
| lotId | `lot_id` | string | LOT 번호 |
| defectRate | `defect_rate` | float | 불량률 (0.0~1.0) |
| operatorName | `operator` | string | 담당자 |
| notes | `notes` | string | 비고 |

---

## JSON 파서 설계

`JsonParser`는 Recursive-Descent 방식으로 동작합니다.

- `parseValue()` → 타입 분기 후 각 파서 호출
- `parseObject()` / `parseArray()` / `parseString()` / `parseNumber()` / `parseBool()`
- `JsonValue`는 `std::map<string, JsonValue>` + `std::vector<JsonValue>` 유니온 구조
- 이스케이프 시퀀스(`\"`, `\\`, `\n`, `\r`, `\t`) 지원

---

## 기능 목록

1. **전체 주문 목록** — 테이블 형식 출력
2. **상태별 필터** — 대기중 / 진행중 / 완료 / 취소
3. **시료종류별 필터** — JSON 데이터 기준 동적 목록 생성
4. **우선순위 정렬** — HIGH → MEDIUM → LOW
5. **주문 상세 조회** — 주문번호 입력, 전 필드 출력
6. **통계 요약** — 상태별·우선순위별·시료종류별 집계, 평균 불량률
7. **데이터 새로고침** — 파일 재로드 (핫 리로드 PoC)

---

## 빌드 환경

- **표준**: C++20 (`std::map` structured bindings, `std::ostringstream` 등)
- **플랫폼**: Windows (MSVC v145), Win32/x64 구성 지원
- **의존성**: 표준 라이브러리만 사용 (`<iostream>`, `<fstream>`, `<map>`, `<vector>`, `<algorithm>`, `<chrono>`)
- **Windows 전용**: `<windows.h>` 로 ANSI VT 처리 및 UTF-8 콘솔 설정

---

## 실행 방법

```
DataMonitor-KyusungPark-19017482.exe [JSON 파일 경로]
```

- 인수 생략 시 실행 디렉터리 기준 `data/orders.json` 사용
- Visual Studio에서 실행 시 작업 디렉터리는 프로젝트 폴더(`$(ProjectDir)`)

---

## 확장 방향 (PoC 이후)

- `orders.json` 을 실시간 갱신하는 외부 프로세스(MES 연동)와 연계
- 파일 감시(FileSystemWatcher) 기반 자동 새로고침
- 주문 상태 변경 기능 추가 (JSON write-back)
- 필터 조합 검색 (상태 + 우선순위 + 날짜 범위)
- 불량률 임계치 초과 시 경고 출력
