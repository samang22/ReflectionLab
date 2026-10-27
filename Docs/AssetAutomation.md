# 공통 데이터 에셋 수정·검증 도구와 MCP

## 목적과 범위

매 요청마다 일회용 Python 스크립트를 작성하지 않고, 기존 데이터 에셋의
속성 변경을 공통 절차로 처리한다. 게임 플레이 코드와 배포 동작은 변경하지 않는다

현재 지원 범위는 **기존 DataAsset의 직접 편집 속성 및 구조체 내부의 지원 타입 말단 속성**이다

- 숫자·논리값·문자열·Name·Enum
- 에셋 참조와 null 참조
- Vector2D·Vector·LinearColor·Rotator
- 위 값으로 구성된 배열
- `mirror.shot_barrier_visual_height`처럼 구조체를 거치는 말단 경로 (최대 4단계)

로컬라이즈된 Text는 번역 식별자를 잃지 않도록 변경을 거부한다
맵 액터, 블루프린트 기본 객체, 머티리얼 그래프, 임의 구조체 전체 교체·Map·Set,
에셋 생성·삭제는 이 공통 도구의 범위에 포함하지 않는다
중간 경로는 구조체만 허용하며 객체 참조를 넘어가거나 배열 인덱스로 접근하지 않는다
필요하면 별도의 명시적 어댑터와 테스트를 추가한다

## 자동 실행 흐름

1. 사람이 원하는 변경을 요청한다
2. AI가 현재 에셋과 속성을 확인하고 `preview`를 호출한다
3. 변경 전·후 값이 담긴 계획 ID와 JSON 기록이 생성된다
4. 요청 범위에 해당하는 계획을 `apply`로 적용한다
5. 수정한 에셋만 저장하고 패키지를 재로드한 뒤 값을 비교한다
6. 결과를 보고하고, 요청 시 `rollback`으로 해당 속성만 복구한다

PIE·Simulate 중에는 변경을 거부한다. 대상 에셋에 저장하지 않은 변경이 있어도
덮어쓰지 않고 중단한다. 이 경우 사용자가 기존 변경을 저장하거나 취소한 뒤
새 미리보기를 생성한다

미리보기 이후 메모리 값이나 에셋 파일이 달라지면 적용을 거부한다
이미 적용한 계획을 다시 실행해도 중복 저장하지 않는다
요청하지 않은 속성은 수정하지 않으며, 허용 경로 밖의 변경도 거부한다

## MCP 연결

에디터의 Python 초기화 파일 `Content/Python/init_unreal.py`가
`rl_asset_toolset.RLAssetToolset`을 등록한다. 에디터 시작 시 에셋 수정은 하지 않는다
새 도구를 처음 설치한 경우 에디터 재시작이 필요하며, 이후 속성 조정에는
C++ 재빌드나 에디터 종료가 필요하지 않다

필요 조건은 Python Editor Script Plugin, ToolsetRegistry, ModelContextProtocol이다
현재 프로젝트의 MCP 주소는 `.codex/config.toml`에서 관리한다
서버가 꺼져 있으면 에디터를 `-ModelContextProtocolStartServer` 인자로 시작할 수 있다
이 구현은 서버의 공개 범위나 인증 설정을 변경하지 않는다

연결 후 도구 목록에서 다음 도구를 확인한다

- `rl_asset_toolset.RLAssetToolset.preview(request_json)`
- `rl_asset_toolset.RLAssetToolset.apply(plan_id)`
- `rl_asset_toolset.RLAssetToolset.verify(plan_id)`
- `rl_asset_toolset.RLAssetToolset.rollback(plan_id)`

예를 들어 벽 높이 변경을 요청했다면 아래 JSON을 `preview`에 전달한다
예시이므로 이 문서를 읽는 것만으로 실제 값이 변경되지는 않는다

```json
{
  "changes": [
    {
      "asset": "/Game/ReflectionLab/Data/Experiments/DA_MirrorArena",
      "property": "mirror.shot_barrier_visual_height",
      "expected": 100.0,
      "value": 120.0
    }
  ]
}
```

`expected`는 선택 사항이며, 지정하면 현재 값과 다를 때 미리보기부터 중단한다
에셋 경로는 `.에셋이름` 접미사가 없는 패키지 경로를 사용한다
속성명은 Python 편집 속성 이름인 snake_case를 사용한다. 구조체 말단은 점으로 구분한다
기믹의 구형 평면 속성은 이관 후 편집을 거부한다. 새 그룹 경로를 사용한다

벡터는 `{"$type":"Vector2D","x":950,"y":800}`,
참조는 `{"$type":"Object","path":"/Game/.../Asset.Asset"}`,
Enum은 `{"$type":"Enum","class":"RLArenaExperimentType","value":"MIRROR_ARENA"}` 형식이다

## 기록·실패·복구

계획과 결과는 `Saved/AssetChanges/<계획ID>.json`에 남긴다
프로젝트 식별자, 원래 값, 요청 값, 에셋 파일 지문, 진행 상태를 포함한다
계획은 첫 에셋 변경 전에 디스크에 기록된다

정상 적용은 `applied`, 정상 복구는 `rolled_back`, 적용 실패 후 원상 복구까지
성공하면 `recovered`로 기록된다. 복구 중에도 문제가 생기면 `recovery_failed`이며
이를 성공으로 보고하지 않는다. 재시도 전 원인과 실제 에셋 상태를 확인해야 한다

적용 중 오류가 나면 시도한 속성의 원래 값을 복원·저장·재로드 검증한다
여러 에셋의 파일 저장은 원자적 트랜잭션이 아니므로, 저장 장치 오류나 에디터 강제
종료까지 완전히 보장하지는 않는다. `applying`이나 `rolling_back` 상태로 남은
계획도 자동 재실행하지 말고 JSON과 실제 값을 대조해 복구한다
중요한 작업 전 Git 커밋은 여전히 권장된다

이미 적용한 값이 이후 다른 값으로 변경되면 `rollback`도 충돌로 중단한다
이 도구의 복구는 에셋 전체 파일을 과거 버전으로 덮어쓰는 기능이 아니다

`verify`는 현재 로드된 속성과 기대값의 일치를 확인한다
`apply`와 `rollback`은 저장 후 패키지 재로드까지 수행하므로 디스크 저장 결과도
확인하지만, 재로드 이후 별도로 실행한 `verify` 자체가 매번 강제 재로드하는 것은 아니다
타입·유한수·실제 대입 결과를 검사하며, 모든 게임 디자인 규칙이나 밸런스 범위를
자동으로 검증하는 것은 아니다

## 프로젝트별 허용 범위

`Config/AssetAutomation.json`의 `allowed_roots`로 수정 가능한 경로를 제한한다
현재는 `/Game/ReflectionLab/`만 허용한다. 한 계획은 최대 64개 속성을 처리한다
다른 프로젝트에서 사용하려면 Python 파일을 옮기고 해당 설정의 경로를 변경한다
프로젝트 경로나 엔진 설치 경로는 도구 코드에 하드코딩하지 않는다

## 검증 방법

엔진 밖의 로직 검사:

```text
python -m unittest discover -s Scripts/Tests -p test_asset_changes.py -v
```

엔진 내부 통합 검사는 `Scripts/Editor/TestAssetChanges.py`를 실행한다
고유 이름의 임시 데이터 에셋만 생성하며, 적용·저장·재로드·복구를 확인하고 삭제한다
실제 게임 데이터 에셋을 테스트 목적으로 수정하지 않는다

MCP 연결 검사는 임시 데이터 에셋에 대해 네 도구를 순서대로 호출해 확인한다
이 구현 작업에서 단위 테스트 13개, 엔진 통합 검사, 열린 에디터에서의 MCP 적용·복구를
검증했다. 임시 테스트 에셋은 제거했으며 검증 계획과 로그는 Saved에 남아 있다

기믹 설정 분리 후 구조체 경로 관련 검사를 추가하여 단위 테스트 17개가 통과했다.
엔진 내부 통합 검사에서 구조체 말단 적용·저장·재로드·복구, 같은 구조체의 미요청 속성 보존,
구형 기믹 속성 경로 거부를 확인했다. 이는 직접 서비스 호출 검사이며 새 MCP 왕복 검사는 아니다.
