# TreasureHunt 프로젝트

## 개요
- Unreal Engine 5.5.4, 1인칭 멀티 서바이벌, 2인 개발 (포트폴리오용)
- Listen Server + Steam Online Subsystem
- 나는 언리얼/Git 초보. 설명은 원리부터 차근차근, 한 번에 하나씩.

## 절대 규칙
- 모든 게임 로직은 서버 권한 검증: HasAuthority() + Server RPC 패턴 준수
- Listen → Dedicated 전환 가능한 구조 유지

## 인벤토리 (기획서 v1.2) — 현재 작업 중
- 5×4 = 20칸 그리드 배치 방식 (슬롯 합산 아님)
- 아이템은 GridWidth × GridHeight 모양대로 칸 차지 (ItemData)
- 각 아이템 위치는 FInventoryEntry의 GridX/GridY에 저장
- 백엔드 완료: FindFreeGridPosition으로 빈 자리 탐색 후 배치
- 남은 작업: 그리드 UI (WBP는 블루프린트라 내가 에디터에서 직접 함)
- 정리 예정: SlotsRequired, MaxSlots는 미사용이나 BP/DataAsset가 참조 중이라 보류

## Git
- 브랜치: feat-작업내용 / 커밋: feat: 내용, 한 커밋 = 한 작업
- uasset은 Git LFS 관리. 머지 후 uasset 에러 나면 파일 크기 확인(256바이트면 LFS 포인터 → git lfs fetch --all + checkout)

## 작업 규칙
- git 명령(checkout, merge, stash, reset, push)은 실행하지 말고 나에게 제안만. 내가 GitHub Desktop에서 직접 처리
- Content 폴더의 .uasset은 바이너리라 읽기/수정 불가. 블루프린트 작업은 내가 에디터에서 함
- Git 작업 전 언리얼 에디터와 Visual Studio 반드시 종료
- 위험한 변경은 2단계로: 먼저 조사·보고 → 내 판단 → 실행
- 빌드는 내가 Visual Studio에서 직접 (Development Editor / Win64)
