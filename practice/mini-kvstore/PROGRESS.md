# 진행 현황 — Mini KV Store

각 단계는 C++ 초보자 기준 ~30분 소요를 목표로 한다.
30분 안에 못 끝내도 괜찮다 — 컴파일 에러 잡고, 문서 찾아보는 시간이 학습이다.
완료하면 체크박스를 체크한다.

---

## 1단계: 디렉토리 구조 만들기 (30분)

**목표**: 폴더와 빈 파일을 전부 만든다. 코드는 아직 안 쓴다.

- [ ] `practice/mini-kvstore/` 아래에 `include/`, `src/` 디렉토리 생성
- [ ] 빈 파일 생성: `src/main.cpp`, `src/store.cpp`, `src/command.cpp`, `src/exporter.cpp`
- [ ] 빈 헤더 생성: `include/store.h`, `include/command.h`, `include/exporter.h`
- [ ] `src/main.cpp`에 "hello"를 출력하고 0을 반환하는 최소한의 코드 작성
- [ ] 나머지 `.cpp` 파일에는 대응하는 `.h`를 `#include`만 해둔다
- [ ] 나머지 `.h` 파일에는 include guard (`#pragma once` 또는 `#ifndef`)만 넣는다

**검증**: 파일이 전부 있는지 `ls`나 `tree`로 확인.

**배우는 것**: C++ 프로젝트의 include/src 관례, include guard의 역할.

---

## 2단계: CMakeLists.txt 직접 작성 + 첫 빌드 (30분)

**목표**: CMake를 처음부터 직접 쓰고 빌드에 성공한다.

- [ ] `CMakeLists.txt` 파일 생성
- [ ] `cmake_minimum_required`로 최소 버전 지정 (3.20)
- [ ] `project()`로 프로젝트 이름과 언어(CXX) 지정
- [ ] `set(CMAKE_CXX_STANDARD 17)`, `set(CMAKE_CXX_STANDARD_REQUIRED ON)` 추가
- [ ] `add_executable()`로 실행 파일 이름과 소스 파일 4개 나열
- [ ] `target_include_directories()`로 `include` 폴더 지정
- [ ] `cmake -B build -S .` 실행 — 에러 나면 고친다
- [ ] `cmake --build build` 실행 — 컴파일 에러 나면 고친다
- [ ] `./build/kvstore` 실행해서 "hello" 출력 확인

**검증**: `./build/kvstore` → "hello" 출력.

**배우는 것**: CMake의 기본 명령어 5개, out-of-source 빌드 개념, `target_include_directories`가 왜 필요한지.

---

## 3단계: KVStore 헤더 선언 (30분)

**목표**: `store.h`에 클래스 선언만 작성한다. 구현은 아직 안 한다.

- [ ] 필요한 표준 헤더 `#include` — `<map>`, `<string>`, `<optional>`
- [ ] `class KVStore` 선언
- [ ] `public`: `explicit` 생성자 (매개변수: `const std::string &path`)
- [ ] `public`: `set(const std::string &key, const std::string &value)` → `void`
- [ ] `public`: `get(const std::string &key)` → `std::optional<std::string>`
- [ ] `public`: `remove(const std::string &key)` → `bool`
- [ ] `public`: `list()` → `const std::map<std::string, std::string>&`
- [ ] `public`: `count()` → `size_t`
- [ ] `public`: `save()` → `void`
- [ ] `private`: `std::map<std::string, std::string> data_`
- [ ] `private`: `std::string path_`
- [ ] `private`: `void load()`
- [ ] 빌드해서 컴파일 에러 없는지 확인

**검증**: `cmake --build build` 성공 (링크 에러는 OK — 구현이 아직 없으니까).

**배우는 것**: 클래스 선언 문법, `public`/`private` 구분, `explicit`이 왜 필요한지, `const &` 매개변수.

---

## 4단계: KVStore 구현 — 메모리에서만 동작 (30분)

**목표**: `store.cpp`에서 각 메서드를 구현한다. 파일 I/O는 아직 빈 함수.

- [ ] `store.cpp`에 `#include "store.h"` 추가
- [ ] 생성자: `path_`에 경로 저장, `load()` 호출
- [ ] `set`: `data_[key] = value;`
- [ ] `get`: `data_.find(key)` → 찾으면 `it->second`, 못 찾으면 `std::nullopt`
- [ ] `remove`: `data_.erase(key)` → 반환값이 1이면 `true`, 0이면 `false`
- [ ] `list`: `return data_;`
- [ ] `count`: `return data_.size();`
- [ ] `load()`: 일단 빈 함수 `{}`
- [ ] `save()`: 일단 빈 함수 `{}`
- [ ] `main.cpp`에서 `KVStore store("store.dat");`를 만들고 `set`/`get`/`list`/`count` 직접 호출, 결과 출력

**검증**: 빌드 + 실행해서 set한 값이 get으로 나오는지, list가 출력되는지, count가 맞는지 확인.

**배우는 것**: `std::map`의 `[]`, `find`, `erase` 사용법, `std::optional` 반환, `ClassName::method` 구현 문법.

---

## 5단계: 파일 저장 — save() (30분)

**목표**: `save()`를 구현해서 데이터를 `store.dat`에 쓴다.

- [ ] `store.cpp` 상단에 `#include <fstream>` 추가
- [ ] `save()` 구현: `std::ofstream`으로 `path_` 파일 열기
- [ ] `data_`를 순회하면서 `key=value` 형태로 한 줄씩 쓰기
- [ ] `set()`과 `remove()` 끝에 `save()` 호출 추가
- [ ] 테스트: 프로그램 실행 후 `cat store.dat`로 파일 내용 확인

**검증**: `./build/kvstore` 실행 후 `store.dat` 파일에 `key=value` 행이 있다.

**배우는 것**: `std::ofstream` 사용법, RAII (스코프 끝에서 파일 자동 닫힘), range-based for로 map 순회.

---

## 6단계: 파일 로드 — load() (30분)

**목표**: `load()`를 구현해서 프로그램 시작 시 기존 데이터를 복원한다.

- [ ] `load()` 구현: `std::ifstream`으로 `path_` 파일 열기
- [ ] 파일이 없으면 (열기 실패) 그냥 return — 첫 실행 시 정상
- [ ] `std::getline`으로 한 줄씩 읽기
- [ ] 각 줄에서 `=`의 위치를 `find()`로 찾기
- [ ] `substr()`로 key와 value 분리해서 `data_`에 저장
- [ ] 테스트: 프로그램 실행 → set → 프로그램 종료 → 다시 실행 → get으로 데이터 확인

**검증**: 프로그램 두 번 실행 — 두 번째에서 첫 번째 데이터가 살아 있다.

**배우는 것**: `std::ifstream`, `std::getline`, `std::string::find`/`substr`로 파싱, 파일 없을 때의 방어 처리.

---

## 7단계: Command 인터페이스 선언 (30분)

**목표**: `command.h`에 추상 클래스 `Command`와 구체 클래스 `SetCommand`, `GetCommand`를 선언한다.

- [ ] `command.h`에 `#include "store.h"` 추가
- [ ] 추상 클래스 `Command` 정의:
  - `virtual ~Command() = default;`
  - `virtual void execute(KVStore &store) = 0;`
- [ ] `SetCommand : public Command` 선언 — 생성자에서 `key`, `value` 받기, `execute` 선언 (`override`)
- [ ] `GetCommand : public Command` 선언 — 생성자에서 `key` 받기, `execute` 선언 (`override`)
- [ ] 빌드해서 컴파일 에러 없는지 확인

**검증**: `cmake --build build` 성공 (링크 에러는 OK).

**배우는 것**: 순수 가상 함수 (`= 0`), 가상 소멸자가 왜 필요한지, `override` 키워드, 추상 클래스를 인스턴스화 할 수 없다는 것.

---

## 8단계: SetCommand, GetCommand 구현 + 검증 (30분)

**목표**: 두 커맨드의 `execute`를 구현하고 `main.cpp`에서 직접 테스트한다.

- [ ] `command.cpp`에 `SetCommand::execute` 구현: `store.set(key_, value_)` 후 `"OK"` 출력
- [ ] `command.cpp`에 `GetCommand::execute` 구현: `store.get(key_)` → 값 있으면 출력, 없으면 에러 메시지
- [ ] `main.cpp`에서 `SetCommand`와 `GetCommand`를 직접 만들어 호출
  ```cpp
  SetCommand set("name", "alice");
  set.execute(store);
  GetCommand get("name");
  get.execute(store);
  ```
- [ ] 빌드 + 실행해서 결과 확인

**검증**: "OK" 출력 후 "alice" 출력.

**배우는 것**: `override`가 있으면 부모의 시그니처와 안 맞을 때 컴파일 에러가 나는 것, 가상 함수 호출 흐름.

---

## 9단계: 나머지 커맨드 추가 (30분)

**목표**: `DeleteCommand`, `ListCommand`, `CountCommand`, `ExportCommand`를 선언 + 구현한다.

- [ ] `command.h`에 4개 클래스 선언 추가
- [ ] `command.cpp`에 각 `execute` 구현:
  - `DeleteCommand`: `store.remove(key_)` → 성공이면 "OK", 실패면 에러
  - `ListCommand`: `store.list()` 순회하면서 `key = value` 출력
  - `CountCommand`: `store.count()` 출력
  - `ExportCommand`: 일단 `std::cout << "TODO" << std::endl;`
- [ ] 빌드 확인

**검증**: `cmake --build build` 성공.

**배우는 것**: 같은 인터페이스를 여러 클래스가 구현하는 반복 연습, 패턴이 손에 익기 시작.

---

## 10단계: parseCommand 팩토리 + main 재작성 (30분)

**목표**: CLI 인자를 파싱해서 커맨드 객체를 생성하는 팩토리 함수 작성. `main.cpp`를 CLI답게 변경.

- [ ] `command.h`에 팩토리 함수 선언: `std::unique_ptr<Command> parseCommand(int argc, char* argv[]);`
- [ ] `command.cpp`에 구현:
  - `argc < 2` → `nullptr` 반환 (또는 에러 출력)
  - `argv[1]`이 "set"이면 `make_unique<SetCommand>(argv[2], argv[3])` (인자 수 체크 포함)
  - 나머지 커맨드도 동일하게
  - 모르는 커맨드 → 에러 출력 후 `nullptr`
- [ ] `main.cpp` 재작성:
  ```cpp
  auto cmd = parseCommand(argc, argv);
  if (!cmd) return 1;
  KVStore store("store.dat");
  cmd->execute(store);
  ```
- [ ] 테스트: `./build/kvstore set name alice` → `./build/kvstore get name`

**검증**: 터미널에서 진짜 CLI처럼 동작한다.

**배우는 것**: Factory 함수, `std::make_unique`, `unique_ptr` 소유권 이전, `argc`/`argv` 사용법.

---

## 11단계: Exporter 인터페이스 + CsvExporter (30분)

**목표**: `exporter.h`에 추상 클래스 정의, `CsvExporter`부터 구현한다 (JSON보다 쉬우니까).

- [ ] `exporter.h`에 추상 클래스 `Exporter` 정의:
  - `virtual ~Exporter() = default;`
  - `virtual void dump(const std::map<std::string, std::string> &data, std::ostream &out) = 0;`
- [ ] `CsvExporter : public Exporter` 선언 + 구현 — 줄마다 `key,value`
- [ ] `exporter.cpp`에 구현
- [ ] `main.cpp`에서 임시로 `CsvExporter`를 직접 만들어 테스트:
  ```cpp
  CsvExporter csv;
  csv.dump(store.list(), std::cout);
  ```
- [ ] 빌드 + 실행

**검증**: key,value 형태로 출력된다.

**배우는 것**: `std::ostream &`으로 출력 대상을 추상화하는 이유 (stdout이든 파일이든 같은 코드).

---

## 12단계: JsonExporter + createExporter 팩토리 (30분)

**목표**: JSON 출력 구현, 팩토리 함수로 포맷 선택, `ExportCommand` 연결.

- [ ] `JsonExporter` 선언 + 구현 — `{"key1":"val1","key2":"val2"}` 형태
  - 힌트: 첫 항목이 아니면 앞에 `,`를 붙인다
- [ ] `createExporter(const std::string &format)` 팩토리 함수 작성
  - "json" → `make_unique<JsonExporter>()`
  - "csv" → `make_unique<CsvExporter>()`
  - 그 외 → `nullptr`
- [ ] `ExportCommand::execute`에서 `createExporter` 사용하도록 수정
- [ ] 테스트: `./build/kvstore export json`, `./build/kvstore export csv`

**검증**: 두 포맷 모두 정상 출력.

**배우는 것**: Strategy 패턴 (같은 인터페이스, 다른 알고리즘), Factory 패턴 두 번째 연습, JSON 문자열 조립.

---

## 13단계: 에러 처리 마무리 + test.sh (30분)

**목표**: 모든 에러 케이스를 점검하고, 테스트 스크립트로 전체 검증.

- [ ] 하나씩 직접 실행해서 확인:
  - `./build/kvstore` → `Error: usage: kvstore <command> [args...]`
  - `./build/kvstore foo` → `Error: unknown command: foo`
  - `./build/kvstore get missing` → `Error: key not found: missing`
  - `./build/kvstore delete missing` → `Error: key not found: missing`
  - `./build/kvstore export xml` → `Error: unknown format: xml`
- [ ] 안 되는 게 있으면 고친다
- [ ] `test.sh` 작성 (SPEC 참고하되, 직접 타이핑)
- [ ] `chmod +x test.sh && ./test.sh` 실행
- [ ] 전부 통과할 때까지 반복
- [ ] 정리: 디버그 출력 제거, 사용하지 않는 include 제거

**검증**: `./test.sh` → "ALL TESTS PASSED"

**배우는 것**: 방어적 프로그래밍, 에러 메시지 일관성, 셸 스크립트 기초 (`set -e`, 변수, `$?`).

---

## 전체: ~6.5시간 (13단계 x 30분)

이틀에 나눠서 해도 좋다.

| 단계 | 만드는 것 | 핵심 개념 |
|------|-----------|-----------|
| 1 | 디렉토리 + 빈 파일 | 프로젝트 구조 관례 |
| 2 | CMakeLists.txt + 첫 빌드 | CMake 기본 명령어 |
| 3 | KVStore 헤더 선언 | 클래스 선언, `const &`, `explicit` |
| 4 | KVStore 구현 (메모리) | `std::map`, `std::optional` |
| 5 | save() | `std::ofstream`, RAII |
| 6 | load() | `std::ifstream`, 문자열 파싱 |
| 7 | Command 인터페이스 선언 | 순수 가상 함수, 가상 소멸자 |
| 8 | Set/Get 커맨드 구현 | `override`, 가상 함수 호출 |
| 9 | 나머지 커맨드 | 패턴 반복 연습 |
| 10 | parseCommand + main | Factory, `unique_ptr`, `argc`/`argv` |
| 11 | Exporter + CSV | `ostream &`, Strategy 패턴 |
| 12 | JSON + 팩토리 | Factory 두 번째 연습 |
| 13 | 에러 처리 + test.sh | 방어적 프로그래밍 |
