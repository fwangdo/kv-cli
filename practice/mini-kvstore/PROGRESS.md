# 진행 현황 — Mini KV Store

각 단계는 C++ 초보자 기준 ~30분 소요를 목표로 한다.
30분 안에 못 끝내도 괜찮다 — 컴파일 에러 잡고, 문서 찾아보는 시간이 학습이다.
완료하면 체크박스를 체크한다.

---

## 1단계: 디렉토리 구조 만들기 (30분)

**목표**: 폴더와 빈 파일을 전부 만든다. 코드는 아직 안 쓴다.

- [x] `practice/mini-kvstore/` 아래에 `include/`, `src/` 디렉토리 생성
- [x] 빈 파일 생성: `src/main.cpp`, `src/store.cpp`, `src/command.cpp`, `src/exporter.cpp`
- [x] 빈 헤더 생성: `include/store.h`, `include/command.h`, `include/exporter.h`
- [x] `src/main.cpp`에 "hello"를 출력하고 0을 반환하는 최소한의 코드 작성
- [x] 나머지 `.cpp` 파일에는 대응하는 `.h`를 `#include`만 해둔다
- [x] 나머지 `.h` 파일에는 include guard (`#pragma once` 또는 `#ifndef`)만 넣는다

**검증**: 파일이 전부 있는지 `ls`나 `tree`로 확인.

**배우는 것**: C++ 프로젝트의 include/src 관례, include guard의 역할.

---

## 2단계: CMakeLists.txt 직접 작성 + 첫 빌드 (30분)

**목표**: CMake를 처음부터 직접 쓰고 빌드에 성공한다.

- [x] `CMakeLists.txt` 파일 생성
- [x] `cmake_minimum_required`로 최소 버전 지정 (3.20)
- [x] `project()`로 프로젝트 이름과 언어(CXX) 지정
- [x] `set(CMAKE_CXX_STANDARD 17)`, `set(CMAKE_CXX_STANDARD_REQUIRED ON)` 추가
- [x] `add_executable()`로 실행 파일 이름과 소스 파일 4개 나열
- [x] `target_include_directories()`로 `include` 폴더 지정
- [x] `cmake -B build -S .` 실행 — 에러 나면 고친다
- [x] `cmake --build build` 실행 — 컴파일 에러 나면 고친다
- [x] `./build/kvstore` 실행해서 "hello" 출력 확인

**검증**: `./build/kvstore` → "hello" 출력.

**배우는 것**: CMake의 기본 명령어 5개, out-of-source 빌드 개념, `target_include_directories`가 왜 필요한지.

---

## 3단계: KVStore 헤더 선언 (30분)

**목표**: `store.h`에 클래스 선언만 작성한다. 구현은 아직 안 한다.

- [x] 필요한 표준 헤더 `#include` — `<map>`, `<string>`, `<optional>`
- [x] `class KVStore` 선언
- [x] `public`: `explicit` 생성자 (매개변수: `const std::string &path`)
- [x] `public`: `set(const std::string &key, const std::string &value)` → `void`
- [x] `public`: `get(const std::string &key)` → `std::optional<std::string>`
- [x] `public`: `remove(const std::string &key)` → `bool`
- [x] `public`: `list()` → `const std::map<std::string, std::string>&`
- [x] `public`: `count()` → `size_t`
- [x] `public`: `save()` → `void`
- [x] `private`: `std::map<std::string, std::string> data_`
- [x] `private`: `std::string path_`
- [x] `private`: `void load()`
- [x] 빌드해서 컴파일 에러 없는지 확인

**검증**: `cmake --build build` 성공 (링크 에러는 OK — 구현이 아직 없으니까).

**배우는 것**: 클래스 선언 문법, `public`/`private` 구분, `explicit`이 왜 필요한지, `const &` 매개변수.

---

## 4단계: KVStore 구현 — 메모리에서만 동작 (30분)

**목표**: `store.cpp`에서 각 메서드를 구현한다. 파일 I/O는 아직 빈 함수.

- [x] `store.cpp`에 `#include "store.h"` 추가
- [x] 생성자: `path_`에 경로 저장, `load()` 호출
- [x] `set`: `data_[key] = value;`
- [x] `get`: `data_.find(key)` → 찾으면 `it->second`, 못 찾으면 `std::nullopt`
- [x] `remove`: `data_.erase(key)` → 반환값이 1이면 `true`, 0이면 `false`
- [x] `list`: `return data_;`
- [x] `count`: `return data_.size();`
- [x] `load()`: 일단 빈 함수 `{}`
- [x] `save()`: 일단 빈 함수 `{}`
- [x] `main.cpp`에서 `KVStore store("store.dat");`를 만들고 `set`/`get`/`list`/`count` 직접 호출, 결과 출력

**검증**: 빌드 + 실행해서 set한 값이 get으로 나오는지, list가 출력되는지, count가 맞는지 확인.

**배우는 것**: `std::map`의 `[]`, `find`, `erase` 사용법, `std::optional` 반환, `ClassName::method` 구현 문법.

---

## 5단계: 파일 저장 — save() (30분)

**목표**: `save()`를 구현해서 데이터를 `store.dat`에 쓴다.

- [x] `store.cpp` 상단에 `#include <fstream>` 추가
- [x] `save()` 구현: `std::ofstream`으로 `path_` 파일 열기
- [x] `data_`를 순회하면서 `key=value` 형태로 한 줄씩 쓰기
- [x] `set()`과 `remove()` 끝에 `save()` 호출 추가
- [x] 테스트: 프로그램 실행 후 `cat store.dat`로 파일 내용 확인

**검증**: `./build/kvstore` 실행 후 `store.dat` 파일에 `key=value` 행이 있다.

**배우는 것**: `std::ofstream` 사용법, RAII (스코프 끝에서 파일 자동 닫힘), range-based for로 map 순회.

---

## 6단계: 파일 로드 — load() (30분)

**목표**: `load()`를 구현해서 프로그램 시작 시 기존 데이터를 복원한다.

- [x] `load()` 구현: `std::ifstream`으로 `path_` 파일 열기
- [x] 파일이 없으면 (열기 실패) 그냥 return — 첫 실행 시 정상
- [x] `std::getline`으로 한 줄씩 읽기
- [x] 각 줄에서 `=`의 위치를 `find()`로 찾기
- [x] `substr()`로 key와 value 분리해서 `data_`에 저장
- [x] 테스트: 프로그램 실행 → set → 프로그램 종료 → 다시 실행 → get으로 데이터 확인

**검증**: 프로그램 두 번 실행 — 두 번째에서 첫 번째 데이터가 살아 있다.

**배우는 것**: `std::ifstream`, `std::getline`, `std::string::find`/`substr`로 파싱, 파일 없을 때의 방어 처리.

---

## 7단계: Command 인터페이스 선언 (30분)

**목표**: `command.h`에 추상 클래스 `Command`와 구체 클래스 `SetCommand`, `GetCommand`를 선언한다.

- [x] `command.h`에 `#include "store.h"` 추가
- [x] 추상 클래스 `Command` 정의:
  - `virtual ~Command() = default;`
  - `virtual void execute(KVStore &store) = 0;`
- [x] `SetCommand : public Command` 선언 — 생성자에서 `key`, `value` 받기, `execute` 선언 (`override`)
- [x] `GetCommand : public Command` 선언 — 생성자에서 `key` 받기, `execute` 선언 (`override`)
- [x] 빌드해서 컴파일 에러 없는지 확인

**검증**: `cmake --build build` 성공 (링크 에러는 OK).

**배우는 것**: 순수 가상 함수 (`= 0`), 가상 소멸자가 왜 필요한지, `override` 키워드, 추상 클래스를 인스턴스화 할 수 없다는 것.

---

## 8단계: SetCommand, GetCommand 구현 + 검증 (30분)

**목표**: 두 커맨드의 `execute`를 구현하고 `main.cpp`에서 직접 테스트한다.

- [x] `command.cpp`에 `SetCommand::execute` 구현: `store.set(key_, value_)` 후 `"OK"` 출력
- [x] `command.cpp`에 `GetCommand::execute` 구현: `store.get(key_)` → 값 있으면 출력, 없으면 에러 메시지
- [x] `main.cpp`에서 `SetCommand`와 `GetCommand`를 직접 만들어 호출
  ```cpp
  SetCommand set("name", "alice");
  set.execute(store);
  GetCommand get("name");
  get.execute(store);
  ```
- [x] 빌드 + 실행해서 결과 확인

**검증**: "OK" 출력 후 "alice" 출력.

**배우는 것**: `override`가 있으면 부모의 시그니처와 안 맞을 때 컴파일 에러가 나는 것, 가상 함수 호출 흐름.

---

## 9단계: 나머지 커맨드 추가 (30분)

**목표**: `DeleteCommand`, `ListCommand`, `CountCommand`, `ExportCommand`를 선언 + 구현한다.

- [x] `command.h`에 4개 클래스 선언 추가
- [x] `command.cpp`에 각 `execute` 구현:
  - `DeleteCommand`: `store.remove(key_)` → 성공이면 "OK", 실패면 에러
  - `ListCommand`: `store.list()` 순회하면서 `key = value` 출력
  - `CountCommand`: `store.count()` 출력
  - `ExportCommand`: 일단 `std::cout << "TODO" << std::endl;`
- [x] 빌드 확인

**검증**: `cmake --build build` 성공.

**배우는 것**: 같은 인터페이스를 여러 클래스가 구현하는 반복 연습, 패턴이 손에 익기 시작.

---

## 10단계: parseCommand 팩토리 + main 재작성 (30분)

**목표**: CLI 인자를 파싱해서 커맨드 객체를 생성하는 팩토리 함수 작성. `main.cpp`를 CLI답게 변경.

- [x] `command.h`에 팩토리 함수 선언: `std::unique_ptr<Command> parseCommand(int argc, char* argv[]);`
- [x] `command.cpp`에 구현:
  - `argc < 2` → `nullptr` 반환 (또는 에러 출력)
  - `argv[1]`이 "set"이면 `make_unique<SetCommand>(argv[2], argv[3])` (인자 수 체크 포함)
  - 나머지 커맨드도 동일하게
  - 모르는 커맨드 → 에러 출력 후 `nullptr`
- [x] `main.cpp` 재작성:
  ```cpp
  auto cmd = parseCommand(argc, argv);
  if (!cmd) return 1;
  KVStore store("store.dat");
  cmd->execute(store);
  ```
- [x] 테스트: `./build/kvstore set name alice` → `./build/kvstore get name`

**검증**: 터미널에서 진짜 CLI처럼 동작한다.

**배우는 것**: Factory 함수, `std::make_unique`, `unique_ptr` 소유권 이전, `argc`/`argv` 사용법.

---

## 11단계: Exporter 인터페이스 + CsvExporter (30분)

**목표**: `exporter.h`에 추상 클래스 정의, `CsvExporter`부터 구현한다 (JSON보다 쉬우니까).

- [x] 먼저 목표를 이해한다
  - 아직 `export` CLI를 완성하는 단계가 아니다
  - 지금은 "출력 형식을 담당하는 객체"를 따로 만들고, CSV 한 가지만 먼저 구현하는 단계다
  - 즉 `KVStore`는 데이터를 보관만 하고, "어떻게 출력할지"는 `Exporter`가 맡는다
- [ ] `exporter.h`에 필요한 헤더 추가
  - `<map>`, `<memory>`, `<ostream>`, `<string>`
- [ ] `exporter.h`에 추상 클래스 `Exporter` 정의:
  ```cpp
  class Exporter {
  public:
      virtual ~Exporter() = default;
      virtual void dump(
          const std::map<std::string, std::string>& data,
          std::ostream& out) = 0;
  };
  ```
- [ ] 같은 헤더에 `CsvExporter : public Exporter` 선언
  ```cpp
  class CsvExporter : public Exporter {
  public:
      void dump(
          const std::map<std::string, std::string>& data,
          std::ostream& out) override;
  };
  ```
- [ ] `exporter.cpp`에서 `CsvExporter::dump()` 구현
  - `for (const auto& [key, value] : data)`로 순회
  - 각 줄을 `out << key << ',' << value << '\n';`로 출력
- [ ] `main.cpp`에서 임시 테스트 코드 추가
  ```cpp
  kv::KVStore store("store.dat");
  store.set("name", "alice");
  store.set("city", "seoul");

  kv::CsvExporter csv;
  csv.dump(store.list(), std::cout);
  ```
- [ ] 출력 결과가 정말 CSV처럼 나오는지 눈으로 확인
  ```text
  city,seoul
  name,alice
  ```
- [ ] 빌드 + 실행

**왜 이 단계가 필요한가**: 12단계에서 JSON을 추가할 때 `KVStore`나 `main`을 크게 건드리지 않고, `Exporter` 구현만 하나 더 만들기 위해서다.

**검증**: key,value 형태로 출력된다.

**배우는 것**: 인터페이스 분리, `std::ostream &`으로 출력 대상을 추상화하는 이유 (stdout이든 파일이든 같은 코드), "데이터 보관"과 "출력 형식"의 책임 분리.

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

## 14단계: Template 함수 하나 써보기 (20분)

**목표**: 이 프로젝트 흐름을 깨지 않으면서, 중복 출력 로직을 템플릿 함수로 빼 보는 연습을 한다.

- [ ] `main.cpp` 또는 별도 헤더에 템플릿 함수 추가
  ```cpp
  template <typename MapLike>
  void printEntries(const MapLike& data) {
      for (const auto& [key, value] : data) {
          std::cout << key << " = " << value << '\n';
      }
  }
  ```
- [ ] `store.list()`를 직접 순회하던 코드를 `printEntries(store.list())`로 바꿔 본다
- [ ] 빌드해서 템플릿 함수가 실제로 인스턴스화되는지 확인한다
- [ ] 생각해 보기: 왜 템플릿 구현은 보통 `.cpp`보다 헤더에 두는가?

**검증**: `printEntries(store.list())` 호출 시 기존과 동일하게 `key = value`가 출력된다.

**배우는 것**: 함수 템플릿 기본 문법, 타입을 나중에 결정하는 방식, 왜 템플릿 구현이 헤더에 자주 들어가는지.

**이 예제가 맞는 이유**: `std::map`뿐 아니라 나중에 비슷한 key/value 컨테이너에도 같은 출력 함수를 재사용할 수 있다.

---

## 15단계: `constexpr`로 상수 다루기 (15분)

**목표**: 문자열 리터럴과 매직 값을 코드 여기저기에 흩뿌리지 않고, 컴파일 타임 상수로 정리하는 습관을 익힌다.

- [ ] `main.cpp` 또는 적절한 헤더에 기본 저장 파일명을 `constexpr`로 선언
  ```cpp
  constexpr const char* kDefaultStorePath = "store.dat";
  ```
- [ ] `KVStore store("store.dat");`처럼 직접 쓰던 문자열을 `kDefaultStorePath`로 바꿔 본다
- [ ] 테스트용 파일명도 `constexpr` 상수로 빼 본다
- [ ] 생각해 보기: 왜 이런 값은 `std::string` 변수보다 `constexpr` 상수가 더 어울리는가?

**검증**: 빌드와 실행 결과는 그대로이고, 파일 경로 문자열이 한 곳에서 관리된다.

**배우는 것**: 컴파일 타임 상수, 매직 문자열 제거, 의미 있는 이름으로 상수 표현하기.

---

## 16단계: RAII와 Move Semantics 손으로 확인하기 (25분)

**목표**: 파일 스트림에서 이미 쓰고 있는 RAII 개념을 `std::unique_ptr`와 연결해서 이해하고, move가 왜 필요한지 직접 확인한다.

- [ ] `main.cpp`에 `std::unique_ptr<kv::Command>` 하나를 만든다
  ```cpp
  auto cmd = std::make_unique<kv::SetCommand>("name", "alice");
  ```
- [ ] `cmd->execute(store);`를 호출해 본다
- [ ] 같은 포인터를 다른 변수로 옮겨 본다
  ```cpp
  auto moved = std::move(cmd);
  ```
- [ ] move 후 `cmd`는 비어 있고, `moved`만 소유권을 가진다는 것을 출력으로 확인한다
- [ ] 생각해 보기: 왜 `unique_ptr`는 복사(copy)보다 이동(move)만 허용하는가?

**검증**: `std::move` 후 원래 포인터는 비고, 새 포인터로만 `execute()`를 호출할 수 있다.

**배우는 것**: RAII를 사용자 수준 소유권 관리에 적용하는 방법, move semantics가 필요한 이유, `unique_ptr`가 왜 안전한지.

**연결 포인트**: `ifstream`/`ofstream`도 같은 RAII 철학으로 스코프가 끝나면 자동 정리된다.

---

## 17단계: `vector`와 다형성 함께 쓰기 (25분)

**목표**: 여러 종류의 `Command` 객체를 한 컨테이너에 담고, 공통 인터페이스로 순회 실행하는 현대 C++ 패턴을 익힌다.

- [ ] `std::vector<std::unique_ptr<Command>>`를 선언한다
- [ ] `SetCommand`, `GetCommand`, `DeleteCommand` 같은 서로 다른 타입을 `push_back(std::make_unique<...>())`로 넣어 본다
- [ ] `for (const auto& command : commands)`로 순회하면서 `command->execute(store);`를 호출한다
- [ ] 생각해 보기: 왜 `std::vector<Command>`가 아니라 `std::vector<std::unique_ptr<Command>>`를 써야 하는가?
- [ ] 생각해 보기: 왜 이 구조에서 virtual destructor가 필요한가?

**검증**: 서로 다른 커맨드 타입이 하나의 `vector` 안에 담기고, 순회 실행 시 올바른 `execute()`가 호출된다.

**배우는 것**: 컨테이너 + 다형성 조합, object slicing 회피, 소유권을 가진 포인터 컨테이너 패턴.

**힌트**: 지금 `main.cpp`의 테스트 코드는 이 패턴의 아주 작은 예시다. 이걸 더 확장해서 `DeleteCommand`, `CountCommand`까지 넣어 보면 좋다.

---

## 18단계: CMakeLists.txt 한 번 더 다듬기 (25분)

**목표**: 빌드는 이미 되더라도, 타깃 분리와 include 전파를 더 정확하게 이해하도록 CMakeLists를 개선한다.

- [ ] 실행 파일과 라이브러리 타깃을 다시 분리해 본다
  - `main.cpp`만 `add_executable(kv_cli ...)`
  - 나머지 `store.cpp`, `command.cpp`, `exporter.cpp`는 `add_library(kv_core ...)`
- [ ] `target_link_libraries(kv_cli PRIVATE kv_core)`로 연결한다
- [ ] `target_include_directories(kv_core PUBLIC include)`로 헤더 경로를 전파한다
- [ ] 생각해 보기: 왜 `kv_cli`에 include 디렉토리를 직접 주지 않아도 `main.cpp`가 헤더를 찾을 수 있는가?
- [ ] `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)`을 켜고, 왜 IDE/clangd에 도움이 되는지 확인한다
- [ ] 가능하면 `target_compile_features(kv_core PUBLIC cxx_std_17)`도 써 보고, 전역 `CMAKE_CXX_STANDARD`와 차이를 생각해 본다

**검증**: `cmake -B build -S .`와 `cmake --build build`가 여전히 성공하고, `main.cpp`에서 `#include "store.hpp"`가 그대로 동작한다.

**배우는 것**: 타깃 기반 CMake, `PUBLIC` include 전파, `target_link_libraries()`의 의미, IDE가 CMake 정보를 어떻게 활용하는지.

---

## 전체: ~8.5시간 (18단계, 마지막 5단계는 짧은 확장 연습)

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
| 14 | Template 출력 함수 | 함수 템플릿, 헤더에 구현 두기 |
| 15 | `constexpr` 상수 정리 | 컴파일 타임 상수, 매직 값 제거 |
| 16 | RAII + move semantics | `unique_ptr`, 소유권 이동, 자동 정리 |
| 17 | `vector` + 다형성 | object slicing 회피, 컨테이너 + 가상 함수 |
| 18 | CMakeLists 다듬기 | 타깃 분리, include 전파, CMake 실전 감각 |
