# DataAsset 정리 진행 상황

## 1차: 라운드 편집 단위 통합

`DA_DefaultRun.Rounds`는 이제 튜토리얼 1개 + 표시 라운드 1~10의 11개 행이다.
튜토리얼을 제외하고 위에서부터 차례대로 화면의 라운드 번호가 된다.
각 행의 `Difficulty Schedule`을 펼쳐 해당 라운드의 웨이브만 편집한다.
원본 난이도를 런타임에 둘로 나눠 화면의 라운드로 만드는 기존 동작은,
저장된 명시적 라운드 데이터로 옮겼다. 이관된 Run은 다시 분할하지 않는다.

각 행에 함께 모은 설정:

- `Experiment`: 기믹
- `Enemy Class`: 해당 라운드의 적 클래스. 지정 시 기존과 같이 일반 체력 혼합·기믹을 적용하지 않는다
- `Enemy Count Override`: 0이면 웨이브 수에 `Enemy Count Multiplier` 적용, 양수면 해당 수로 대체
- `Single Wave`: 첫 웨이브 종료 시 라운드 완료
- `Override Enemy Health` / `Enemy Health`: 일반 적 체력 구성. 기존 이전 라운드 프로필 상속 결과를 명시적으로 복사했다
- `Base Attack Timing Multiplier`: 기본 간격 배율. 이후 Run의 라운드별 증가율과 기믹 배율이 적용된다

데이터를 복사해 계산하므로 플레이·재시작으로 저장된 웨이브 수치가 누적 변경되지 않는다.
무한모드는 기존과 같이 최종 라운드를 재사용한다.

기존 숫자 키 기반 맵은 호환·복구를 위해 직렬화 필드로 남겼지만 편집 UI에서는 숨겼다.
게임 모드는 이 맵을 읽지 않는다. 과거 Setup 스크립트가 이 맵을 수정하더라도
이관된 Run에는 반영되지 않으므로 라운드 조정에 재실행하지 않는다.
새 입력 어댑터/시트 연동은 새 `Rounds` 구조를 대상으로 작성해야 한다.
현재 공통 속성 편집 도구는 구조체 말단을 지원하지만 배열 인덱스나 중첩된 라운드 배열 전체 편집은 지원하지 않는다.

## 이관 및 복구 근거

에디터를 닫고 `Scripts/Editor/MigrateExplicitRounds.py`를 Python commandlet으로 실행했다.
기본 Run과 테스트 Run만 수정하며, 현재 디스크 파일을 `Saved/Migrations/ExplicitRounds_*`에 먼저 복사한다.
시작 라운드와 모든 웨이브의 export text를 비교하고 저장 후 재로드하여 다시 비교한다.
테스트 Run은 기존 내용을 분할한 4개 행이며 기본 Run과 같은 11개를 강제하지 않는다.
이미 이관한 에셋에 다시 실행하면 분할·저장하지 않는다.
새 Run을 직접 작성할 때는 Advanced의 `Explicit Round Layout`을 먼저 켜고 표시 라운드를 추가한다.

검증:

- Development Editor 빌드 성공
- 이관 commandlet: 기본 Run 11행 / 테스트 Run 4행, 두 Run의 기존 시작 라운드 0 유지
- `ReflectionLab.Run.RoundLayoutMigration`: 홀수 웨이브 분할, 프로필 상속·기믹·적 수 예외 이관, 중복 이관 방지
- 전체 `ReflectionLab` 자동 테스트 27개 성공
- PIE 화면·패키징 동작은 별도 검증 필요

## 2차: 기믹별 설정 그룹 분리

기존 `DA_*` 파일 경로와 라운드의 참조는 유지한다. 기믹별 별도 파일을 추가하지 않고
한 에셋 안의 설정을 타입별 USTRUCT로 분리했으며 `Type`에 해당하는 그룹만 표시한다.

- Crossfire: 교차 사격 타이밍·벽·적 배치
- Field Layout: 가속/굴절 영역의 위치와 크기
- Acceleration: 가속 배율·기본 탄속 배율·가속음
- Refraction: 굴절 각도
- Support: 지원 유닛·범위·공격 배율·보호막
- Zones: 교대 장판 타이밍·범위·피해
- Mirror: 대각 반사벽·중앙 구역·시각 높이·카메라·반사 횟수

표시 이름·공격 빈도 배율·기본 탄환은 공통 설정으로 유지한다.
다른 기믹에서도 쓰던 `Shutter Material`은 의미에 맞게 `Surface Material`로 옮겼다.
교차 사격과 장판이 공유하던 Phase/Warning은 각 그룹으로 복사하여 이후 독립적으로 편집한다.
구형 Mirror Distance / Mirror Half Width는 실행 경로가 없으므로 편집창에서 제거했다.
구형 필드 자체는 직렬화 호환·이관 비교를 위해 보존하며 런타임에서는 그룹 값만 사용한다.

`MigrateArenaSettings.py`로 기존 기믹 6개를 백업하고 에셋마다 35개 이관 항목 및 공통 설정을
비교하여 저장·재로드 확인했다. 반사벽 높이 120 등 현재 조정값도 유지된다.
구버전 에셋은 PostLoad에서 한 번 변환하며, 새 에셋은 처음부터 그룹 설정을 사용한다.
구형 생성/튜닝 스크립트는 재실행하지 않는다. MCP 공통 도구는 구형 평면 속성 편집을 거부한다.
새 경로 예: `mirror.shot_barrier_visual_height`, `acceleration.field_speed_multiplier`.

검증: Development Editor 빌드 성공 / ReflectionLab 자동 테스트 28개 성공 /
Python 단위 테스트 17개 성공 / 임시 에셋을 이용한 중첩 속성 적용·저장·재로드·복구 성공.
에디터 화면과 PIE의 실제 연출은 별도 확인한다.

## 다음 단계 — 아직 미완료

3. 일반 적·보스·미니언의 기본 능력치 위치 통일
4. Asset Registry 참조 확인 후 테스트·구버전 에셋 폴더 분리
5. 플레이어·투사체의 설정 그룹 정돈

1·2차 단계에서는 에셋 삭제·폴더 이동을 하지 않았다.
