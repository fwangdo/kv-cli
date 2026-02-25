# 진행 현황 — Mini KV Store

각 단계는 C++ 초보자 기준 ~30분 소요를 목표로 한다.
완료하면 체크박스를 체크한다.

---

## 1단계: 프로젝트 뼈대 + CMake (30분)

**목표**: 파일 구조를 만들고 빌드가 되는지 확인한다 (내용은 비어 있어도 됨).

- [ ] `CMakeLists.txt` 작성 (SPEC에서 복사)
- [ ] 빈 파일 생성: `src/main.cpp`, `src/store.cpp`, `src/command.cpp`, `src/exporter.cpp`
- [ ] 빈 헤더 생성: `include/store.h`, `include/command.h`, `include/exporter.h`
- [ ] `main.cpp`에 "hello"를 출력하는 최소 코드 작성
- [ ] `cmake -B build -S . && cmake --build build` 로 빌드
- [ ] `./build/kvstore` 실행해서 "hello" 출력 확인

**배우는 것**: CMake 기초, 헤더/소스 파일 구조, 빌드 사이클.

---

## 2단계: KVStore 클래스 — 메모리에서만 동작 (30분)

**목표**: `set`, `get`, `remove`, `list`, `count` 구현. 파일 I/O는 아직 없음.

- [ ] `store.h`에 `class KVStore` 선언 — `std::map<std::string, std::string> data_` 멤버
- [ ] 메서드 선언: `set`, `get` (`std::optional<std::string>` 반환), `remove` (`bool` 반환), `list` (`const std::map&` 반환), `count` (`size_t` 반환)
- [ ] 생성자에 `explicit` 붙이고 `std::string path` 매개변수 받기 (저장만 하고 아직 안 씀)
- [ ] `store.cpp`에서 각 메서드 구현
- [ ] `main.cpp`에서 `set`, `get`, `list`, `count`를 직접 호출하고 결과 출력해서 검증

**배우는 것**: `std::map`, `std::optional`, `const` 참조 반환, `explicit` 생성자, 헤더/구현 분리.

---

## 3단계: 파일 저장/로드 — 영속성 (30분)

**목표**: 프로그램을 다시 실행해도 데이터가 유지된다.

- [ ] `store.cpp`에서 `load()` 구현 — `std::ifstream`으로 파일 열기, `key=value` 행 파싱, `data_`에 저장
- [ ] `store.cpp`에서 `save()` 구현 — `std::ofstream`으로 파일 열기, 각 항목을 `key=value`로 쓰기
- [ ] 생성자에서 `load()` 호출
- [ ] `set`과 `remove` 후에 `save()` 호출
- [ ] 테스트: 프로그램을 두 번 실행 — 두 번째 실행에서 첫 번째 데이터가 남아 있어야 함

**배우는 것**: RAII 파일 I/O (`ifstream`/`ofstream`), `std::getline`, `std::string::find` / `substr`로 파싱.

---

## 4단계: Command 인터페이스 + SetCommand, GetCommand (30분)

**목표**: Command 패턴을 도입하고 2개의 구체 커맨드 구현.

- [ ] `command.h`에 추상 `class Command` 정의 — `virtual ~Command() = default`, `virtual void execute(KVStore &store) = 0`
- [ ] `class SetCommand : public Command` 정의 — `key_`와 `value_` 저장, `execute` 구현
- [ ] `class GetCommand : public Command` 정의 — `key_` 저장, `execute` 구현 (값 출력 또는 에러)
- [ ] `command.cpp`에서 `execute` 메서드 구현
- [ ] `main.cpp`에서 `SetCommand`와 `GetCommand`를 직접 만들어 `execute` 호출하여 검증

**배우는 것**: Virtual 인터페이스, 순수 가상 함수 (`= 0`), `override`, 가상 소멸자, `unique_ptr<Command>`.

---

## 5단계: 나머지 커맨드 + parseCommand 팩토리 (30분)

**목표**: 6개 커맨드 전부 동작. 팩토리 함수로 생성.

- [ ] `DeleteCommand`, `ListCommand`, `CountCommand`, `ExportCommand` 추가 (ExportCommand는 일단 "TODO" 출력)
- [ ] `std::unique_ptr<Command> parseCommand(int argc, char* argv[])` 작성 — `argv[1]`을 읽어서 적절한 커맨드 객체 반환
- [ ] 에러 처리: 인자 없음 → usage 에러, 알 수 없는 커맨드 → 에러 메시지
- [ ] `main.cpp` 재작성: `parseCommand` 호출 후 `cmd->execute(store)`
- [ ] 테스트: `./kvstore set name alice`, `./kvstore get name`, `./kvstore list`, `./kvstore count`, `./kvstore delete name`

**배우는 것**: Factory 함수, `std::make_unique`, `unique_ptr`을 통한 소유권 이전, `argc`/`argv`로 CLI 인자 파싱.

---

## 6단계: Exporter 인터페이스 + JSON/CSV (30분)

**목표**: `export json`과 `export csv`가 동작한다.

- [ ] `exporter.h`에 추상 `class Exporter` 정의 — `virtual void dump(const std::map<std::string, std::string> &data, std::ostream &out) = 0`
- [ ] `JsonExporter` 정의 — `{"key1":"val1","key2":"val2"}` 형태 출력
- [ ] `CsvExporter` 정의 — 줄마다 `key1,val1` 형태 출력
- [ ] `std::unique_ptr<Exporter> createExporter(const std::string &format)` 작성
- [ ] `ExportCommand::execute`에서 `createExporter` 사용하도록 수정
- [ ] 테스트: `./kvstore export json`, `./kvstore export csv`

**배우는 것**: Strategy 패턴 (같은 인터페이스, 다른 출력), `std::ostream &`으로 출력 대상 추상화, Factory 패턴 두 번째 연습.

---

## 7단계: 에러 처리 마무리 + test.sh (30분)

**목표**: 모든 에러 케이스 처리. 전체 테스트 스크립트 통과.

- [ ] 확인: `./kvstore get missing` → `Error: key not found: missing`
- [ ] 확인: `./kvstore delete missing` → `Error: key not found: missing`
- [ ] 확인: `./kvstore export xml` → `Error: unknown format: xml`
- [ ] 확인: `./kvstore` (인자 없음) → `Error: usage: kvstore <command> [args...]`
- [ ] 확인: `./kvstore foo` → `Error: unknown command: foo`
- [ ] `test.sh` 작성 (SPEC에서 복사), 실행, 실패하는 부분 수정
- [ ] 정리: 디버그 출력 제거, 사용하지 않는 include 확인

**배우는 것**: 방어적 프로그래밍, 일관된 에러 출력 형식, 셸 스크립트로 테스트.

---

## 전체: ~3.5시간

| 단계 | 만드는 것 | 핵심 개념 |
|------|-----------|-----------|
| 1 | 뼈대 + CMake | 빌드 시스템 |
| 2 | KVStore (메모리) | `std::map`, `std::optional`, `const&` |
| 3 | 파일 영속성 | RAII, `ifstream`/`ofstream` |
| 4 | Command 베이스 + 2개 커맨드 | Virtual 인터페이스, `override` |
| 5 | 전체 커맨드 + 팩토리 | Factory 패턴, `unique_ptr` |
| 6 | Exporter + JSON/CSV | Strategy 패턴, `ostream&` |
| 7 | 에러 처리 + 테스트 | 방어적 프로그래밍 |
