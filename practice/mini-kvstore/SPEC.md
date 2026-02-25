# Mini Key-Value Store — 설계 명세서

터미널에서 동작하는 작은 key-value 저장소를 만든다.
데이터는 파일에 저장되어 프로그램을 다시 실행해도 유지된다.

---

## 사용 예시

```bash
$ ./kvstore set name alice
OK

$ ./kvstore set age 25
OK

$ ./kvstore get name
alice

$ ./kvstore get missing
Error: key not found: missing

$ ./kvstore list
age = 25
name = alice

$ ./kvstore delete age
OK

$ ./kvstore delete age
Error: key not found: age

$ ./kvstore count
1

$ ./kvstore export json
{"name":"alice"}

$ ./kvstore export csv
name,alice
```

---

## 기능 요구사항

### 커맨드

| 커맨드 | 설명 |
|---|---|
| `set <key> <value>` | 키-값 쌍 저장 (이미 있으면 덮어씀) |
| `get <key>` | 값 조회. 없으면 에러 메시지 |
| `delete <key>` | 삭제. 없으면 에러 메시지 |
| `list` | 모든 키-값을 알파벳 순으로 출력 |
| `count` | 저장된 키 개수 출력 |
| `export <format>` | json 또는 csv로 전체 데이터 출력 |

### 저장소

- 데이터를 파일(`store.dat`)에 저장한다
- 프로그램 시작 시 파일에서 로드, 변경 후 파일에 저장
- 파일 포맷은 자유 (한 줄에 `key=value` 추천)

### 에러 처리

- 알 수 없는 커맨드 → `Error: unknown command: xxx`
- 인자 부족 → `Error: usage: kvstore <command> [args...]`
- 키 없음 → `Error: key not found: xxx`
- 파일 열기 실패 → 빈 저장소로 시작 (첫 실행 시 정상)
- export에 알 수 없는 포맷 → `Error: unknown format: xxx`

---

## 파일 구조

```
mini-kvstore/
├── SPEC.md              ← 이 문서
├── CMakeLists.txt       ← 빌드 설정
├── include/
│   ├── store.h          ← KVStore 클래스 (데이터 저장 + 조회)
│   ├── command.h        ← Command 인터페이스 + 구체 커맨드들
│   └── exporter.h       ← Exporter 인터페이스 + json/csv 구현
├── src/
│   ├── main.cpp         ← CLI 파싱 + 커맨드 디스패치
│   ├── store.cpp        ← KVStore 구현 (파일 I/O 포함)
│   ├── command.cpp      ← 각 커맨드 구현
│   └── exporter.cpp     ← 각 exporter 구현
└── test.sh              ← 수동 테스트 스크립트
```

---

## 클래스 설계

### 1. KVStore (`store.h / store.cpp`)

데이터를 담고 파일에 읽기/쓰기.

```
class KVStore
  public:
    explicit KVStore(파일 경로)     ← 생성자에서 파일 로드
    set(key, value) → void
    get(key) → optional<string>    ← 없으면 nullopt
    remove(key) → bool             ← 있었으면 true
    list() → const map 참조 반환
    count() → size_t
    save() → void                  ← 파일에 쓰기
  private:
    std::map<string, string> data_
    std::string path_
    void load()                    ← 파일에서 읽기
```

**연습 포인트:**
- `explicit` 생성자 (함정 4)
- `const std::map&` 반환 (패턴 5: const 참조)
- `std::optional` 반환 (패턴 6)
- RAII 파일 I/O (패턴 1)
- 헤더/구현 분리 (함정 5)

### 2. Command (`command.h / command.cpp`)

커맨드를 객체로 표현. Virtual interface + Factory.

```
class Command                       ← 인터페이스 (순수 가상)
  public:
    virtual ~Command() = default
    virtual void execute(KVStore &store) = 0

class SetCommand : public Command
class GetCommand : public Command
class DeleteCommand : public Command
class ListCommand : public Command
class CountCommand : public Command
class ExportCommand : public Command

팩토리 함수:
  unique_ptr<Command> parseCommand(argc, argv) → 커맨드 객체
```

**연습 포인트:**
- Virtual interface (패턴 9)
- Factory 함수 (패턴 3)
- unique_ptr 소유권 (패턴 17)
- override 키워드
- 가상 소멸자 (함정 2 방지)

### 3. Exporter (`exporter.h / exporter.cpp`)

출력 포맷을 다형성으로 처리.

```
class Exporter                      ← 인터페이스
  public:
    virtual ~Exporter() = default
    virtual void dump(const map<string,string> &data, ostream &out) = 0

class JsonExporter : public Exporter
class CsvExporter : public Exporter

팩토리 함수:
  unique_ptr<Exporter> createExporter(format_string) → exporter 객체
```

**연습 포인트:**
- Virtual interface (패턴 9) — Command와 같은 구조를 반복 연습
- Factory (패턴 3)
- `const map &`로 데이터 전달 (패턴 5)
- `ostream &`으로 출력 대상 추상화 (stdout이든 파일이든)

---

## 연습할 패턴 체크리스트

| 02c 패턴 | 이 프로젝트에서 어디 |
|---|---|
| 1. RAII | `KVStore` — 파일 열기/닫기가 스코프에 의해 자동 관리 |
| 3. Factory | `parseCommand()`, `createExporter()` |
| 4. enum class | 커맨드 타입을 enum으로 정의해도 됨 (선택) |
| 5. const T& | `KVStore::list()` 반환, `Exporter::dump()` 매개변수 |
| 6. optional | `KVStore::get()` 반환값 |
| 9. Virtual Interface | `Command`, `Exporter` — 두 번 연습 |
| 10. string_view | 커맨드 인자를 `string_view`로 받기 (선택) |
| 12. constexpr | 파일 경로 기본값을 constexpr로 (선택) |
| 15. Aggregate | 설정 struct (선택) |
| 17. 이동 | `unique_ptr<Command>` 반환 및 전달 |
| P1. Dangling ref | `get()`에서 optional 반환 vs 참조 반환 — 왜 optional이 안전한지 |
| P2. Slicing | Command를 값이 아닌 `unique_ptr<Command>`로 다루는 이유 |
| P4. explicit | `KVStore(path)` 생성자 |
| P5. 헤더 분리 | 모든 클래스가 .h/.cpp로 분리 |

---

## CMakeLists.txt 뼈대

```cmake
cmake_minimum_required(VERSION 3.20)
project(MiniKVStore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(kvstore
    src/main.cpp
    src/store.cpp
    src/command.cpp
    src/exporter.cpp
)

target_include_directories(kvstore PRIVATE include)
```

---

## 구현 순서 (추천)

```
1단계: KVStore 클래스
   → store.h / store.cpp 작성
   → 파일 I/O 없이 메모리에서만 동작하게 먼저
   → 검증: main.cpp에서 set/get/list 직접 호출

2단계: 파일 저장/로드
   → load() / save() 구현
   → 검증: 프로그램 재시작 후 데이터 유지

3단계: Command 패턴
   → command.h / command.cpp 작성
   → parseCommand() 팩토리
   → main.cpp를 커맨드 객체 사용으로 리팩토링
   → 검증: ./kvstore set name alice

4단계: Exporter 패턴
   → exporter.h / exporter.cpp 작성
   → ExportCommand가 Exporter를 사용
   → 검증: ./kvstore export json

5단계: 에러 처리 마무리
   → 모든 에러 케이스 처리
   → test.sh로 전체 테스트
```

---

## test.sh 예시

```bash
#!/bin/bash
set -e
BIN=./build/kvstore
rm -f store.dat

echo "=== set ==="
$BIN set name alice
$BIN set age 25
$BIN set city seoul

echo "=== get ==="
$BIN get name        # alice
$BIN get age         # 25

echo "=== list ==="
$BIN list            # age=25, city=seoul, name=alice

echo "=== count ==="
$BIN count           # 3

echo "=== delete ==="
$BIN delete age
$BIN count           # 2

echo "=== export ==="
$BIN export json
$BIN export csv

echo "=== error cases ==="
$BIN get missing     # Error: key not found
$BIN delete missing  # Error: key not found
$BIN export xml      # Error: unknown format
$BIN                 # Error: usage

echo "=== persistence ==="
$BIN get name        # alice (파일에서 로드)

echo "ALL TESTS PASSED"
```
