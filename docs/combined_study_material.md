# 통합 학습 자료 (Combined Study Material)

이 문서는 docs/ 디렉토리의 모든 학습 자료를 하나로 합친 것입니다.

---

# 1장. LLVM/MLIR 빌드 시스템

## 왜 이걸 알아야 하는가

MLIR 프로젝트를 빌드하려면 먼저 LLVM/MLIR을 소스에서 빌드해야 한다.
이 과정에서 CMake의 핵심 개념을 이해하지 않으면 에러 메시지를 해석할 수 없다.

---

## 1.1 LLVM 빌드의 전체 흐름

```
소스코드 다운로드 → CMake 설정(configure) → 빌드(compile) → 설치(install)
```

### Step 1: 소스 받기

```bash
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
git checkout llvmorg-18.1.0   # 특정 버전 고정 (중요!)
```

**왜 버전을 고정하는가?**
LLVM은 매우 빠르게 변한다. main 브랜치를 쓰면 API가 바뀌어서
내 코드가 갑자기 안 되는 경우가 생긴다. release 태그를 쓰면 안정적이다.

### Step 2: CMake 설정 (Configure)

```bash
cmake -S llvm -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=$HOME/llvm-install \
  -DLLVM_ENABLE_PROJECTS=mlir \
  -DLLVM_TARGETS_TO_BUILD=host \
  -DLLVM_ENABLE_ASSERTIONS=ON
```

각 옵션의 의미:

| 옵션 | 의미 | 왜 필요한가 |
|------|------|------------|
| `-S llvm` | 소스 디렉토리 (CMakeLists.txt가 있는 곳) | LLVM의 최상위 CMakeLists는 `llvm/` 안에 있다 |
| `-B build` | 빌드 디렉토리 (빌드 결과물이 생기는 곳) | 소스와 빌드를 분리 (out-of-source build) |
| `-DCMAKE_BUILD_TYPE=Release` | 최적화 빌드 | Debug로 하면 빌드 결과물이 10GB+ 되고 느리다 |
| `-DCMAKE_INSTALL_PREFIX=...` | 설치 경로 | `make install`하면 여기에 헤더/라이브러리가 복사됨 |
| `-DLLVM_ENABLE_PROJECTS=mlir` | MLIR 서브프로젝트도 같이 빌드 | 기본값은 LLVM만 빌드. MLIR도 필요하다고 명시 |
| `-DLLVM_TARGETS_TO_BUILD=host` | 현재 CPU 아키텍처만 빌드 | 모든 타겟 빌드하면 시간이 몇 배 더 걸린다 |
| `-DLLVM_ENABLE_ASSERTIONS=ON` | assert 문 활성화 | 디버깅할 때 에러 위치를 알려줌 |

### Step 3: 빌드

```bash
cmake --build build -j$(nproc)
```

- `--build build`: build 디렉토리에서 빌드 실행
- `-j$(nproc)`: CPU 코어 수만큼 병렬 빌드 (Mac에서는 `-j$(sysctl -n hw.ncpu)`)
- 이 단계에서 실제로 C++ 컴파일이 일어난다. 30분~2시간 소요.

### Step 4: 설치

```bash
cmake --install build
```

- `CMAKE_INSTALL_PREFIX`에 지정한 경로(`~/llvm-install`)에 복사
- 설치 후 구조:

```
~/llvm-install/
├── bin/            # mlir-opt, mlir-tblgen, llc 등 실행파일
├── include/        # 헤더 파일 (mlir/IR/..., llvm/Support/...)
└── lib/
    ├── libMLIRIR.a       # 정적 라이브러리
    ├── libLLVMSupport.a
    └── cmake/
        ├── llvm/
        │   ├── LLVMConfig.cmake    # find_package(LLVM)이 찾는 파일
        │   └── AddLLVM.cmake       # include(AddLLVM)이 로드하는 파일
        └── mlir/
            ├── MLIRConfig.cmake    # find_package(MLIR)이 찾는 파일
            └── AddMLIR.cmake       # include(AddMLIR)이 로드하는 파일
```

---

## 1.2 CMake의 핵심 원리

CMake는 **빌드 시스템 생성기**이다. 직접 컴파일하지 않는다.

```
CMakeLists.txt → cmake → Makefile (또는 Ninja) → make → 바이너리
```

### CMake가 하는 3가지

1. **소스 파일 찾기**: 어떤 `.cpp`를 컴파일할지 결정
2. **의존성 해결**: 헤더 경로, 라이브러리 경로 설정
3. **빌드 규칙 생성**: Makefile이나 build.ninja 파일 생성

### 핵심 명령어 5개

```cmake
# 1. 프로젝트 선언
project(GaweeMLIR LANGUAGES CXX)

# 2. 라이브러리 만들기 (소스 → .a 파일)
add_library(GaweeDialect lib/Gawee/GaweeDialect.cpp)

# 3. 실행파일 만들기 (소스 → 바이너리)
add_executable(gawee-opt tools/gawee-opt.cpp)

# 4. 의존성 선언 ("A는 B에 의존한다")
target_link_libraries(gawee-opt PRIVATE GaweeDialect MLIRIR)

# 5. 외부 패키지 찾기
find_package(MLIR REQUIRED CONFIG)
```

---

## 1.3 find_package 동작 원리

```cmake
find_package(MLIR REQUIRED CONFIG)
```

이 한 줄이 하는 일:

```
1. -DMLIR_DIR=~/llvm-install/lib/cmake/mlir 로 지정된 경로에서
2. MLIRConfig.cmake 파일을 찾아서
3. 그 안에 정의된 변수들을 현재 CMake에 로드:
   - MLIR_INCLUDE_DIRS = ~/llvm-install/include
   - MLIR_CMAKE_DIR = ~/llvm-install/lib/cmake/mlir
   - LLVM_DIR = ~/llvm-install/lib/cmake/llvm
   - ...
```

**비유**: Python의 `import`와 비슷하다.
`import mlir`을 하면 mlir 패키지의 모든 함수를 쓸 수 있듯이,
`find_package(MLIR)`을 하면 MLIR의 모든 경로 정보를 쓸 수 있다.

---

## 1.4 include() — CMake 모듈 로드

```cmake
include(AddLLVM)
```

이것은 `CMAKE_MODULE_PATH`에 등록된 경로에서 `AddLLVM.cmake`를 찾아서 로드한다.

```
실제 파일: ~/llvm-install/lib/cmake/llvm/AddLLVM.cmake
```

Python으로 비유:
```python
sys.path.append("~/llvm-install/lib/cmake/llvm")  # list(APPEND CMAKE_MODULE_PATH ...)
from AddLLVM import *                              # include(AddLLVM)
```

---

## 1.5 PUBLIC vs PRIVATE

```cmake
target_link_libraries(GaweeDialect PUBLIC MLIRIR)
target_link_libraries(gawee-opt PRIVATE GaweeDialect)
```

| | PUBLIC | PRIVATE |
|---|--------|---------|
| 의미 | "나도 쓰고, 나를 쓰는 사람도 씀" | "나만 씀" |
| 전파 | GaweeDialect에 링크하는 모든 타겟이 자동으로 MLIRIR도 링크 | gawee-opt만 GaweeDialect를 쓰고, 전파 안 됨 |
| 언제 | 라이브러리에 사용 (다른 곳에서 링크할 때) | 최종 실행파일에 사용 (다른 곳에서 링크 안 할 때) |

---

## 암기 정리표

```
┌─────────────────────────────────────────────────────┐
│ LLVM 빌드 4단계                                      │
│   clone → cmake configure → cmake build → install   │
├─────────────────────────────────────────────────────┤
│ CMake 핵심 5개 명령어                                 │
│   project / add_library / add_executable            │
│   target_link_libraries / find_package              │
├─────────────────────────────────────────────────────┤
│ find_package(MLIR)                                  │
│   → MLIRConfig.cmake를 찾아서 경로 변수 로드         │
│   → -DMLIR_DIR=... 로 위치 지정                     │
├─────────────────────────────────────────────────────┤
│ include(AddLLVM)                                    │
│   → AddLLVM.cmake 파일을 로드                        │
│   → llvm_map_components_to_libnames() 등 사용 가능   │
├─────────────────────────────────────────────────────┤
│ PUBLIC vs PRIVATE                                   │
│   PUBLIC  = 나도 쓰고 + 나를 링크하는 사람도 자동 전파  │
│   PRIVATE = 나만 쓰고 + 전파 안 함                    │
├─────────────────────────────────────────────────────┤
│ 빌드 에러 디버깅                                      │
│   "undefined reference" → target_link_libraries 누락 │
│   "file not found"      → include_directories 누락   │
│   "RTTI mismatch"       → -fno-rtti 빠짐             │
└─────────────────────────────────────────────────────┘
```

---

# 2장. C++ 핵심 개념 — 설계 판단을 위한 깊은 이해

문법이 아니라 **"왜 이걸 쓰는가, 언제 다른 걸 쓰는가"**를 다룬다.
MLIR 코드를 읽고 쓸 때 마주치는 핵심 설계 결정을 정리한다.

---

## 2.1 Template vs Virtual — 정적 다형성 vs 동적 다형성

### 원리

C++에서 "다른 타입에 같은 인터페이스를 적용"하는 방법이 두 가지 있다.

| | Virtual (동적) | Template (정적) |
|---|---|---|
| 결정 시점 | 런타임 (vtable 조회) | 컴파일 타임 (코드 생성) |
| 오버헤드 | 함수 포인터 간접 호출 | 없음 (인라인 가능) |
| 단점 | 매 호출마다 간접 비용 | 코드 팽창 (타입마다 복사본) |
| 유연성 | 런타임에 타입 결정 가능 | 컴파일 타임에 타입이 확정되어야 함 |

### Virtual — 런타임 디스패치

```cpp
// 부모가 인터페이스 정의
class Animal {
  virtual void speak() = 0;  // 순수 가상 함수 — 자식이 반드시 구현
};

// 자식이 구현
class Dog : public Animal {
  void speak() override { printf("멍"); }
};
class Cat : public Animal {
  void speak() override { printf("야옹"); }
};

// 런타임에 어떤 자식인지 모르지만 호출 가능
void makeNoise(Animal *a) { a->speak(); }  // vtable에서 함수 포인터를 찾아서 호출
```

**vtable**: 각 클래스마다 가상 함수 포인터 배열이 존재한다.
`a->speak()` 호출 시 vtable에서 실제 함수 주소를 찾아 점프한다.
→ 간접 호출 비용 (branch prediction miss 가능)

### Template — 컴파일 타임 코드 생성

```cpp
template <typename T>
void process(T &item) {
  item.speak();  // T에 speak()이 있어야 컴파일됨
}

process(dog);  // Dog 전용 코드 생성
process(cat);  // Cat 전용 코드 생성
```

컴파일러가 각 타입별로 별도의 함수를 만든다.
→ 간접 호출 없음, 인라인 가능 → 빠름
→ 하지만 타입마다 코드 복사 → 바이너리 크기 증가

### Gawee에서의 사용

```cpp
// ★ CRTP (Template): PassWrapper — 컴파일 타임에 자식 타입을 앎
struct GaweeToLinalgPass
    : public PassWrapper<GaweeToLinalgPass, OperationPass<ModuleOp>> {
  //                     ^^^^^^^^^^^^^^^^^
  //  자기 자신의 타입을 부모에게 전달 → 부모가 컴파일 타임에 자식을 앎
};

// ★ Virtual: matchAndRewrite — 런타임에 어떤 패턴인지 프레임워크가 결정
struct ConvOpLowering : public OpConversionPattern<gawee::ConvOp> {
  LogicalResult matchAndRewrite(...) const override {
    //                                     ^^^^^^^^ virtual 함수 재정의
  }
};
```

**OpConversionPattern은 둘 다 사용한다:**
- Template: `<gawee::ConvOp>` — 컴파일 타임에 어떤 Op을 처리하는지 결정
- Virtual: `matchAndRewrite` — 런타임에 프레임워크가 패턴을 호출

### 판단 기준

```
타입이 컴파일 타임에 확정되는가?
├── YES → Template (성능 우선)
│   예: PassWrapper, OpConversionPattern의 타입 파라미터
└── NO → Virtual (유연성 우선)
    예: matchAndRewrite (프레임워크가 런타임에 패턴 선택)
```

---

## 2.2 포인터 총정리 — Raw, Smart, Reference

### 원리: 소유권(Ownership)의 문제

C++에서 가장 중요한 질문: **"이 메모리를 누가 해제하는가?"**

| 종류 | 표기 | 소유권 | Nullable | 자동 해제 |
|---|---|---|---|---|
| Raw pointer | `T*` | 없음 (누군가 다른 곳에서 관리) | O | X |
| `unique_ptr<T>` | `unique_ptr<T>` | 단독 소유 | O | O |
| `shared_ptr<T>` | `shared_ptr<T>` | 공유 소유 (참조 카운팅) | O | O (카운트=0일 때) |
| Reference | `T&` | 없음 (별명일 뿐) | X | X |

### Raw Pointer (`T*`) — 빌려 쓰는 포인터

```cpp
// MLIREmitter.cpp — JSON 파싱에서
const auto *node = nodeVal.getAsObject();   // 소유하지 않음, 그냥 들여다봄
const auto *attrs = node->getObject("attrs");
if (!attrs) continue;  // nullptr 체크 필수!
```

**용도**: 소유하지 않고 참조만 할 때. JSON 객체는 `graph`가 소유하고, 우리는 포인터로 들여다본다.

### `unique_ptr<T>` — 단독 소유

```cpp
// MLIREmitter.h
std::unique_ptr<OpBuilder> builder;

// MLIREmitter.cpp — 생성
builder = std::make_unique<OpBuilder>(ctx);

// Pass 생성 — 소유권 이전
std::unique_ptr<Pass> createGaweeToLinalgPass() {
  return std::make_unique<GaweeToLinalgPass>();
  // 호출자에게 소유권을 넘긴다
}
```

**핵심 규칙:**
- 복사 불가: `auto b2 = builder;` → 컴파일 에러
- 이동만 가능: `auto b2 = std::move(builder);`
- 스코프 끝에서 자동 delete

### `shared_ptr<T>` — 공유 소유 (Gawee에서는 거의 안 씀)

여러 곳에서 같은 객체를 소유해야 할 때. 참조 카운터가 0이 되면 자동 해제.
MLIR/LLVM은 성능상 이유로 가급적 사용하지 않는다.

### Reference (`T&`) — null 불가능한 별명

```cpp
// GaweeToLinalg.cpp — rewriter는 항상 유효
LogicalResult matchAndRewrite(gawee::ConvOp op, OpAdaptor adaptor,
                              ConversionPatternRewriter &rewriter) const override {
  //                          ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  //  참조: null일 수 없고, 항상 유효한 rewriter를 가리킴
}
```

### MLIR 특수 타입

```cpp
Value input = adaptor.getInput();    // Value — 경량 핸들 (포인터 크기)
                                     // 값 복사가 저렴하므로 참조로 안 넘겨도 됨

OwningOpRef<ModuleOp> module;        // unique_ptr과 비슷 — ModuleOp의 소유권
                                     // 스코프 끝에서 IR 트리 전체가 해제됨
```

### 암기표

```
┌──────────────────────────────────────────────────────┐
│ T*            빌려 쓴다. null 가능. 해제 책임 없음.     │
│ unique_ptr<T> 나만 소유. 스코프 끝에서 자동 해제.      │
│ shared_ptr<T> 여럿이 소유. 카운트=0이면 해제.          │
│ T&            별명. null 불가. 복사 없이 원본 접근.     │
│ const T&      읽기 전용 별명. 가장 안전한 매개변수.     │
├──────────────────────────────────────────────────────┤
│ MLIR: Value = 경량 핸들 (복사 OK)                     │
│       OwningOpRef = IR 트리의 소유권                   │
│       raw pointer = JSON 파싱에서 빌려보기             │
└──────────────────────────────────────────────────────┘
```

---

## 2.3 Move Semantics & Ownership — std::move의 진짜 의미

### 원리

복사(copy)는 데이터를 복제한다. 이동(move)은 소유권만 넘긴다.

```
복사: [A의 데이터] → [A의 데이터] + [B의 데이터]   (메모리 2배)
이동: [A의 데이터] → [B의 데이터] + [A는 빈 껍데기]  (메모리 그대로)
```

### std::move — "이제 이 객체를 안 쓸 테니 가져가라"

```cpp
// GaweeToLinalg.cpp — Pass의 runOnOperation
RewritePatternSet patterns(ctx);
patterns.add<ConvOpLowering>(ctx);
patterns.add<ReluOpLowering>(ctx);

// patterns를 프레임워크에 넘기면서 소유권 이전
if (failed(applyPartialConversion(module, target, std::move(patterns)))) {
  //                                               ^^^^^^^^^^^^^^^^^^
  // 이 줄 이후 patterns는 빈 껍데기. 다시 사용하면 안 됨!
  signalPassFailure();
}
```

### Rvalue Reference (`T&&`) — 이동을 받는 매개변수

```cpp
// applyPartialConversion의 시그니처 (간략화)
LogicalResult applyPartialConversion(
    Operation *op,
    const ConversionTarget &target,
    RewritePatternSet &&patterns    // ← && : rvalue reference
);
//                     ^^
// "patterns의 소유권을 가져가겠다"는 선언
```

- `T&` (lvalue ref): "빌려 쓰겠다" → 원본 유지
- `T&&` (rvalue ref): "가져가겠다" → 원본 무효화

### 핵심 규칙

```
1. std::move 후에는 원본을 사용하지 않는다
2. std::move는 실제로 이동시키지 않는다 — 그냥 "이동해도 된다"고 표시할 뿐
3. 받는 쪽이 T&&를 매개변수로 가져야 실제 이동이 일어남
```

### Gawee에서 move를 보는 곳

```cpp
// 1. patterns 넘기기
applyPartialConversion(module, target, std::move(patterns));

// 2. unique_ptr 반환 (return은 암묵적 move)
std::unique_ptr<Pass> createGaweeToLinalgPass() {
  return std::make_unique<GaweeToLinalgPass>();
  // return에서 자동으로 move됨 — std::move 안 써도 됨
}

// 3. unique_ptr 멤버 초기화
builder = std::make_unique<OpBuilder>(ctx);
// make_unique가 rvalue를 반환 → 자동 move
```

---

## 2.4 RTTI & LLVM 타입 시스템 — isa, cast, dyn_cast

### 원리: C++ RTTI vs LLVM 자체 타입 시스템

C++ 표준의 RTTI(`dynamic_cast`, `typeid`)는 느리고 메모리 비용이 크다.
LLVM/MLIR은 RTTI를 비활성화(`-fno-rtti`)하고 자체 타입 시스템을 쓴다.

### LLVM 캐스팅 3형제

| LLVM | Python 대응 | 실패 시 |
|---|---|---|
| `isa<T>(x)` | `isinstance(x, T)` | false 반환 |
| `cast<T>(x)` | `(T)x` (실패하면 crash) | assertion 실패 → 프로그램 종료 |
| `dyn_cast<T>(x)` | `x as T` or None | nullptr 반환 |

### Gawee에서의 사용

```cpp
// GaweeToLinalg.cpp — 타입 캐스팅
auto outputType = mlir::cast<RankedTensorType>(op.getOutput().getType());
//                     ^^^^
// "이것은 반드시 RankedTensorType이다" — 아니면 crash

auto elementType = outputType.getElementType();
// elementType은 Type (부모 타입) — 구체적 타입은 모름

// 안전하게 시도하려면:
if (auto tensorType = mlir::dyn_cast<RankedTensorType>(someType)) {
  // tensorType이 유효할 때만 이 블록 실행
  auto shape = tensorType.getShape();
}
```

### 판단 기준

```
확실히 그 타입인가?
├── YES → cast<T>(x)     빠르지만 틀리면 crash
├── 아마도 → dyn_cast<T>(x)  안전 (nullptr 체크 필수)
└── 체크만 → isa<T>(x)       bool 반환
```

### -fno-rtti의 영향

```
CMakeLists.txt에서:
  if(NOT LLVM_ENABLE_RTTI)
    add_compile_options(-fno-rtti)   ← LLVM과 RTTI 설정을 맞춰야 함
  endif()

이것이 없으면:
  → 링크 에러: "undefined reference to typeinfo for ..."
  → 원인: LLVM은 RTTI 없이 빌드, 우리 코드는 RTTI 있이 빌드 → 불일치
```

---

## 2.5 Value Semantics vs Reference Semantics

### 원리

```
Python: 기본이 참조. a = b → 같은 객체를 가리킴
C++:    기본이 값.   a = b → 복사본을 만듦
```

```python
# Python — 참조 의미론
a = [1, 2, 3]
b = a          # 같은 리스트
b.append(4)    # a도 [1, 2, 3, 4]
```

```cpp
// C++ — 값 의미론
std::vector<int> a = {1, 2, 3};
std::vector<int> b = a;    // 복사! 완전히 독립적인 별개의 벡터
b.push_back(4);            // a는 여전히 {1, 2, 3}
```

### const T& — 복사를 피하면서 안전하게

```cpp
// 나쁜 예: 큰 객체를 복사
void process(SmallVector<AffineMap> maps) { ... }  // 호출할 때마다 복사

// 좋은 예: 참조로 받기
void process(const SmallVector<AffineMap> &maps) { ... }  // 복사 없음, 수정도 불가
```

### StringRef vs std::string — View vs Owner

```cpp
// std::string — 소유자. 메모리를 할당하고 관리
std::string name = "gawee.conv";  // 힙에 복사

// StringRef — 뷰. 다른 곳의 문자열을 가리킬 뿐
StringRef getArgument() const override { return "convert-gawee-to-linalg"; }
//        ^^^^^^^^^
// 문자열 리터럴을 가리키는 뷰 — 복사 없음, 가볍다

// 주의: StringRef의 원본이 사라지면 댕글링!
StringRef danger() {
  std::string s = "temp";
  return StringRef(s);    // s가 사라지면 StringRef도 무효!
}
```

### MLIR에서의 패턴

```cpp
// Value — 경량 핸들 (8바이트). 값으로 복사해도 저렴
Value input = adaptor.getInput();      // 복사 OK

// ArrayRef — 배열의 뷰. 복사 없이 배열을 넘김
auto padding = op.getPadding();        // ArrayRef<int64_t>

// TypeRange, ValueRange — 여러 타입/값의 뷰
TypeRange{outputType}                  // 소유하지 않고 참조만
```

### 암기표

```
┌──────────────────────────────────────────────────┐
│ Python 기본 = 참조     C++ 기본 = 값 (복사)        │
│                                                  │
│ std::string   소유자 (메모리 할당)                 │
│ StringRef     뷰 (가리킬 뿐, 원본 필요)            │
│ ArrayRef<T>   배열 뷰 (복사 없이 슬라이스)          │
│ Value         경량 핸들 (복사 OK, 8바이트)          │
│ const T&      읽기 전용 참조 (복사 피하기)          │
│ TypeRange     여러 Type의 뷰                      │
│ ValueRange    여러 Value의 뷰                     │
└──────────────────────────────────────────────────┘
```

---

## 2.6 Template Metaprogramming 기초

### 원리

템플릿은 "타입을 매개변수로 받는 코드"이다.
컴파일러가 사용하는 타입마다 별도의 코드를 생성한다.

### Template Specialization — 특정 타입에 대한 특수 처리

```cpp
// 기본 버전
template <typename T>
struct Printer {
  void print(T val) { std::cout << val; }
};

// bool에 대한 특수화
template <>
struct Printer<bool> {
  void print(bool val) { std::cout << (val ? "true" : "false"); }
};
```

MLIR의 `OpConversionPattern<gawee::ConvOp>`이 이 원리이다.
`ConvOp`에 특화된 패턴 코드를 컴파일 타임에 생성한다.

### Type Traits — 타입에 대한 질문

```cpp
#include <type_traits>

std::is_same<T, int>::value       // T가 int인가?
std::is_pointer<T>::value         // T가 포인터인가?
std::is_base_of<Base, T>::value   // T가 Base의 자식인가?
```

### Variadic Templates — 가변 인자 템플릿

```cpp
// GaweeDialect.cpp — 여러 Op을 한번에 등록
void GaweeDialect::initialize() {
  addOperations<
    ConvOp,
    ReluOp,
    AddOp,
    MaxPoolOp,
    AdAvgPoolOp,
    FlattenOp,
    LinearOp
  >();
}
// addOperations<Ops...>()는 가변 인자 템플릿
// 각 Op에 대해 등록 코드를 컴파일 타임에 생성
```

```cpp
// DialectRegistry — 여러 방언 한번에 등록
registry.insert<
  gawee::GaweeDialect,
  linalg::LinalgDialect,
  arith::ArithDialect
>();
// insert<Dialects...>()도 가변 인자 템플릿
```

### CRTP 복습 (Template의 고급 활용)

```cpp
// 부모가 자식의 타입을 컴파일 타임에 앎
struct GaweeToLinalgPass
    : public PassWrapper<GaweeToLinalgPass, OperationPass<ModuleOp>> {
  //                     ^^^^^^^^^^^^^^^^^
  //  PassWrapper 내부에서 static_cast<GaweeToLinalgPass*>(this) 가능
  //  → virtual 없이 자식 메서드 호출 → 오버헤드 제로
};
```

자세한 내용은 `04_CRTP.md` 참조.

---

## 최종 암기표

```
┌─────────────────────────────────────────────────────────┐
│ Template vs Virtual                                      │
│   Template = 컴파일 타임, 빠름, 코드 팽창                  │
│   Virtual = 런타임, 유연, vtable 비용                     │
│   MLIR은 둘 다 씀: CRTP(template) + matchAndRewrite(virtual) │
├─────────────────────────────────────────────────────────┤
│ 포인터                                                    │
│   T*         = 빌려봄 (JSON 파싱)                         │
│   unique_ptr = 나만 소유 (Pass, Builder)                  │
│   T&         = null 불가 별명 (rewriter)                  │
│   Value      = MLIR 경량 핸들 (복사 OK)                   │
├─────────────────────────────────────────────────────────┤
│ Move                                                     │
│   std::move(x) → x의 소유권 이전, x는 이후 사용 금지      │
│   대표 예: applyPartialConversion(..., std::move(patterns))│
├─────────────────────────────────────────────────────────┤
│ LLVM 캐스팅                                               │
│   isa<T>(x)      → isinstance 체크                       │
│   cast<T>(x)     → 확실할 때 (틀리면 crash)               │
│   dyn_cast<T>(x) → 안전 캐스트 (nullptr 반환)             │
├─────────────────────────────────────────────────────────┤
│ Value vs Reference Semantics                             │
│   C++ 기본 = 복사. const T&로 복사 방지.                  │
│   StringRef/ArrayRef/ValueRange = 뷰 (소유 안 함)         │
├─────────────────────────────────────────────────────────┤
│ Template Metaprogramming                                 │
│   Specialization = 특정 타입 특수 처리                     │
│   Variadic = addOperations<A, B, C>() 가변 인자           │
│   CRTP = 자식이 부모에게 자기 타입 전달                    │
└─────────────────────────────────────────────────────────┘
```

---

# 2b장. C++ 코드 패턴 — 반복되는 구조를 통째로 외우기

Python에서 `argparse`, `if __name__ == "__main__"`, `class Module` 같은 패턴이 있듯이,
C++/MLIR에도 반복되는 코드 구조가 있다. 이것을 통째로 외우면 새 코드를 빠르게 작성할 수 있다.

모든 예시는 Gawee 코드베이스에서 가져왔다.

---

## 패턴 1: CLI 도구 (Python의 argparse)

**Python에서는:**
```python
parser = argparse.ArgumentParser(description="my tool")
parser.add_argument("input", help="input file")
parser.add_argument("-o", "--output", default="-")
args = parser.parse_args()
```

**C++/LLVM에서는:** (`gawee-translate.cpp`)

```cpp
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"

// ① 전역 변수로 옵션 선언 (argparse.add_argument에 해당)
static cl::opt<std::string> inputFilename(
    cl::Positional,                     // 위치 인자
    cl::desc("<input JSON file>"),      // help 문자열
    cl::Required                        // 필수
);

static cl::opt<std::string> outputFilename(
    "o",                                // -o 플래그
    cl::desc("Output filename"),
    cl::value_desc("filename"),
    cl::init("-")                       // default="-"
);

int main(int argc, char **argv) {
    // ② 초기화 (LLVM 시그널 핸들러 등록)
    InitLLVM y(argc, argv);

    // ③ 파싱 (args = parser.parse_args()에 해당)
    cl::ParseCommandLineOptions(argc, argv, "My tool description\n");

    // ④ 사용 — 전역 변수를 그냥 쓰면 됨
    auto fileOrErr = MemoryBuffer::getFileOrSTDIN(inputFilename);
    // ...
}
```

**핵심**: Python은 `args.input`으로 접근하지만, LLVM은 전역 `static cl::opt` 변수 자체가 값이다.

---

## 패턴 2: 에러 체인 (Python의 try/except 대신)

**Python에서는:**
```python
try:
    data = json.load(f)
except json.JSONDecodeError as e:
    print(f"Error: {e}")
    sys.exit(1)
```

**C++/LLVM에서는:** (`gawee-translate.cpp`)

```cpp
// LLVM 스타일: 각 단계마다 실패 체크 → 에러 메시지 → return
// 예외(exception) 대신 반환값으로 에러를 전달한다

// ① 파일 읽기
auto fileOrErr = MemoryBuffer::getFileOrSTDIN(inputFilename);
if (auto ec = fileOrErr.getError()) {
    errs() << "Error: " << ec.message() << "\n";
    return 1;
}

// ② JSON 파싱
auto jsonOrErr = json::parse(fileOrErr.get()->getBuffer());
if (!jsonOrErr) {
    errs() << "Error: " << toString(jsonOrErr.takeError()) << "\n";
    return 1;
}

// ③ 타입 체크
const auto *graph = jsonOrErr->getAsObject();
if (!graph) {
    errs() << "Error: JSON root must be an object\n";
    return 1;
}
```

**핵심**: LLVM/MLIR은 exception을 쓰지 않는다 (`-fno-exceptions`).
대신 `Expected<T>`, `Optional`, `LogicalResult`, `nullptr` 반환으로 에러를 전달한다.

---

## 패턴 3: 처리기 클래스 (Python의 stateful class)

**Python에서는:**
```python
class Emitter:
    def __init__(self, context):
        self.ctx = context
        self.value_map = {}        # 내부 상태

    def emit(self, graph):         # 공개 API
        self._collect_weights(graph)
        self._emit_nodes(graph)

    def _collect_weights(self, g):  # 내부 헬퍼 (_로 시작)
        ...
    def _emit_nodes(self, g):
        ...
```

**C++에서는:** (`MLIREmitter.h`)

```cpp
class MLIREmitter {
public:
    // ① 생성자 — explicit로 암묵적 변환 방지
    explicit MLIREmitter(MLIRContext *context);

    // ② 공개 API — Python의 def emit(self, ...)
    OwningOpRef<ModuleOp> emit(const llvm::json::Object &graph);

    // ③ 에러 접근 — Python의 @property
    llvm::StringRef getError() const { return errorMsg; }

private:
    // ④ 내부 상태 — Python의 self.xxx
    MLIRContext *ctx;                                      // 빌려 쓰는 포인터
    std::unique_ptr<OpBuilder> builder;                    // 소유하는 객체
    std::string errorMsg;                                  // 에러 메시지 버퍼
    std::unordered_map<std::string, Value> valueMap;       // name → Value 매핑

    // ⑤ 내부 헬퍼 — Python의 def _xxx(self, ...)
    RankedTensorType parseShape(const llvm::json::Array *shape);
    bool emitNode(const llvm::json::Object &node, const llvm::json::Object &values);
    bool emitConv(const llvm::json::Object &node, const llvm::json::Object &values);
    bool emitRelu(const llvm::json::Object &node, const llvm::json::Object &values);
    // ...

    void setError(const llvm::Twine &msg);
};
```

**사용 패턴:**
```cpp
// Python: emitter = Emitter(context); module = emitter.emit(graph)
gawee::MLIREmitter emitter(&context);
auto module = emitter.emit(*graph);
if (!module) {
    errs() << "Error: " << emitter.getError() << "\n";
    return 1;
}
```

**핵심 대응:**
| Python | C++ |
|---|---|
| `self.xxx` (인스턴스 변수) | `private:` 멤버 변수 |
| `def _helper(self)` | `private:` 멤버 함수 |
| `def emit(self, graph)` | `public:` 멤버 함수 |
| `__init__` | constructor (생성자) |

---

## 패턴 4: Visitor 디스패치 (Python의 if/elif 체인)

**Python에서는:**
```python
def process_node(node):
    op = node["op_type"]
    if op == "Conv":    return emit_conv(node)
    elif op == "Relu":  return emit_relu(node)
    elif op == "Add":   return emit_add(node)
    else: raise ValueError(f"Unknown op: {op}")
```

**C++에서는:** (`MLIREmitter.cpp` — emitNode 함수)

```cpp
bool MLIREmitter::emitNode(const json::Object &node, const json::Object &values) {
    auto opType = node.getString("op_type");
    if (!opType) {
        setError("Missing op_type");
        return false;
    }

    // Visitor 디스패치 — 각 타입에 맞는 핸들러 호출
    if (*opType == "Conv")        return emitConv(node, values);
    if (*opType == "Relu")        return emitRelu(node, values);
    if (*opType == "Add")         return emitAdd(node, values);
    if (*opType == "MaxPool")     return emitMaxPool(node, values);
    if (*opType == "AdAvgPool")   return emitAdAvgPool(node, values);
    if (*opType == "Flatten")     return emitFlatten(node, values);
    if (*opType == "MatMul")      return emitLinear(node, values);

    setError("Unknown op_type: " + *opType);
    return false;
}
```

**핵심**: Python과 구조가 거의 동일하다. 다만 `*opType`에서 `*`는 Optional을 벗기는 것.

---

## 패턴 5: 레지스트리 (Python의 dict에 함수 등록)

**Python에서는:**
```python
handlers = {}
handlers["conv"] = ConvHandler()
handlers["relu"] = ReluHandler()
# 또는
registry = [ConvHandler, ReluHandler, AddHandler]
```

**C++/MLIR에서는:** (`GaweeToLinalg.cpp`)

```cpp
// ① 패턴 레지스트리 — add<T>로 타입을 등록
RewritePatternSet patterns(ctx);
patterns.add<ConvOpLowering>(ctx);      // ConvOp 처리기 등록
patterns.add<ReluOpLowering>(ctx);      // ReluOp 처리기 등록
patterns.add<AddOpLowering>(ctx);       // AddOp 처리기 등록
patterns.add<MaxPoolOpLowering>(ctx);
patterns.add<LinearOpLowering>(ctx);

// 프레임워크가 알아서 매칭해서 호출
applyPartialConversion(module, target, std::move(patterns));
```

```cpp
// ② 방언 레지스트리 — insert<T>로 방언 등록
DialectRegistry registry;
registry.insert<gawee::GaweeDialect>();
registry.insert<linalg::LinalgDialect>();
registry.insert<arith::ArithDialect>();
// ...
```

**핵심**: Python은 dict/list에 인스턴스를 넣지만, C++/MLIR은 `add<타입>()`으로 **타입 자체**를 등록한다.
프레임워크가 내부적으로 인스턴스를 만들어서 관리한다.

---

## 패턴 6: 헤더 가드 + 네임스페이스 (Python의 모듈)

**Python에서는:**
```python
# my_module.py — 파일 자체가 모듈
# import my_module 하면 끝
```

**C++에서는:** (`MLIREmitter.h`)

```cpp
// ① 헤더 가드 — 같은 헤더를 두 번 include해도 에러 안 남
#ifndef GAWEE_EMIT_MLIREMITTER_H
#define GAWEE_EMIT_MLIREMITTER_H

// ② include — Python의 import
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/Support/JSON.h"
#include <string>

// ③ 네임스페이스 — Python의 패키지 경로
namespace mlir::gawee {

class MLIREmitter {
    // ...
};

} // namespace mlir::gawee

#endif // GAWEE_EMIT_MLIREMITTER_H
```

```cpp
// ④ 익명 네임스페이스 — Python의 _private 모듈 관례
// GaweeToLinalg.cpp
namespace {
    // 이 안의 모든 것은 이 파일에서만 접근 가능
    struct ConvOpLowering : public OpConversionPattern<gawee::ConvOp> { ... };
    struct ReluOpLowering : public OpConversionPattern<gawee::ReluOp> { ... };
} // namespace
```

**핵심**: `namespace {}` 안에 넣으면 **이 파일 전용**이 된다. Python에서 `_`로 시작하는 모듈 변수처럼.

---

## 패턴 7: Forward Declaration (Python에 없는 패턴)

파일 A에서 구현하고 파일 B에서 사용할 때, 헤더 없이 "이런 함수가 존재한다"고 선언만 한다.

```cpp
// gawee-opt.cpp — GaweeToLinalg.cpp의 함수를 사용하려면
namespace mlir::gawee {
std::unique_ptr<Pass> createGaweeToLinalgPass();  // "이 함수가 어딘가에 있다"
}

int main(...) {
    pm.addPass(gawee::createGaweeToLinalgPass());  // 링크 타임에 연결됨
}
```

**왜 필요한가?** C++은 컴파일 순서가 중요하다. 함수를 사용하기 전에 "시그니처"를 알아야 한다.
헤더 파일을 만들기 귀찮을 때 forward declaration으로 대체한다.

---

## 패턴 8: MLIR 도구의 main() (Python의 if __name__ == "__main__")

**gawee-opt 스타일** — MLIR 프레임워크에게 대부분 위임:

```cpp
int main(int argc, char **argv) {
    // ① Pass 파이프라인 등록
    PassPipelineRegistration<>(
        "my-pipeline-name",
        "Description",
        [](OpPassManager &pm) {
            pm.addPass(createMyPass());
        });

    // ② 방언 등록
    DialectRegistry registry;
    registry.insert<MyDialect>();
    registry.insert<linalg::LinalgDialect>();

    // ③ MlirOptMain에 위임 (CLI 파싱, 파일 읽기, Pass 실행, 출력 — 전부 처리)
    return asMainReturnCode(
        MlirOptMain(argc, argv, "My Tool\n", registry));
}
```

**gawee-translate 스타일** — 직접 제어:

```cpp
int main(int argc, char **argv) {
    InitLLVM y(argc, argv);
    cl::ParseCommandLineOptions(argc, argv, "My translator\n");

    // ① 입력 읽기
    auto fileOrErr = MemoryBuffer::getFileOrSTDIN(inputFilename);
    if (auto ec = fileOrErr.getError()) { /* 에러 처리 */ return 1; }

    // ② MLIR 컨텍스트 생성 + 방언 로드
    MLIRContext context;
    context.loadDialect<gawee::GaweeDialect>();
    context.loadDialect<func::FuncDialect>();

    // ③ 처리
    MyEmitter emitter(&context);
    auto result = emitter.process(input);
    if (!result) { /* 에러 처리 */ return 1; }

    // ④ 출력
    result->print(output);
    return 0;
}
```

**판단**: opt처럼 "MLIR을 읽고 변환하고 MLIR을 출력"하면 `MlirOptMain` 사용.
translate처럼 "다른 포맷을 읽어서 MLIR 생성"하면 직접 작성.

---

## 패턴 9: Lowering 패턴 (MLIR 특유의 반복 구조)

모든 Lowering 패턴이 이 구조를 따른다:

```cpp
namespace {

struct MyOpLowering : public OpConversionPattern<gawee::MyOp> {
    using OpConversionPattern::OpConversionPattern;  // 부모 생성자 상속

    LogicalResult
    matchAndRewrite(gawee::MyOp op, OpAdaptor adaptor,
                    ConversionPatternRewriter &rewriter) const override {
        Location loc = op.getLoc();

        // ① adaptor에서 입력 추출 (변환된 값)
        Value input = adaptor.getInput();

        // ② op에서 속성 추출 (변환 불필요)
        auto strides = op.getStridesAttr();

        // ③ 타입 추출
        auto outputType = mlir::cast<RankedTensorType>(op.getOutput().getType());
        auto elementType = outputType.getElementType();

        // ④ 새 Op 생성 (rewriter 사용)
        Value empty = tensor::EmptyOp::create(rewriter, loc, outputType.getShape(), elementType);
        // ... 더 많은 Op 생성 ...

        // ⑤ 원래 Op 교체
        rewriter.replaceOp(op, newResults);
        return success();
    }
};

} // namespace
```

**새 Op을 lowering할 때**: 이 뼈대를 복사하고 ①~⑤만 채우면 된다.

---

## 패턴 10: Pass 등록 (Lowering 패턴들을 묶는 껍데기)

```cpp
namespace {

struct MyPass : public PassWrapper<MyPass, OperationPass<ModuleOp>> {
    StringRef getArgument() const override { return "my-pass-name"; }
    StringRef getDescription() const override { return "My description"; }

    void getDependentDialects(DialectRegistry &registry) const override {
        registry.insert<linalg::LinalgDialect>();  // 이 Pass가 만들 Op의 방언
    }

    void runOnOperation() override {
        MLIRContext *ctx = &getContext();
        ModuleOp module = getOperation();

        // ① 합법/불법 설정
        ConversionTarget target(*ctx);
        target.addLegalDialect<linalg::LinalgDialect>();
        target.addIllegalDialect<gawee::GaweeDialect>();

        // ② 패턴 등록
        RewritePatternSet patterns(ctx);
        patterns.add<MyOpLowering>(ctx);

        // ③ 실행
        if (failed(applyPartialConversion(module, target, std::move(patterns))))
            signalPassFailure();
    }
};

} // namespace

// 외부에서 사용할 팩토리 함수
namespace mlir::gawee {
std::unique_ptr<Pass> createMyPass() {
    return std::make_unique<MyPass>();
}
}
```

---

## 한눈에 보기 — Python 패턴 ↔ C++ 패턴

| Python 패턴 | C++ 패턴 | Gawee 파일 |
|---|---|---|
| `argparse` + 옵션 | `static cl::opt` + `ParseCommandLineOptions` | `gawee-translate.cpp` |
| `try/except` 체인 | `if (err) { report; return; }` 반복 | `gawee-translate.cpp` |
| `class Processor` + `_helpers` | `class` + `private:` 멤버 | `MLIREmitter.h` |
| `if/elif` 디스패치 | `if (*opType == "X") return emitX()` | `MLIREmitter.cpp` |
| `dict[key] = handler` | `patterns.add<Handler>(ctx)` | `GaweeToLinalg.cpp` |
| 모듈 (`_private`) | `namespace { }` 익명 | `GaweeToLinalg.cpp` |
| `import` | `#include` + 헤더 가드 | 모든 `.h` 파일 |
| `if __name__ == "__main__"` | `int main()` + `MlirOptMain` 또는 직접 | `gawee-opt.cpp` |
| 없음 (forward decl) | `namespace X { fn(); }` 선언만 | `gawee-opt.cpp` |

---

# 2c장. Modern C++ 코드 패턴 — 반복되는 구조 모음

MLIR이 아닌 **일반 C++ 코드**에서 반복되는 구조를 정리한다.
새 코드를 작성할 때 해당하는 패턴을 복사하고 수정하면 된다.

---

## 패턴 1: RAII — 자원을 생성자에서 잡고 소멸자에서 놓기

**문제**: 파일/메모리/락 등 자원을 열었으면 반드시 닫아야 한다.

**Python에서는:**
```python
with open("data.txt") as f:
    data = f.read()
# 블록 끝나면 자동으로 f.close()
```

**C++에서는:** 객체의 수명 = 자원의 수명

```cpp
// ① 파일 — 스코프 끝에서 자동 닫힘
{
    std::ifstream file("data.txt");  // 생성자에서 open
    std::string line;
    while (std::getline(file, line)) {
        // ...
    }
}  // 소멸자에서 자동 close — finally 블록 필요 없음

// ② 메모리 — unique_ptr이 RAII
{
    auto builder = std::make_unique<OpBuilder>(ctx);  // 생성자에서 new
    builder->create(...);
}  // 소멸자에서 자동 delete

// ③ 락 — lock_guard가 RAII
{
    std::lock_guard<std::mutex> lock(mtx);  // 생성자에서 lock
    shared_data.push_back(item);
}  // 소멸자에서 자동 unlock
```

**핵심**: C++에서 `with`문이 필요 없는 이유. 모든 자원 관리를 소멸자에 맡긴다.

---

## 패턴 2: Rule of Zero — 특별한 멤버 함수를 안 쓰기

**문제**: 생성자/소멸자/복사/이동을 언제 직접 써야 하는가?

```cpp
// ★ 좋은 예: Rule of Zero — 컴파일러가 다 해줌
struct Config {
    std::string name;
    std::vector<int> values;
    std::unique_ptr<Logger> logger;
    // 생성자, 소멸자, 복사, 이동 — 전부 안 써도 됨
    // 각 멤버가 자기 자원을 관리하므로 (RAII)
};

// ✗ 나쁜 예: 직접 관리
struct Config {
    char* name;           // raw pointer → 직접 관리해야 함
    int* values;
    ~Config() {           // 소멸자 필수
        delete[] name;
        delete[] values;
    }
    // 복사 생성자, 이동 생성자도 전부 직접 써야 함...
};
```

**규칙**: 멤버 변수를 `string`, `vector`, `unique_ptr` 같은 RAII 타입으로만 쓰면
생성자/소멸자를 직접 안 써도 된다.

---

## 패턴 3: Factory 함수 — unique_ptr 반환

**문제**: 다형성(polymorphism) 객체를 생성해서 넘겨야 할 때.

**Python에서는:**
```python
def create_optimizer(config):
    if config.type == "adam":
        return Adam(config.lr)
    return SGD(config.lr)
```

**C++에서는:**
```cpp
// 부모 타입의 unique_ptr로 반환 → 호출자가 소유
std::unique_ptr<Optimizer> createOptimizer(const Config &config) {
    if (config.type == "adam")
        return std::make_unique<Adam>(config.lr);
    return std::make_unique<SGD>(config.lr);
}

// 사용
auto opt = createOptimizer(config);  // 호출자가 소유권 가짐
opt->step();                          // 가상 함수 호출
// 스코프 끝에서 자동 delete
```

**핵심**: `return std::make_unique<자식>();` — 소유권 이전의 정석.

---

## 패턴 4: enum class + switch — 안전한 열거형

**Python에서는:**
```python
from enum import Enum
class Color(Enum):
    RED = 0
    GREEN = 1
    BLUE = 2
```

**C++에서는:**
```cpp
// ① 정의 — enum class는 타입 안전 (다른 enum과 섞이지 않음)
enum class Color { Red, Green, Blue };

// ② switch — 모든 경우를 처리 (-Wswitch 경고로 누락 방지)
std::string colorName(Color c) {
    switch (c) {
    case Color::Red:   return "red";
    case Color::Green: return "green";
    case Color::Blue:  return "blue";
    }
    llvm_unreachable("unknown color");  // 여기 도달하면 버그
}
```

**핵심**: `enum class`는 `Color::Red`처럼 접두사 필수 → 이름 충돌 방지.
`switch`에서 case 하나라도 빠뜨리면 컴파일러가 경고.

---

## 패턴 5: const 참조 매개변수 — 복사 없이 읽기

**문제**: 함수에 큰 객체를 넘길 때 복사 비용을 없애고 싶다.

```cpp
// ✗ 복사 발생 — vector 전체가 복제됨
void process(std::vector<int> data) { ... }

// ★ 복사 없음 — 원본을 읽기 전용으로 빌림
void process(const std::vector<int> &data) { ... }

// ★ 수정 필요하면 — const 없이
void modify(std::vector<int> &data) { data.push_back(42); }

// ★ 작은 타입 (int, bool, pointer)은 그냥 값으로
void check(int value) { ... }       // 복사가 참조보다 쌈
void check(bool flag) { ... }
```

**판단 기준:**
```
sizeof(T) <= 16바이트?  → 값으로 (int, bool, float, pointer, Value)
읽기만?                → const T&
수정 필요?             → T&
소유권 넘기기?         → T&& 또는 unique_ptr<T>
```

---

## 패턴 6: Optional 반환 — "실패할 수 있는 함수"

**Python에서는:**
```python
def find_user(name):
    if name in db:
        return db[name]
    return None  # 없으면 None
```

**C++에서는:**
```cpp
// ① std::optional — 값이 있거나 없거나
std::optional<User> findUser(const std::string &name) {
    auto it = db.find(name);
    if (it != db.end())
        return it->second;         // 값 있음
    return std::nullopt;            // 값 없음 (Python의 None)
}

// ② 사용 — 값 유무 체크 후 접근
auto user = findUser("alice");
if (user) {                         // has_value() 와 동일
    std::cout << user->name;        // *user 또는 user-> 로 접근
}

// ③ 기본값 — Python의 or 패턴
auto name = findName(id).value_or("unknown");
// Python: name = find_name(id) or "unknown"
```

---

## 패턴 7: 콜백 / std::function — 함수를 인자로 넘기기

**Python에서는:**
```python
def apply(items, fn):
    return [fn(x) for x in items]

apply(data, lambda x: x * 2)
```

**C++에서는:**
```cpp
// ① 템플릿 — 가장 빠름 (인라인 가능)
template <typename Fn>
void forEach(const std::vector<int> &items, Fn fn) {
    for (auto &x : items) fn(x);
}
forEach(data, [](int x) { std::cout << x * 2; });

// ② std::function — 저장해야 할 때 (멤버 변수, 컨테이너)
class EventHandler {
    std::function<void(int)> callback;  // 함수를 멤버로 저장
public:
    void setCallback(std::function<void(int)> fn) {
        callback = std::move(fn);
    }
    void fire(int event) {
        if (callback) callback(event);
    }
};

handler.setCallback([&](int e) { log(e); });
```

**판단**: 바로 호출 → 템플릿. 저장해뒀다가 나중에 호출 → `std::function`.

---

## 패턴 8: Structured Bindings — 여러 값 한번에 꺼내기

**Python에서는:**
```python
for key, value in items.items():
    print(f"{key}: {value}")

x, y, z = get_point()
```

**C++에서는:**
```cpp
// ① map 순회
std::unordered_map<std::string, int> scores = {{"alice", 95}, {"bob", 87}};
for (const auto &[name, score] : scores) {
    std::cout << name << ": " << score << "\n";
}

// ② pair/tuple 분해
auto [min, max] = std::minmax({3, 1, 4, 1, 5});
// min = 1, max = 5

// ③ struct 분해 (public 멤버만)
struct Point { double x, y; };
Point p{3.0, 4.0};
auto [x, y] = p;

// ④ 함수에서 여러 값 반환
std::pair<bool, std::string> validate(int x) {
    if (x < 0) return {false, "negative"};
    return {true, "ok"};
}
auto [ok, msg] = validate(42);
```

---

## 패턴 9: Virtual Interface — 인터페이스 정의하고 자식이 구현

**Python에서는:**
```python
from abc import ABC, abstractmethod

class Serializer(ABC):
    @abstractmethod
    def serialize(self, data) -> bytes: ...

    @abstractmethod
    def deserialize(self, raw) -> dict: ...

class JsonSerializer(Serializer):
    def serialize(self, data): return json.dumps(data).encode()
    def deserialize(self, raw): return json.loads(raw)
```

**C++에서는:**
```cpp
// ① 인터페이스 정의 — 순수 가상 함수 (= 0)
class Serializer {
public:
    virtual ~Serializer() = default;  // ★ 가상 소멸자 필수!

    // 순수 가상 함수 — 자식이 반드시 구현
    virtual std::vector<uint8_t> serialize(const Data &data) = 0;
    virtual Data deserialize(const std::vector<uint8_t> &raw) = 0;

    // 일반 가상 함수 — 기본 구현 있음, 자식이 선택적으로 재정의
    virtual std::string name() const { return "unknown"; }
};

// ② 구현 — override 키워드로 실수 방지
class JsonSerializer : public Serializer {
public:
    std::vector<uint8_t> serialize(const Data &data) override {
        // ... JSON 직렬화
    }
    Data deserialize(const std::vector<uint8_t> &raw) override {
        // ... JSON 역직렬화
    }
    std::string name() const override { return "json"; }
};

class BinarySerializer : public Serializer {
public:
    std::vector<uint8_t> serialize(const Data &data) override { /* ... */ }
    Data deserialize(const std::vector<uint8_t> &raw) override { /* ... */ }
    std::string name() const override { return "binary"; }
};

// ③ 사용 — 부모 포인터/참조로 자식을 다룸
void save(Serializer &s, const Data &data) {
    auto bytes = s.serialize(data);  // 런타임에 어떤 자식인지 결정
    writeFile(bytes);
}

// ④ Factory와 조합
std::unique_ptr<Serializer> createSerializer(const std::string &fmt) {
    if (fmt == "json")   return std::make_unique<JsonSerializer>();
    if (fmt == "binary") return std::make_unique<BinarySerializer>();
    return nullptr;
}

auto s = createSerializer("json");
save(*s, data);
```

**반드시 기억할 것:**
```
1. 부모에 virtual ~Destructor() = default;  ← 없으면 자식 소멸자 안 불림 → 메모리 누수
2. 자식에 override 키워드                     ← 없으면 오타 시 새 함수가 생김 (버그)
3. 순수 가상(= 0)이면 인스턴스 생성 불가       ← Python의 ABC와 동일
```

---

## 패턴 10: string_view / StringRef — 문자열을 복사 없이 빌려보기

**Python에서는:** 문자열 슬라이싱이 새 객체를 만들지만 보통 신경 안 씀.

**C++에서는:**
```cpp
// ✗ 복사 발생
void log(std::string msg) { std::cout << msg; }
log("hello");  // "hello" → std::string 임시 객체 생성

// ★ 복사 없음 — 문자열을 가리킬 뿐
void log(std::string_view msg) { std::cout << msg; }
log("hello");             // 리터럴을 직접 가리킴
log(some_string);         // std::string도 받음
log(some_string.substr(0, 5));  // 부분 문자열도 복사 없이

// LLVM에서는 StringRef (같은 개념)
void log(llvm::StringRef msg) { llvm::errs() << msg; }
```

**주의**: 원본 문자열이 사라지면 view도 무효.
```cpp
std::string_view danger() {
    std::string s = "temp";
    return s;  // s가 사라지면 view는 댕글링 포인터!
}
```

---

## 패턴 11: Template 함수/클래스 — 타입을 매개변수로 받는 코드

**Python에서는:** 타입을 신경 안 써도 됨 (duck typing).
```python
def max_element(items):       # int든 float든 str이든 그냥 작동
    return max(items)
```

**C++에서는:** 타입이 다르면 다른 함수가 필요 → 템플릿으로 해결.

```cpp
// ═══════════════════════════════════════════
// ① Template 함수 — 가장 기본
// ═══════════════════════════════════════════
template <typename T>
T maxElement(const std::vector<T> &items) {
    T result = items[0];
    for (const auto &item : items) {
        if (item > result) result = item;
    }
    return result;
}

// 사용 — 컴파일러가 T를 추론
int m1 = maxElement(std::vector<int>{3, 1, 4});       // T = int
double m2 = maxElement(std::vector<double>{2.7, 3.14}); // T = double
// 컴파일러가 int 버전과 double 버전을 각각 생성

// ═══════════════════════════════════════════
// ② Template 클래스 — 타입을 품은 컨테이너
// ═══════════════════════════════════════════
template <typename T>
class Stack {
    std::vector<T> data;
public:
    void push(const T &val) { data.push_back(val); }
    void push(T &&val) { data.push_back(std::move(val)); }  // 이동 버전

    T pop() {
        T top = std::move(data.back());
        data.pop_back();
        return top;
    }

    bool empty() const { return data.empty(); }
    size_t size() const { return data.size(); }
};

Stack<int> intStack;
intStack.push(42);

Stack<std::string> strStack;
strStack.push("hello");

// ═══════════════════════════════════════════
// ③ 여러 타입 매개변수
// ═══════════════════════════════════════════
template <typename K, typename V>
class Registry {
    std::unordered_map<K, V> entries;
public:
    void add(const K &key, V value) {
        entries[key] = std::move(value);
    }
    V* find(const K &key) {
        auto it = entries.find(key);
        return (it != entries.end()) ? &it->second : nullptr;
    }
};

Registry<std::string, int> scores;
scores.add("alice", 95);

// ═══════════════════════════════════════════
// ④ Template + Virtual 조합 (MLIR의 OpConversionPattern)
// ═══════════════════════════════════════════
// Template이 구체 타입을 고정하고, Virtual이 런타임 디스패치를 담당
template <typename OpT>
class OpHandler {
public:
    virtual ~OpHandler() = default;
    virtual bool handle(OpT &op) = 0;  // 자식이 구현

    // Template이 제공하는 타입 정보
    using OpType = OpT;
};

// 사용: ConvOp 전용 핸들러
class ConvHandler : public OpHandler<ConvOp> {
    bool handle(ConvOp &op) override { /* ... */ }
};
```

**핵심 규칙:**
```
함수 템플릿: 호출 시 타입 추론 → maxElement(vec) 처럼 <T> 생략 가능
클래스 템플릿: 선언 시 타입 명시 → Stack<int>, Registry<string, int>
헤더에 구현: 템플릿은 .cpp에 넣으면 안 됨 → .h에 전부 작성
```

---

## 패턴 12: constexpr — 컴파일 타임 계산

**Python에서는:** 상수는 그냥 대문자 변수.

```python
MAX_SIZE = 1024
HEADER_SIZE = 4 + 8 + 2  # 런타임에 계산해도 상관없음
```

**C++에서는:**
```cpp
// ① 컴파일 타임 상수 — #define 대신
constexpr int MAX_SIZE = 1024;
constexpr int HEADER_SIZE = 4 + 8 + 2;  // 컴파일 타임에 14로 계산됨

// ② 컴파일 타임 함수
constexpr int factorial(int n) {
    return (n <= 1) ? 1 : n * factorial(n - 1);
}
constexpr int f5 = factorial(5);  // 컴파일 타임에 120

// ③ if constexpr — 컴파일 타임 분기 (코드 제거)
template <typename T>
void print(T val) {
    if constexpr (std::is_same_v<T, std::string>) {
        std::cout << "string: " << val;
    } else if constexpr (std::is_integral_v<T>) {
        std::cout << "int: " << val;
    }
    // 해당 안 되는 분기는 아예 컴파일되지 않음
}
```

---

## 패턴 13: static_assert — 컴파일 타임 검증

**Python에서는:** 런타임에만 체크 가능.

```python
assert len(FIELDS) == 3, "expected 3 fields"
```

**C++에서는:**
```cpp
// 컴파일 타임에 조건 체크 — 틀리면 컴파일 에러
static_assert(sizeof(int) == 4, "int must be 4 bytes");
static_assert(std::is_trivially_copyable_v<MyStruct>, "must be trivially copyable");

// 템플릿에서 타입 제약
template <typename T>
void serialize(T val) {
    static_assert(std::is_arithmetic_v<T>, "T must be a number type");
    // ...
}
```

---

## 패턴 14: Scope Guard — "이 블록 끝에서 반드시 실행"

**Python에서는:**
```python
try:
    resource = acquire()
    do_work(resource)
finally:
    release(resource)
```

**C++에서는:** (RAII 활용)

```cpp
// ① 간단한 scope guard (C++에 표준은 없지만 자주 쓰는 패턴)
auto cleanup = [&]() { release(resource); };

// ② 좀 더 정석적으로 — 작은 헬퍼 struct
struct ScopeGuard {
    std::function<void()> fn;
    ~ScopeGuard() { fn(); }
};

void process() {
    auto resource = acquire();
    ScopeGuard guard{[&]() { release(resource); }};

    do_work(resource);       // 예외가 나도
    if (error) return;       // 중간에 return해도
    // guard 소멸자에서 release 실행됨
}

// ③ unique_ptr + custom deleter (가장 실전적)
auto resource = std::unique_ptr<Resource, decltype(&release)>(
    acquire(), &release
);
// 스코프 끝에서 자동으로 release(resource) 호출
```

---

## 패턴 15: Aggregate 초기화 — struct를 dict처럼

**Python에서는:**
```python
@dataclass
class Config:
    width: int = 640
    height: int = 480
    fullscreen: bool = False
```

**C++에서는:**
```cpp
struct Config {
    int width = 640;
    int height = 480;
    bool fullscreen = false;
};

// ① 기본값 사용
Config c1;  // {640, 480, false}

// ② 일부만 변경
Config c2{1920, 1080};  // {1920, 1080, false} — fullscreen은 기본값

// ③ C++20 designated initializers (Python의 키워드 인자와 비슷)
Config c3{.width = 1920, .height = 1080, .fullscreen = true};
```

---

## 패턴 16: CRTP — 부모가 자식 타입을 컴파일 타임에 아는 패턴

**문제**: virtual은 런타임 비용이 있다. 자식 타입이 컴파일 타임에 확정되면 비용을 없앨 수 있다.

**Python에서는:** 이런 개념이 없다 (전부 런타임).

**C++에서는:**
```cpp
// ═══════════════════════════════════════════
// ① 기본 CRTP — 자식이 자기 타입을 부모에게 넘김
// ═══════════════════════════════════════════
template <typename Derived>
class Printable {
public:
    void print() const {
        // static_cast로 자식에 접근 — virtual 없이!
        const auto &self = static_cast<const Derived &>(*this);
        std::cout << self.toString() << "\n";
    }
};

class Dog : public Printable<Dog> {
public:
    std::string toString() const { return "Dog: 멍멍"; }
};

class Cat : public Printable<Cat> {
public:
    std::string toString() const { return "Cat: 야옹"; }
};

Dog d;
d.print();  // "Dog: 멍멍" — virtual 호출 없음, 인라인됨

// ═══════════════════════════════════════════
// ② 실전 CRTP — 공통 로직을 부모에 두고, 자식이 핵심만 구현
// ═══════════════════════════════════════════
template <typename Derived>
class TestCase {
public:
    void run() {
        auto &self = static_cast<Derived &>(*this);
        std::cout << "Setup...\n";
        self.setUp();          // 자식이 구현
        self.runTest();        // 자식이 구현
        self.tearDown();       // 자식이 구현 (또는 기본값)
        std::cout << "Done.\n";
    }

    // 기본 구현 — 자식이 안 쓰면 이게 호출됨
    void setUp() {}
    void tearDown() {}
};

class MyTest : public TestCase<MyTest> {
public:
    void setUp() { db.connect(); }
    void runTest() { assert(db.query("SELECT 1") == 1); }
    void tearDown() { db.disconnect(); }
};

MyTest t;
t.run();  // Setup → setUp → runTest → tearDown → Done

// ═══════════════════════════════════════════
// ③ MLIR에서의 CRTP
// ═══════════════════════════════════════════
// PassWrapper<자기자신, 부모Pass>
struct GaweeToLinalgPass
    : public PassWrapper<GaweeToLinalgPass, OperationPass<ModuleOp>> {
    // PassWrapper 내부에서 static_cast<GaweeToLinalgPass*>(this) 가능
    // → virtual 없이 자식 메서드 접근
    void runOnOperation() override { /* ... */ }
};
```

**Virtual vs CRTP 판단:**
```
런타임에 어떤 자식인지 결정?
├── YES → Virtual (예: factory에서 반환된 객체)
│   std::unique_ptr<Base> obj = createSomething();
│   obj->doWork();  // 런타임에 어떤 자식인지 모름
│
└── NO → CRTP (예: 타입이 코드에 박혀있음)
    MyPass pass;
    pass.run();     // 항상 MyPass — 컴파일 타임에 확정
```

---

## 패턴 17: 이동으로 소유권 넘기기 — push_back + emplace_back

**Python에서는:** 신경 안 써도 됨 (전부 참조).

**C++에서는:**
```cpp
std::vector<std::string> names;

// ① 복사 — 원본 유지
std::string name = "alice";
names.push_back(name);         // name의 복사본이 들어감
// name은 여전히 "alice"

// ② 이동 — 원본 비움 (빠름)
names.push_back(std::move(name));  // name의 내용물이 벡터로 이동
// name은 이제 빈 문자열 — 사용 금지

// ③ emplace_back — 컨테이너 안에서 직접 생성 (가장 효율적)
names.emplace_back("bob");     // "bob"으로 string을 벡터 안에서 바로 생성
// 임시 객체도, 복사도, 이동도 없음

// ④ unique_ptr은 이동만 가능
std::vector<std::unique_ptr<Base>> items;
items.push_back(std::make_unique<Derived>());     // OK (이동)
// items.push_back(items[0]);                      // 에러! 복사 불가
items.push_back(std::move(items[0]));              // OK (이동)
```

---

## 패턴 18: 반복자 무효화 방지 — 순회 중 수정

**Python에서는:**
```python
# 위험: 순회 중 삭제
for item in items:
    if bad(item):
        items.remove(item)  # 버그 가능

# 안전: 새 리스트
items = [x for x in items if not bad(x)]
```

**C++에서는:**
```cpp
// ✗ 위험: 반복자 무효화
for (auto it = vec.begin(); it != vec.end(); ++it) {
    if (bad(*it))
        vec.erase(it);  // it이 무효화됨! 다음 ++it에서 crash
}

// ★ 안전: erase가 다음 반복자를 반환
for (auto it = vec.begin(); it != vec.end(); ) {
    if (bad(*it))
        it = vec.erase(it);  // erase가 다음 유효한 반복자 반환
    else
        ++it;
}

// ★ 더 간단: erase-remove 관용구
vec.erase(
    std::remove_if(vec.begin(), vec.end(), [](auto &x) { return bad(x); }),
    vec.end()
);

// ★ C++20: 가장 간단
std::erase_if(vec, [](auto &x) { return bad(x); });
```

---

## 패턴 19: 참고용 완전한 클래스 — "이렇게 생긴 게 정석"

C++ 클래스를 처음부터 끝까지 제대로 작성한 참고 예제.
새 클래스를 만들 때 이 구조를 복사해서 시작하면 된다.

### 예제 A: 값 타입 클래스 (데이터를 담는 객체)

Python의 `@dataclass`에 해당. 복사/비교/출력이 가능한 객체.

```cpp
class Tensor {
public:
    // ═══ 생성자들 ═══

    // 기본 생성자 — 빈 텐서
    Tensor() = default;

    // 매개변수 생성자 — 보통 이걸로 만듦
    Tensor(std::vector<int64_t> shape, float fillValue)
        : shape_(std::move(shape))  // ★ 멤버 초기화 리스트 (본문보다 빠름)
    {
        int64_t total = 1;
        for (auto d : shape_) total *= d;
        data_.resize(total, fillValue);
    }

    // ═══ 복사/이동 — Rule of Zero가 적용됨 ═══
    // vector, string 등 RAII 멤버만 있으므로 직접 안 써도 됨
    // Tensor(const Tensor &) = default;
    // Tensor(Tensor &&) = default;
    // Tensor &operator=(const Tensor &) = default;
    // Tensor &operator=(Tensor &&) = default;

    // ═══ 공개 인터페이스 ═══

    // getter — const 메서드 (객체 상태 안 바꿈)
    const std::vector<int64_t> &shape() const { return shape_; }
    int64_t rank() const { return static_cast<int64_t>(shape_.size()); }
    int64_t numElements() const { return static_cast<int64_t>(data_.size()); }

    // 원소 접근 — const 버전과 non-const 버전
    float operator[](size_t i) const { return data_[i]; }  // 읽기
    float &operator[](size_t i) { return data_[i]; }       // 쓰기

    // 비교
    bool operator==(const Tensor &other) const {
        return shape_ == other.shape_ && data_ == other.data_;
    }

    // reshape — 자기 자신을 수정하고 참조 반환 (체이닝 가능)
    Tensor &reshape(std::vector<int64_t> newShape) {
        shape_ = std::move(newShape);
        return *this;  // ★ 체이닝: t.reshape({2,3}).fill(0)
    }

    // fill — 모든 원소를 값으로 채움
    Tensor &fill(float val) {
        std::fill(data_.begin(), data_.end(), val);
        return *this;
    }

private:
    std::vector<int64_t> shape_;
    std::vector<float> data_;
};

// 사용
Tensor t({2, 3, 4}, 0.0f);
t[0] = 1.0f;
auto s = t.shape();           // const vector<int64_t>&
t.reshape({6, 4}).fill(1.0f); // 체이닝
```

**이 클래스에서 보이는 패턴:**
```
멤버 초기화 리스트:  shape_(std::move(shape))   — 본문 대입보다 빠름
const 메서드:       shape() const              — "이 함수는 상태 안 바꿈"
operator[]:        const/non-const 오버로드     — 읽기/쓰기 구분
operator==:        const & 매개변수             — 복사 없이 비교
return *this:      T& 반환                     — 메서드 체이닝
std::move:         매개변수를 멤버로 이동        — 불필요한 복사 방지
```

### 예제 B: 상속 계층 — Interface + 구현 + Factory + 사용

Python에서 흔한 "ABC 정의 → 자식 구현 → factory로 생성" 패턴의 C++ 정석.

```cpp
// ═══════════════════════════════════════════
// ① 인터페이스 (interface.h)
// ═══════════════════════════════════════════
class Logger {
public:
    virtual ~Logger() = default;           // 가상 소멸자 필수
    virtual void log(std::string_view msg) = 0;  // 순수 가상
    virtual void flush() {}                // 기본 구현 있는 가상
};

// ═══════════════════════════════════════════
// ② 구현 클래스들 (console_logger.h, file_logger.h)
// ═══════════════════════════════════════════
class ConsoleLogger : public Logger {
public:
    void log(std::string_view msg) override {
        std::cout << "[LOG] " << msg << "\n";
    }
};

class FileLogger : public Logger {
public:
    explicit FileLogger(const std::string &path)
        : file_(path) {}  // RAII: 생성자에서 파일 열기

    void log(std::string_view msg) override {
        file_ << "[LOG] " << msg << "\n";
    }

    void flush() override {
        file_.flush();
    }

    // 소멸자 안 써도 됨 — ofstream이 RAII로 자동 닫힘

private:
    std::ofstream file_;
};

// ═══════════════════════════════════════════
// ③ Factory 함수
// ═══════════════════════════════════════════
std::unique_ptr<Logger> createLogger(const std::string &type,
                                      const std::string &path = "") {
    if (type == "console") return std::make_unique<ConsoleLogger>();
    if (type == "file")    return std::make_unique<FileLogger>(path);
    return nullptr;
}

// ═══════════════════════════════════════════
// ④ 사용하는 쪽 — Logger가 뭔지 모르고 쓸 수 있음
// ═══════════════════════════════════════════
class Application {
public:
    explicit Application(std::unique_ptr<Logger> logger)
        : logger_(std::move(logger)) {}   // 소유권을 받아옴

    void run() {
        logger_->log("Application started");
        // ...
        logger_->log("Application finished");
        logger_->flush();
    }

private:
    std::unique_ptr<Logger> logger_;  // 소유. 스코프 끝에서 자동 해제.
};

// ═══════════════════════════════════════════
// ⑤ 조립 (main 또는 test)
// ═══════════════════════════════════════════
int main() {
    auto logger = createLogger("file", "app.log");
    Application app(std::move(logger));  // 소유권 이전
    app.run();
}
```

**이 계층에서 보이는 모든 패턴:**
```
virtual ~Logger() = default    → 부모 포인터로 delete 시 자식 소멸자도 호출
= 0                           → 순수 가상, 인스턴스 생성 불가
override                      → 오타 방지
explicit                      → 암묵적 변환 방지
unique_ptr<Logger>             → 소유권 명확, 자동 해제
std::move(logger)              → 소유권 이전
std::string_view               → 문자열 복사 없이 전달
```

### 예제 C: Template + Virtual 조합 — 프레임워크 설계

MLIR의 OpConversionPattern과 같은 구조. 프레임워크가 쓰는 진짜 패턴.

```cpp
// ═══════════════════════════════════════════
// ① 비템플릿 기반 클래스 — 컨테이너에 담을 수 있음
// ═══════════════════════════════════════════
class HandlerBase {
public:
    virtual ~HandlerBase() = default;
    virtual bool canHandle(const std::string &type) const = 0;
    virtual void execute(const Data &data) = 0;
};

// ═══════════════════════════════════════════
// ② 템플릿 중간 클래스 — 타입별 boilerplate 제거
// ═══════════════════════════════════════════
template <typename DataT>
class TypedHandler : public HandlerBase {
public:
    bool canHandle(const std::string &type) const override {
        return type == DataT::typeName();  // 컴파일 타임에 타입 이름 결정
    }

    void execute(const Data &data) override {
        // 안전하게 캐스트 후 자식의 handle() 호출
        const auto &typed = static_cast<const DataT &>(data);
        handle(typed);  // 자식이 구현할 순수 가상 함수
    }

    virtual void handle(const DataT &data) = 0;  // 자식이 이것만 구현하면 됨
};

// ═══════════════════════════════════════════
// ③ 구체 핸들러 — 사용자가 작성하는 부분
// ═══════════════════════════════════════════
class ImageHandler : public TypedHandler<ImageData> {
    void handle(const ImageData &img) override {
        // ImageData 전용 로직
        resize(img);
    }
};

class TextHandler : public TypedHandler<TextData> {
    void handle(const TextData &txt) override {
        // TextData 전용 로직
        tokenize(txt);
    }
};

// ═══════════════════════════════════════════
// ④ 프레임워크 — 핸들러를 모아서 실행
// ═══════════════════════════════════════════
class Pipeline {
    std::vector<std::unique_ptr<HandlerBase>> handlers;  // 다형성 컨테이너
public:
    template <typename H>
    void addHandler() {
        handlers.push_back(std::make_unique<H>());  // 타입으로 등록
    }

    void process(const Data &data, const std::string &type) {
        for (auto &h : handlers) {
            if (h->canHandle(type)) {
                h->execute(data);
                return;
            }
        }
    }
};

// 사용
Pipeline pipeline;
pipeline.addHandler<ImageHandler>();
pipeline.addHandler<TextHandler>();
pipeline.process(someData, "image");
```

**이 구조의 핵심:**
```
HandlerBase         — 비템플릿 → vector<unique_ptr<HandlerBase>>에 담을 수 있음
TypedHandler<DataT> — 템플릿 → 타입별 boilerplate (canHandle, 캐스트)를 한 번만 작성
ImageHandler        — 구체 → handle()만 구현하면 됨

이것이 MLIR의 구조와 동일:
  HandlerBase       = RewritePattern
  TypedHandler<T>   = OpConversionPattern<T>
  ImageHandler      = ConvOpLowering
  Pipeline          = applyPartialConversion
```

---

---

# 함정 모음 (Pitfalls) — Python에서는 절대 안 생기는 버그

Python 경험만 있으면 이 함정들을 예측할 수 없다.
"왜 crash가 나는지 모르겠다"의 80%가 여기에 해당한다.

---

## 함정 1: Dangling Reference — 사라진 객체를 가리키는 참조

**Python에서는:** 가비지 컬렉터가 참조 카운트를 관리하므로 절대 발생 안 함.

**C++에서는:** 로컬 변수가 스코프를 벗어나면 메모리가 사라진다.

```cpp
// ═══════════════════════════════════════════
// ✗ 함정: 로컬 변수의 참조를 반환
// ═══════════════════════════════════════════
std::string& getName() {
    std::string name = "alice";
    return name;  // ★ name은 이 함수가 끝나면 사라짐!
}
// 호출자가 받은 참조는 이미 해제된 메모리를 가리킴 → crash 또는 쓰레기 값

// ═══════════════════════════════════════════
// ✗ 함정: 포인터도 동일
// ═══════════════════════════════════════════
int* getPointer() {
    int x = 42;
    return &x;  // x는 스택에 있고, 함수 끝나면 사라짐
}

// ═══════════════════════════════════════════
// ✗ 함정: string_view/StringRef로 임시 객체를 가리킴
// ═══════════════════════════════════════════
std::string_view getView() {
    std::string s = "hello";
    return s;  // s가 사라지면 view도 댕글링!
}

// ═══════════════════════════════════════════
// ✗ 함정: 람다가 로컬 변수를 참조 캡처
// ═══════════════════════════════════════════
std::function<int()> makeCounter() {
    int count = 0;
    return [&count]() { return ++count; };
    //      ^^^^^^ count가 사라진 후 호출하면 crash!
}

// ═══════════════════════════════════════════
// ★ 올바른 방법들
// ═══════════════════════════════════════════
std::string getName() {             // 값으로 반환 (복사 또는 move)
    std::string name = "alice";
    return name;                    // 컴파일러가 move 최적화 (NRVO)
}

std::function<int()> makeCounter() {
    int count = 0;
    return [count]() mutable { return ++count; };
    //      ^^^^^ 값 캡처 → 복사본이 람다 안에 살아있음
}
```

**규칙: 이것만 기억하면 된다**
```
반환할 때:  참조/포인터/view로 로컬 변수를 반환하지 않는다 → 값으로 반환
캡처할 때:  람다가 함수 밖으로 나가면 [&] 쓰지 않는다 → [=] 또는 값 캡처
저장할 때:  string_view/StringRef를 멤버로 저장하지 않는다 → std::string으로 저장
```

---

## 함정 2: Object Slicing — 자식을 부모로 복사하면 잘림

**Python에서는:** 모든 게 참조이므로 절대 발생 안 함.

**C++에서는:** 값 복사가 기본이므로 자식 객체를 부모 타입에 넣으면 자식 부분이 잘린다.

```cpp
class Animal {
public:
    virtual ~Animal() = default;
    virtual std::string speak() const { return "..."; }
    std::string name;
};

class Dog : public Animal {
public:
    std::string speak() const override { return "멍멍"; }
    std::string breed;  // Dog만 가진 필드
};

// ═══════════════════════════════════════════
// ✗ 슬라이싱: 값으로 복사하면 Dog 부분이 잘림
// ═══════════════════════════════════════════
Dog dog;
dog.name = "바둑이";
dog.breed = "진돗개";

Animal a = dog;          // ★ 복사! breed가 사라짐, speak()도 Animal의 것으로
a.speak();               // "..." — Dog::speak()가 아님!

// 함수 매개변수에서도 발생
void process(Animal a) { a.speak(); }  // 슬라이싱!
process(dog);                           // "..." — Dog가 아니라 Animal

// vector에서도 발생
std::vector<Animal> animals;
animals.push_back(dog);  // 슬라이싱! Dog → Animal로 잘림

// ═══════════════════════════════════════════
// ★ 올바른 방법: 참조 또는 포인터
// ═══════════════════════════════════════════
void process(const Animal &a) { a.speak(); }  // "멍멍" — 정상
process(dog);

Animal &ref = dog;
ref.speak();  // "멍멍" — 정상

// vector는 unique_ptr로
std::vector<std::unique_ptr<Animal>> animals;
animals.push_back(std::make_unique<Dog>());
animals[0]->speak();  // "멍멍" — 정상
```

**규칙:**
```
상속 관계에서는 절대 값으로 복사하지 않는다.
항상 참조(const Base &) 또는 포인터(unique_ptr<Base>)를 사용한다.
```

---

## 함정 3: 멤버 초기화 순서 — 선언 순서대로, 초기화 리스트 순서가 아님

```cpp
class Buffer {
    size_t size_;           // ① 먼저 선언됨
    std::vector<int> data_; // ② 나중에 선언됨

public:
    // ✗ 초기화 리스트 순서와 실제 초기화 순서가 다름!
    Buffer(size_t n)
        : data_(n, 0)   // 이것을 먼저 쓴 것 같지만...
        , size_(n)       // 실제로는 size_가 먼저 초기화됨 (선언 순서)
    {}
    // 이 예에서는 둘 다 독립적이라 문제없지만...

    // ✗ 위험한 경우: 먼저 초기화되는 멤버가 아직 안 된 멤버를 참조
    // Buffer(size_t n)
    //     : data_(size_, 0)  // size_가 초기화되기 전에 사용! → 쓰레기 값!
    //     , size_(n)
    // {}
};
```

**규칙:**
```
1. 멤버 초기화 순서 = 클래스 본문의 선언 순서 (초기화 리스트 순서 무관)
2. 초기화 리스트도 선언 순서대로 쓴다 (컴파일러가 -Wreorder 경고)
3. 멤버 A가 멤버 B에 의존하면, A를 B 뒤에 선언한다
```

---

## 함정 4: Implicit Conversion — 생성자가 몰래 타입을 바꿈

**Python에서는:** 암묵적 변환이 거의 없음 (`int + str`이면 TypeError).

**C++에서는:** 인자 1개짜리 생성자는 암묵적 변환을 허용한다.

```cpp
class Meter {
public:
    Meter(double value) : val_(value) {}  // double → Meter 자동 변환!
    double val_;
};

void printLength(Meter m) {
    std::cout << m.val_ << "m\n";
}

// ✗ 이게 컴파일됨 — double이 몰래 Meter로 변환
printLength(3.14);     // Meter(3.14)로 암묵적 변환
printLength(42);       // Meter(42.0)으로 암묵적 변환
// 개발자가 의도하지 않은 변환이 조용히 일어남

// ═══════════════════════════════════════════
// ★ explicit으로 방지
// ═══════════════════════════════════════════
class Meter {
public:
    explicit Meter(double value) : val_(value) {}  // explicit!
    double val_;
};

printLength(3.14);           // ✗ 컴파일 에러! 암묵적 변환 불가
printLength(Meter(3.14));    // ★ 명시적 변환만 허용
```

**규칙:**
```
인자 1개짜리 생성자에는 무조건 explicit을 붙인다.
예외: 복사/이동 생성자, 의도적인 변환 클래스 (예: string(const char*))
```

**Gawee에서의 예:**
```cpp
// MLIREmitter.h
explicit MLIREmitter(MLIRContext *context);
//       ^^^^^^^^ MLIRContext*가 몰래 MLIREmitter로 변환되는 것을 방지
```

---

## 함정 5: 헤더와 구현 분리 — 어디에 코드를 넣는가

**Python에서는:** 파일 하나에 전부 넣으면 됨. import하면 끝.

**C++에서는:** 헤더(.h)와 구현(.cpp)을 분리해야 하고, 규칙이 까다롭다.

```
규칙: 헤더는 "여러 .cpp에서 include"된다.
      → 헤더에 함수 본문을 넣으면 "여러 번 정의" 에러 발생
```

```cpp
// ═══════════════════════════════════════════
// ★ 일반 클래스: .h에 선언, .cpp에 구현
// ═══════════════════════════════════════════

// --- logger.h ---
#ifndef LOGGER_H
#define LOGGER_H
class Logger {
public:
    explicit Logger(const std::string &path);
    void log(std::string_view msg);    // 선언만
private:
    std::ofstream file_;
};
#endif

// --- logger.cpp ---
#include "logger.h"
Logger::Logger(const std::string &path) : file_(path) {}
void Logger::log(std::string_view msg) {
    file_ << msg << "\n";             // 구현
}

// ═══════════════════════════════════════════
// ★ 헤더에 구현을 넣어도 되는 경우
// ═══════════════════════════════════════════

// ① inline 함수 (짧은 getter)
class Tensor {
public:
    int64_t rank() const { return shape_.size(); }  // OK — 암묵적 inline
};

// ② 템플릿 — 반드시 헤더에
template <typename T>
class Stack {
public:
    void push(const T &val) { data_.push_back(val); }  // 헤더에 있어야 함
    // .cpp에 넣으면 → 다른 파일에서 Stack<int>를 쓸 때 링크 에러
private:
    std::vector<T> data_;
};

// ═══════════════════════════════════════════
// ✗ 흔한 에러들
// ═══════════════════════════════════════════

// 에러: "multiple definition of Logger::log"
// 원인: .h에 non-inline 함수 본문을 넣고 여러 .cpp에서 include
// 해결: .cpp로 옮기거나 inline 키워드 추가

// 에러: "undefined reference to Stack<int>::push"
// 원인: 템플릿 구현을 .cpp에 넣음
// 해결: 헤더에 구현을 넣음
```

**판단 기준:**
```
.h에 넣는 것:
  - 클래스 선언 (멤버 목록)
  - 인라인 함수 (짧은 getter/setter)
  - 템플릿 전체 (선언 + 구현)
  - constexpr 함수

.cpp에 넣는 것:
  - 길고 복잡한 함수 구현
  - static 변수 정의
  - 익명 namespace 안의 헬퍼

기억하기 쉽게:
  "다른 파일이 내 타입을 쓸 수 있어야 한다" → .h
  "다른 파일이 내 구현을 알 필요 없다"       → .cpp
```

---

## 한눈에 보기

| # | 패턴 | 핵심 한줄 |
|---|---|---|
| 1 | RAII | 소멸자가 자원 해제 → `with`문 필요 없음 |
| 2 | Rule of Zero | 멤버를 RAII 타입으로 → 생성자/소멸자 안 써도 됨 |
| 3 | Factory 함수 | `make_unique<자식>()` 반환 |
| 4 | enum class | 타입 안전 열거형 + switch |
| 5 | const T& | 큰 객체는 읽기 전용 참조로 |
| 6 | optional | 실패할 수 있으면 `optional` 반환 |
| 7 | 콜백 | 바로 호출 → 템플릿, 저장 → `std::function` |
| 8 | Structured bindings | `auto [a, b] = ...` |
| 9 | Virtual Interface | `virtual = 0` + `override` + 가상 소멸자 |
| 10 | string_view | 문자열 복사 없이 빌려보기 |
| 11 | Template 함수/클래스 | 타입을 매개변수로 → 코드 재사용 |
| 12 | constexpr | 컴파일 타임 상수/함수 |
| 13 | static_assert | 컴파일 타임 조건 검증 |
| 14 | Scope Guard | 블록 끝에서 반드시 실행 |
| 15 | Aggregate | struct를 dict처럼 초기화 |
| 16 | CRTP | 부모가 자식 타입을 컴파일 타임에 앎 |
| 17 | 이동 + emplace | `push_back(move(x))` vs `emplace_back(args...)` |
| 18 | 반복자 무효화 방지 | `erase-remove` 관용구 |
| 19A | 값 타입 클래스 | 초기화 리스트 + const 메서드 + operator |
| 19B | 상속 계층 | Interface → 구현 → Factory → 사용 |
| 19C | Template+Virtual | 비템플릿 Base + 템플릿 중간층 + 구체 구현 |
| **함정** | | |
| P1 | Dangling Reference | 로컬 변수의 참조/포인터를 반환하면 crash |
| P2 | Object Slicing | 자식을 부모 값으로 복사하면 자식 부분이 잘림 |
| P3 | 초기화 순서 | 멤버는 선언 순서대로 초기화 (리스트 순서 아님) |
| P4 | Implicit Conversion | `explicit` 없으면 생성자가 몰래 타입 변환 |
| P5 | 헤더/구현 분리 | 템플릿은 .h에, 나머지 구현은 .cpp에 |

---

# 3장. C++ 디자인 패턴 — MLIR에서 쓰이는 것만

디자인 패턴 책을 읽을 필요는 없다.
MLIR 코드에서 반복적으로 나타나는 패턴 4가지만 이해하면 된다.

---

## 3.1 패턴 1: Builder 패턴

**문제**: 복잡한 객체를 만들 때 생성자에 매개변수가 너무 많다.

**해결**: Builder 객체가 하나씩 설정하고 최종 결과를 돌려준다.

```cpp
// MLIR의 OpBuilder
auto convOp = builder->create<ConvOp>(
    loc,            // 위치 정보
    resultType,     // 결과 타입
    input,          // 입력 텐서
    weight,         // 가중치 텐서
    bias,           // 바이어스
    strides,        // 속성
    padding,
    dilation
);
```

OpBuilder의 역할:
- 어디에 연산을 삽입할지 관리 (`setInsertionPointToStart`)
- 타입 생성 헬퍼 (`getFunctionType`, `getDenseI64ArrayAttr`)
- 연산 생성 (`create<Op>(...)`)

**핵심**: Builder는 **상태를 가진다** (현재 삽입 위치).
그래서 순서가 중요하다 — `setInsertionPoint`를 호출한 후에 `create`를 해야 한다.

---

## 3.2 패턴 2: Visitor / Dispatch 패턴

**문제**: 여러 종류의 객체를 종류별로 다르게 처리해야 한다.

**해결**: if-else 또는 switch로 타입을 확인하고 분기한다.

```cpp
// MLIREmitter::emitNode — Visitor 패턴
bool MLIREmitter::emitNode(const json::Object &node, ...) {
  auto opType = node.getString("op_type");

  if (*opType == "Conv")     return emitConv(node, values);
  if (*opType == "Relu")     return emitRelu(node, values);
  if (*opType == "Add")      return emitAdd(node, values);
  if (*opType == "MaxPool")  return emitMaxPool(node, values);
  // ...
}
```

**이 패턴이 반복되는 곳**:
1. `MLIREmitter::emitNode` — JSON op_type별 분기
2. `GaweeToLinalgPass::runOnOperation` — 패턴을 등록하면 MLIR이 자동 dispatch
3. Python `Mapper.translate` — PyTorch op → Gawee op 매핑

**개선 방향**: 새 op을 추가할 때마다 if-else를 하나 추가해야 한다.
이게 싫으면 등록(registration) 패턴을 쓴다 (MLIR의 ConversionPattern이 이것).

---

## 3.3 패턴 3: Registry 패턴 (등록 패턴)

**문제**: Visitor 패턴은 새로운 종류를 추가할 때마다 분기문을 수정해야 한다.

**해결**: 처리기를 미리 등록해두고, 프레임워크가 자동으로 매칭한다.

```cpp
// MLIR의 ConversionPattern 등록
void runOnOperation() override {
  RewritePatternSet patterns(ctx);

  // 패턴을 "등록"
  patterns.add<ConvOpLowering>(ctx);    // gawee.conv → linalg.conv
  patterns.add<ReluOpLowering>(ctx);    // gawee.relu → linalg.generic
  patterns.add<AddOpLowering>(ctx);     // gawee.add  → linalg.add

  // 프레임워크가 자동으로 매칭하고 실행
  applyPartialConversion(module, target, std::move(patterns));
}
```

**Visitor vs Registry**:

| | Visitor (emitNode) | Registry (patterns.add) |
|---|---|---|
| 분기 방식 | 내가 if-else 작성 | 프레임워크가 자동 매칭 |
| 새 op 추가 | if-else 추가 | patterns.add 한 줄 추가 |
| 유연성 | 낮음 | 높음 (우선순위, 조건 등 지원) |

---

## 3.4 패턴 4: Template Method 패턴

**문제**: 전체 알고리즘 구조는 같지만, 세부 단계만 다르다.

**해결**: 부모 클래스가 전체 구조를 정의하고,
자식 클래스가 특정 단계만 override한다.

```cpp
// MLIR의 OpConversionPattern
// 부모: 매칭 알고리즘, 타입 변환, 에러 처리 등을 제공
// 자식: matchAndRewrite만 구현

struct ConvOpLowering : public OpConversionPattern<gawee::ConvOp> {
  using OpConversionPattern::OpConversionPattern;  // 부모 생성자 상속

  // 이것만 구현하면 됨 — 나머지는 부모가 처리
  LogicalResult
  matchAndRewrite(gawee::ConvOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    // 내 변환 로직
  }
};
```

**이 패턴이 쓰이는 곳**:
- 모든 `OpConversionPattern<T>` 상속 구조
- `PassWrapper<T, OperationPass<ModuleOp>>` 구조

**왜 강력한가?**
`matchAndRewrite` 하나만 구현하면
매칭, 타입 변환, 에러 처리, 롤백 등은 MLIR 프레임워크가 전부 처리한다.

---

## 3.5 실전: 새로운 Op Lowering을 추가할 때의 체크리스트

```
1. OpConversionPattern<gawee::NewOp>을 상속하는 struct 작성
2. using OpConversionPattern::OpConversionPattern; 추가
3. matchAndRewrite 구현:
   a. adaptor에서 입력값 가져오기
   b. op에서 속성 가져오기
   c. 출력 텐서 생성 (tensor::EmptyOp)
   d. linalg 연산 생성
   e. rewriter.replaceOp(op, result)
4. runOnOperation()에서 patterns.add<NewOpLowering>(ctx) 추가
```

---

## 암기 정리표

```
┌─────────────────────────────────────────────────────┐
│ Builder 패턴                                         │
│   OpBuilder가 상태(삽입 위치)를 관리하며 연산 생성     │
│   create<Op>() / setInsertionPoint() 순서 중요       │
├─────────────────────────────────────────────────────┤
│ Visitor 패턴                                         │
│   op_type별 if-else 분기 (emitNode)                  │
│   새 op 추가 = if문 추가                              │
├─────────────────────────────────────────────────────┤
│ Registry 패턴                                        │
│   patterns.add<T>(ctx) 로 등록                       │
│   applyPartialConversion이 자동 매칭                  │
│   새 op 추가 = patterns.add 한 줄 추가                │
├─────────────────────────────────────────────────────┤
│ Template Method 패턴                                 │
│   부모: 전체 구조 (매칭, 에러처리, 롤백)              │
│   자식: matchAndRewrite 하나만 구현                   │
│   OpConversionPattern<T>가 대표적 예시               │
└─────────────────────────────────────────────────────┘
```

---

# 4장. CRTP — Curiously Recurring Template Pattern

MLIR 코드에서 가장 혼란스러운 C++ 패턴이다.
이해하지 않으면 `PassWrapper`나 `OpConversionPattern`의 구조를 절대 읽을 수 없다.

---

## 4.1 문제 상황

다음 코드를 보자:

```cpp
struct GaweeToLinalgPass
    : public PassWrapper<GaweeToLinalgPass, OperationPass<ModuleOp>> {
//                       ^^^^^^^^^^^^^^^^^
//    자기 자신을 템플릿 인자로 넘긴다??
```

"자기 자신의 타입을 부모 클래스에 넘기다니, 이게 무슨 뜻인가?"

---

## 4.2 CRTP란

```cpp
template <typename Derived>
class Base {
  // Derived(자식) 타입을 컴파일 타임에 알고 있음
};

class Child : public Base<Child> {
  // ↑ 자기 자신을 Base에 전달
};
```

**핵심**: 부모 클래스가 자식 클래스의 타입을 **컴파일 타임에** 알 수 있다.

---

## 4.3 왜 필요한가 — 구체적 예시

### CRTP 없이 (virtual 사용)

```cpp
class Pass {
public:
  virtual void runOnOperation() = 0;  // 런타임에 어떤 자식인지 확인

  void execute() {
    // 준비 작업
    runOnOperation();  // virtual call → 런타임 비용
    // 정리 작업
  }
};
```

### CRTP 사용

```cpp
template <typename Derived>
class PassWrapper {
public:
  void execute() {
    // 준비 작업
    static_cast<Derived*>(this)->runOnOperation();  // 컴파일 타임에 결정!
    // 정리 작업
  }
};

struct MyPass : public PassWrapper<MyPass> {
  void runOnOperation() { /* ... */ }
};
```

**차이점**:
- `virtual`: 런타임에 vtable을 통해 함수를 찾음 (약간의 오버헤드)
- CRTP: 컴파일 타임에 이미 어떤 함수를 호출할지 결정됨 (오버헤드 없음)

---

## 4.4 MLIR에서의 CRTP

### PassWrapper

```cpp
struct GaweeToLinalgPass
    : public PassWrapper<GaweeToLinalgPass, OperationPass<ModuleOp>> {
//           ^^^^^^^^^^^  ^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^^^^^^^^^
//           CRTP Base    자기 자신 (Derived)  Pass가 동작하는 대상
```

`PassWrapper`가 `GaweeToLinalgPass`의 타입을 알기 때문에:
- Pass 등록할 때 고유 ID를 자동 생성
- `std::make_unique<GaweeToLinalgPass>()`를 내부에서 자동 처리
- 타입 정보 기반으로 최적화 가능

### OpConversionPattern

```cpp
struct ConvOpLowering : public OpConversionPattern<gawee::ConvOp> {
//                             ^^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^
//                             CRTP Base              매칭할 Op 타입
```

여기서 `OpConversionPattern<gawee::ConvOp>`는 엄밀히 CRTP가 아니라
**템플릿 특수화**이다. 하지만 원리는 같다:
부모가 `gawee::ConvOp` 타입을 컴파일 타임에 알고 있어서,
자동으로 매칭 조건을 생성하고, `OpAdaptor`를 만들어준다.

---

## 4.5 단순화해서 이해하기

CRTP의 핵심을 한 마디로:

> **"부모에게 내가 누구인지 미리 알려주는 것"**

Python으로 비유하면:

```python
# 불가능한 Python 코드 (개념 비유)
class Base:
    def __init_subclass__(cls):
        cls.type_id = cls.__name__  # 자식 타입을 자동으로 알고 처리

class MyPass(Base):
    pass

print(MyPass.type_id)  # "MyPass" — Base가 자동으로 설정
```

---

## 4.6 CRTP를 볼 때의 읽기 규칙

```cpp
struct A : public B<A, C> {
```

이것을 보면 이렇게 읽는다:
1. `A`는 `B`를 상속한다
2. `B`는 `A`의 타입 정보를 컴파일 타임에 안다
3. `C`는 추가 설정 (예: 어떤 연산에 동작하는지)

MLIR에서 이 패턴을 볼 때마다:
- **첫 번째 템플릿 인자가 자기 자신** → CRTP
- **첫 번째 템플릿 인자가 Op 타입** → 템플릿 특수화 (비슷한 원리)

---

## 암기 정리표

```
┌─────────────────────────────────────────────────────┐
│ CRTP = 자기 자신의 타입을 부모에게 전달하는 패턴       │
│                                                     │
│ class Child : public Base<Child>                    │
│                          ^^^^^                      │
│                          자기 자신                    │
├─────────────────────────────────────────────────────┤
│ 왜 쓰는가?                                           │
│   부모가 자식 타입을 컴파일 타임에 알 수 있음          │
│   → virtual 없이 자식 함수 호출 가능 (성능 이점)       │
│   → 자동 ID 생성, 자동 매칭 등 프레임워크 기능        │
├─────────────────────────────────────────────────────┤
│ MLIR에서의 사용                                      │
│   PassWrapper<MyPass, OperationPass<ModuleOp>>      │
│   → MyPass 타입으로 Pass 등록/생성 자동화             │
│                                                     │
│   OpConversionPattern<gawee::ConvOp>                │
│   → ConvOp에 대한 매칭/adaptor 자동 생성              │
├─────────────────────────────────────────────────────┤
│ 읽기 규칙                                            │
│   struct A : public B<A, C>                         │
│   → A가 B를 상속, B가 A의 타입을 앎, C는 추가 설정    │
└─────────────────────────────────────────────────────┘
```

---

# 5장. MLIR 핵심 개념 — Dialect 정의부터 Lowering까지

MLIR은 "Multi-Level Intermediate Representation"이다.
여러 단계(level)의 IR을 하나의 프레임워크에서 다루는 것이 핵심이다.

이 장에서는 Gawee 프로젝트에서 실제로 사용한 개념만 다룬다.

---

## 5.1 전체 구조 — 한눈에 보기

```
┌──────────────┐      ┌──────────────┐      ┌──────────────┐
│  Gawee 방언   │ ──→  │  Linalg 방언  │ ──→  │  LLVM 방언    │
│  (고수준)      │      │  (중간수준)    │      │  (저수준)      │
└──────────────┘      └──────────────┘      └──────────────┘
  gawee.conv            linalg.conv_2d        llvm.call
  gawee.relu            linalg.generic        llvm.store
  gawee.add             linalg.add            llvm.load
```

**핵심 아이디어**: 각 단계마다 적절한 추상화 수준(level)을 사용한다.
- 고수준: 신경망 연산 (Conv, Relu, Add)
- 중간수준: 선형대수 연산 (행렬곱, 원소별 연산)
- 저수준: 기계어에 가까운 연산 (메모리 읽기/쓰기)

이 단계를 내려가는 것을 **Lowering**이라 한다.

---

## 5.2 Dialect (방언) — MLIR의 네임스페이스

### 방언이란

방언은 **연산(Op)의 묶음**이다. 각 방언은 특정 추상화 수준의 연산들을 모아놓는다.

```
arith 방언:  arith.addf, arith.muli, arith.constant, ...
linalg 방언: linalg.conv_2d_nchw_fchw, linalg.generic, linalg.add, ...
tensor 방언: tensor.empty, tensor.pad, tensor.collapse_shape, ...
gawee 방언:  gawee.conv, gawee.relu, gawee.add, ...
```

**비유**: Python에서 `import numpy`를 하면 `numpy.add`, `numpy.matmul` 등을 쓸 수 있듯이,
MLIR에서 `gawee` 방언을 등록하면 `gawee.conv`, `gawee.relu` 등을 쓸 수 있다.

### 방언 정의

방언은 TableGen(`.td` 파일)으로 선언한다:

```cpp
// GaweeDialect.td
def Gawee_Dialect : Dialect {
  let name = "gawee";                    // IR에서 접두사: gawee.conv, gawee.relu
  let cppNamespace = "::mlir::gawee";    // C++ 네임스페이스
  let summary = "Gawee dialect for neural network graphs";
}
```

그리고 C++에서 이 생성된 코드를 include한다:

```cpp
// GaweeDialect.h
#include "generated/GaweeDialect.h.inc"  // TableGen이 생성한 방언 선언

#define GET_OP_CLASSES
#include "generated/GaweeOps.h.inc"      // TableGen이 생성한 Op 선언
```

---

## 5.3 Operation (연산) — MLIR의 기본 단위

### Op의 구조

MLIR IR의 모든 것은 **Operation**이다. 하나의 Op는 다음으로 구성된다:

```
%result = gawee.conv %input, %weight, %bias {strides = [2, 2], padding = [3, 3], dilation = [1, 1]}
          : tensor<1x3x224x224xf32>, tensor<64x3x7x7xf32>, tensor<64xf32> -> tensor<1x64x112x112xf32>

분해:
  %result          → 결과값 (SSA Value)
  gawee.conv       → Op 이름 (방언.연산)
  %input, %weight  → 입력값 (Operands)
  {strides = ...}  → 속성 (Attributes) — 컴파일 타임 상수
  tensor<...>      → 타입 (Types)
```

### SSA (Static Single Assignment)

MLIR의 모든 값은 **정확히 한 번만 정의**된다.

```mlir
// 올바른 SSA
%0 = gawee.conv %input, %weight, %bias ...
%1 = gawee.relu %0 ...
%2 = gawee.add %1, %shortcut ...

// 잘못된 SSA (같은 변수를 두 번 정의)
%0 = gawee.conv ...
%0 = gawee.relu %0 ...  // 에러! %0을 다시 정의할 수 없다
```

**왜 SSA인가?** 각 값이 어디서 만들어졌는지 항상 추적 가능하다.
이것이 최적화와 변환을 쉽게 만든다.

### Operands vs Attributes

**핵심 구분**: 이것을 정확히 이해해야 한다.

| | Operand | Attribute |
|---|---|---|
| 무엇인가 | 다른 Op의 결과값 | 컴파일 타임 상수 |
| 언제 결정 | 런타임 | 컴파일 타임 |
| 예시 | `%input`, `%weight` | `strides = [2, 2]` |
| TableGen | `AnyTensor:$input` | `DenseI64ArrayAttr:$strides` |

```
gawee.conv %input, %weight, %bias {strides = [2,2], padding = [3,3], dilation = [1,1]}
           ^^^^^^^^^^^^^^^^^^^^^^^^ ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
           Operands (SSA Values)    Attributes (상수)
```

`%input`은 이전 연산의 결과이므로 런타임에 결정된다.
`strides = [2,2]`는 모델 구조에서 고정된 값이므로 컴파일 타임 상수이다.

---

## 5.4 Type 시스템

MLIR의 타입은 계층적이다:

```
Type
├── IntegerType (i32, i64, ...)
├── FloatType (f16, f32, f64, ...)
├── IndexType (인덱스용 정수)
└── ShapedType
    ├── RankedTensorType    tensor<1x3x224x224xf32>
    ├── UnrankedTensorType  tensor<*xf32>
    └── MemRefType          memref<1x3x224x224xf32>
```

### Tensor vs MemRef

| | Tensor | MemRef |
|---|---|---|
| 의미 | 불변 값 (수학적 텐서) | 메모리 버퍼 (읽기/쓰기 가능) |
| 비유 | Python의 `tuple` | Python의 `list` |
| 단계 | 고수준 (Gawee, Linalg) | 저수준 (Bufferization 이후) |

Gawee 프로젝트에서의 흐름:
```
Gawee (tensor) → Linalg (tensor) → Bufferization → MemRef → LLVM
```

Bufferization은 "tensor를 memref로 바꾸는" 과정이다.
이것은 MLIR 프레임워크가 자동으로 해준다.

---

## 5.5 Region과 Block — Op의 내부 구조

일부 Op은 내부에 코드를 가진다. 이것을 **Region**과 **Block**이라 한다.

```mlir
func.func @forward(%input: tensor<1x3x224x224xf32>) -> tensor<1x64x112x112xf32> {
  // ← 여기가 Region > Block
  %0 = gawee.conv ...
  %1 = gawee.relu %0 ...
  return %1 : tensor<1x64x112x112xf32>
}
```

```
func.func         ← Operation
  └── Region      ← 코드 블록들의 컨테이너
      └── Block   ← 실제 연산들의 리스트
          ├── gawee.conv
          ├── gawee.relu
          └── return
```

### linalg.generic의 body도 Region이다

```cpp
auto genericOp = linalg::GenericOp::create(
    rewriter, loc, ...,
    [&](OpBuilder &builder, Location loc, ValueRange args) {
      // 이 람다가 Region의 Block 내용을 만든다
      Value result = arith::MaximumFOp::create(builder, loc, args[0], zero);
      linalg::YieldOp::create(builder, loc, result);
    }
);
```

이 C++ 코드가 생성하는 MLIR IR:

```mlir
linalg.generic ... {
  ^bb0(%arg0: f32, %arg1: f32):   // ← Block (^bb0)
    %max = arith.maximumf %arg0, %cst : f32
    linalg.yield %max : f32
}
```

---

## 5.6 Lowering (변환) — 핵심 메커니즘

Lowering은 **고수준 Op을 저수준 Op으로 변환**하는 것이다.
MLIR에서 이것을 구현하려면 3가지가 필요하다:

### 1. ConversionTarget — "무엇이 합법인가?"

```cpp
ConversionTarget target(*ctx);
target.addLegalDialect<linalg::LinalgDialect>();   // linalg은 OK
target.addLegalDialect<arith::ArithDialect>();     // arith은 OK
target.addLegalDialect<tensor::TensorDialect>();   // tensor는 OK
target.addIllegalDialect<gawee::GaweeDialect>();   // gawee는 전부 변환해야 함
```

**비유**: 입국 심사와 같다.
- "linalg 여권" → 통과
- "gawee 여권" → 입국 불가. linalg로 변환(환전)해야 한다.

### 2. ConversionPattern — "어떻게 변환하는가?"

각 Op마다 하나의 패턴을 작성한다:

```cpp
struct ReluOpLowering : public OpConversionPattern<gawee::ReluOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(gawee::ReluOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    // 1. 기존 Op에서 정보 추출
    Value input = adaptor.getInput();

    // 2. 새로운 Op 생성
    auto genericOp = linalg::GenericOp::create(rewriter, loc, ...);

    // 3. 기존 Op 교체
    rewriter.replaceOp(op, genericOp.getResults());

    return success();
  }
};
```

**matchAndRewrite의 3단계**:
1. **추출**: `adaptor`에서 입력값/속성을 가져온다
2. **생성**: `rewriter`로 새로운 Op을 만든다
3. **교체**: `rewriter.replaceOp`으로 기존 Op을 대체한다

### 3. applyPartialConversion — "실행하라"

```cpp
RewritePatternSet patterns(ctx);
patterns.add<ConvOpLowering>(ctx);
patterns.add<ReluOpLowering>(ctx);
patterns.add<AddOpLowering>(ctx);

if (failed(applyPartialConversion(module, target, std::move(patterns)))) {
  signalPassFailure();
}
```

이 한 줄이 하는 일:
1. 모듈의 모든 Op을 순회한다
2. 각 Op에 대해 매칭되는 패턴을 찾는다
3. 패턴이 있으면 `matchAndRewrite`를 실행한다
4. 변환 후 illegal Op이 남아있으면 실패를 보고한다

**Partial vs Full Conversion**:
- `applyPartialConversion`: illegal이 아닌 Op은 그대로 둔다
- `applyFullConversion`: 모든 Op이 legal이어야 한다

우리는 `func.func`이나 `return` 같은 Op은 변환하지 않으므로 Partial을 쓴다.

---

## 5.7 OpAdaptor — 변환 중 값 접근

`matchAndRewrite`의 두 번째 인자 `OpAdaptor`는 중요한 역할을 한다.

```cpp
LogicalResult matchAndRewrite(gawee::ConvOp op, OpAdaptor adaptor, ...) {
  // adaptor를 사용: 이미 변환된 operand를 가져온다
  Value input = adaptor.getInput();

  // op을 사용: 속성(Attribute)을 가져온다
  auto strides = op.getStridesAttr();
}
```

**왜 `op`과 `adaptor` 두 개가 필요한가?**

변환 도중에는 일부 operand가 이미 다른 타입으로 변환되었을 수 있다.
- `op.getInput()`: 원래 타입의 값 (변환 전)
- `adaptor.getInput()`: 변환된 타입의 값 (변환 후)

속성(Attribute)은 타입이 바뀌지 않으므로 `op`에서 직접 가져온다.

---

## 5.8 OpBuilder / Rewriter — Op 생성 도구

### OpBuilder

IR을 직접 만들 때 사용한다 (예: `gawee-translate`에서 JSON → MLIR 변환):

```cpp
auto builder = std::make_unique<OpBuilder>(ctx);
builder->setInsertionPointToStart(block);
auto convOp = builder->create<gawee::ConvOp>(loc, resultType, input, weight, ...);
```

### ConversionPatternRewriter

Lowering 패턴 안에서 사용한다. OpBuilder의 확장판이다:

```cpp
// OpBuilder 기능: Op 생성
Value empty = tensor::EmptyOp::create(rewriter, loc, shape, elementType);

// Rewriter 고유 기능: Op 교체
rewriter.replaceOp(op, newResults);  // 기존 Op을 새 결과로 교체
```

**핵심 차이**:
- `OpBuilder`: IR을 처음부터 만들 때
- `ConversionPatternRewriter`: 기존 IR을 변환할 때

---

## 5.9 Pass — 변환의 단위

### Pass란

Pass는 **IR 전체에 대해 특정 변환을 수행하는 단위**이다.

```cpp
struct GaweeToLinalgPass
    : public PassWrapper<GaweeToLinalgPass, OperationPass<ModuleOp>> {

  void runOnOperation() override {
    // 여기에 변환 로직 작성
    // ConversionTarget 설정
    // RewritePatternSet에 패턴 등록
    // applyPartialConversion 호출
  }
};
```

`OperationPass<ModuleOp>`는 "이 Pass는 ModuleOp(전체 모듈)에 대해 동작한다"는 의미.

### PassManager — Pass 파이프라인

여러 Pass를 순서대로 실행한다:

```cpp
PassPipelineRegistration<>(
    "gawee-to-llvm",
    "Full pipeline: Gawee -> Linalg -> SCF -> LLVM",
    [](OpPassManager &pm) {
      pm.addPass(gawee::createGaweeToLinalgPass());  // 1단계
      pm.addPass(bufferization::create...());         // 2단계
      pm.addPass(createConvertLinalgToLoopsPass());    // 3단계
      pm.addPass(createSCFToControlFlowPass());        // 4단계
      pm.addPass(createArithToLLVMConversionPass());   // 5단계
      // ...
    });
```

Gawee 프로젝트의 전체 lowering 경로:

```
gawee.conv   →  linalg.conv_2d  →  scf.for  →  cf.br   →  llvm.call
gawee.relu   →  linalg.generic  →  scf.for  →  cf.cond →  llvm.store
                                     ↑             ↑
                              Bufferization    SCF→CF 변환
                           (tensor → memref)
```

---

## 5.10 DialectRegistry — 방언 등록

사용할 방언을 MLIR 컨텍스트에 등록해야 한다:

```cpp
DialectRegistry registry;
registry.insert<gawee::GaweeDialect>();     // 우리 방언
registry.insert<linalg::LinalgDialect>();   // 변환 대상
registry.insert<arith::ArithDialect>();     // 산술 연산
registry.insert<tensor::TensorDialect>();   // 텐서 연산
// ...

return mlir::MlirOptMain(argc, argv, "Gawee Optimizer\n", registry);
```

등록하지 않은 방언의 Op은 파싱/생성할 수 없다.

---

## 5.11 linalg.generic — 가장 유연한 연산

Lowering에서 가장 자주 쓰는 Op이다. ReLU, bias 덧셈, 나눗셈 등 모두
`linalg.generic`으로 표현한다.

### 필수 구성 요소

```cpp
auto genericOp = linalg::GenericOp::create(
    rewriter, loc,
    TypeRange{outputType},          // 결과 타입
    ValueRange{input},              // inputs (읽기 전용)
    ValueRange{output},             // outputs (결과가 여기에 기록됨)
    indexingMaps,                   // 각 텐서의 접근 패턴
    iteratorTypes,                  // 각 차원의 순회 방식
    bodyBuilder                     // 각 원소에 대해 수행할 연산
);
```

### IndexingMap — 핵심 중의 핵심

IndexingMap은 "반복 변수와 텐서 인덱스의 관계"를 정의한다.

```cpp
// 4D 텐서의 identity map: (n, c, h, w) -> (n, c, h, w)
AffineMap identityMap = AffineMap::getMultiDimIdentityMap(4, ctx);

// bias의 broadcast map: (n, c, h, w) -> (c)
AffineMap biasMap = AffineMap::get(4, 0, {getAffineDimExpr(1, ctx)}, ctx);
//                                 ^  ^   ^^^^^^^^^^^^^^^^^^^^^^^^^
//                               차원수 심볼수  결과 표현식
```

bias 덧셈의 예:
```
반복 변수: (n, c, h, w) — 4중 루프

conv 접근:  (n, c, h, w) -> conv[n][c][h][w]     (identity)
bias 접근:  (n, c, h, w) -> bias[c]               (broadcast)
output:     (n, c, h, w) -> output[n][c][h][w]    (identity)

본문: output[n][c][h][w] = conv[n][c][h][w] + bias[c]
```

### IteratorType

```cpp
SmallVector<utils::IteratorType> iteratorTypes(
    rank, utils::IteratorType::parallel  // 모든 차원이 병렬
);
```

- `parallel`: 각 원소가 독립적 (elementwise 연산)
- `reduction`: 여러 원소를 하나로 합침 (sum, max 등)

---

## 암기 정리표

```
┌─────────────────────────────────────────────────────┐
│ Dialect (방언)                                       │
│   연산(Op)의 묶음. 각 방언 = 하나의 추상화 수준        │
│   gawee(고수준) → linalg(중간) → llvm(저수준)         │
├─────────────────────────────────────────────────────┤
│ Operation (연산)                                     │
│   %result = dialect.op %operands {attributes} : types│
│   Operand = 런타임 값 (SSA Value)                    │
│   Attribute = 컴파일 타임 상수                        │
├─────────────────────────────────────────────────────┤
│ Tensor vs MemRef                                    │
│   Tensor = 불변 값 (고수준)                           │
│   MemRef = 메모리 버퍼 (저수준)                       │
│   Bufferization이 tensor → memref 변환               │
├─────────────────────────────────────────────────────┤
│ Lowering 3요소                                       │
│   1. ConversionTarget  — 합법/불법 Op 정의            │
│   2. ConversionPattern — matchAndRewrite 구현         │
│   3. applyPartialConversion — 실행                   │
├─────────────────────────────────────────────────────┤
│ matchAndRewrite 3단계                                │
│   1. adaptor에서 입력값 추출                          │
│   2. rewriter로 새 Op 생성                           │
│   3. rewriter.replaceOp으로 교체                     │
├─────────────────────────────────────────────────────┤
│ OpAdaptor                                           │
│   adaptor.getX() → 변환된 operand                    │
│   op.getXAttr()  → 속성 (변환 불필요)                 │
├─────────────────────────────────────────────────────┤
│ linalg.generic 구성                                  │
│   inputs + outputs + indexingMaps + iteratorTypes    │
│   + bodyBuilder (람다로 원소별 연산 정의)              │
├─────────────────────────────────────────────────────┤
│ IndexingMap                                         │
│   identity: (n,c,h,w) → (n,c,h,w)                  │
│   broadcast: (n,c,h,w) → (c)                       │
│   reduction: (n,k) → (n) + IteratorType::reduction  │
├─────────────────────────────────────────────────────┤
│ Pass Pipeline (Gawee 전체 경로)                      │
│   Gawee → Linalg → Bufferize → SCF → CF → LLVM     │
│   고수준   선형대수   메모리화    루프  분기  기계어    │
└─────────────────────────────────────────────────────┘
```

---

## 5.12 레시피 모음 — 복사해서 수정하면 되는 패턴들

아래 코드는 GaweeToLinalg.cpp에서 실제로 사용하는 패턴을 추출한 것이다.
새로운 Lowering 패턴을 작성할 때 해당 레시피를 복사하고 수정하면 된다.

### 레시피 1: 빈 텐서 만들기 (EmptyOp + FillOp)

대부분의 linalg 연산은 결과를 기록할 "빈 텐서"가 필요하다.

```cpp
// 1. 빈 텐서 생성
Value emptyTensor = tensor::EmptyOp::create(
    rewriter, loc,
    outputType.getShape(),    // ArrayRef<int64_t>: e.g. {1, 64, 112, 112}
    elementType               // Type: e.g. f32
);

// 2. 초기값으로 채우기
Value zero = arith::ConstantOp::create(
    rewriter, loc, elementType, rewriter.getZeroAttr(elementType)
);
Value output = linalg::FillOp::create(
    rewriter, loc, zero, emptyTensor
).getResult(0);
// .getResult(0) — FillOp의 첫 번째 SSA 결과값
```

**언제 쓰는가**: Conv, Linear, MaxPool, AdAvgPool 등 거의 모든 Lowering.

### 레시피 2: Elementwise 연산 (GenericOp + identity maps + parallel)

입력과 출력의 shape가 같은 원소별 연산.

```cpp
// ReLU 예시: max(x, 0)
int64_t rank = inputType.getRank();  // 4 (NCHW)

// 모든 텐서가 같은 방식으로 접근 → identity map
SmallVector<AffineMap, 2> indexingMaps(
    2,  // inputs + outputs 개수
    AffineMap::getMultiDimIdentityMap(rank, rewriter.getContext())
);

// 모든 차원이 독립 → parallel
SmallVector<utils::IteratorType> iteratorTypes(
    rank, utils::IteratorType::parallel
);

Value output = tensor::EmptyOp::create(rewriter, loc, inputType.getShape(), elementType);

auto genericOp = linalg::GenericOp::create(
    rewriter, loc,
    TypeRange{inputType},          // 결과 타입
    ValueRange{input},             // inputs (읽기 전용)
    ValueRange{output},            // outputs (여기에 기록)
    indexingMaps,
    iteratorTypes,
    [&](OpBuilder &builder, Location loc, ValueRange args) {
        // args[0] = input 원소, args[1] = output 원소 (보통 무시)
        Value zero = arith::ConstantOp::create(
            builder, loc, elementType, builder.getZeroAttr(elementType));
        Value result = arith::MaximumFOp::create(builder, loc, args[0], zero);
        linalg::YieldOp::create(builder, loc, result);  // 결과 반환 필수!
    }
);
```

**변형하려면**: body 람다 안의 연산만 바꾸면 된다.
- 덧셈: `arith::AddFOp::create(builder, loc, args[0], args[1])`
- 나눗셈: `arith::DivFOp::create(builder, loc, args[0], countVal)`
- 곱셈: `arith::MulFOp::create(builder, loc, args[0], args[1])`

### 레시피 3: Broadcast 덧셈 (bias map)

shape가 다른 텐서 간 연산. bias `[C]`를 `[N,C,H,W]`에 더할 때.

```cpp
auto ctx = rewriter.getContext();
int64_t rank = outputType.getRank();  // 4

// bias는 채널 차원(dim 1)만 사용 → (n,c,h,w) -> (c)
AffineMap biasMap = AffineMap::get(
    rank,  // 반복 변수 개수: 4 (n, c, h, w)
    0,     // 심볼 개수: 0
    {getAffineDimExpr(1, ctx)},  // 결과: dim1만 = c
    ctx
);
AffineMap identityMap = AffineMap::getMultiDimIdentityMap(rank, ctx);

SmallVector<AffineMap> indexingMaps = {
    identityMap,   // conv 결과: (n,c,h,w) → conv[n][c][h][w]
    biasMap,       // bias:      (n,c,h,w) → bias[c]
    identityMap    // output:    (n,c,h,w) → out[n][c][h][w]
};
SmallVector<utils::IteratorType> iteratorTypes(
    rank, utils::IteratorType::parallel);

Value biasEmpty = tensor::EmptyOp::create(
    rewriter, loc, outputType.getShape(), elementType);

auto biasAdd = linalg::GenericOp::create(
    rewriter, loc,
    TypeRange{outputType},
    ValueRange{convResult, bias},   // inputs
    ValueRange{biasEmpty},          // output
    indexingMaps, iteratorTypes,
    [&](OpBuilder &builder, Location loc, ValueRange args) {
        Value result = arith::AddFOp::create(builder, loc, args[0], args[1]);
        linalg::YieldOp::create(builder, loc, result);
    }
);
```

**핵심**: `AffineMap::get(rank, 0, {getAffineDimExpr(차원번호, ctx)}, ctx)`
- dim 0 = N (배치), dim 1 = C (채널), dim 2 = H, dim 3 = W

### 레시피 4: Padding (PadOp + Region + YieldOp)

linalg.conv / linalg.pooling은 padding을 자체 처리하지 않는다.
명시적으로 PadOp을 사용해야 한다.

```cpp
int64_t padH = padding[0], padW = padding[1];
if (padH != 0 || padW != 0) {
    SmallVector<int64_t> lowPad  = {0, 0, padH, padW};  // N, C는 패딩 없음
    SmallVector<int64_t> highPad = {0, 0, padH, padW};

    // 패딩된 shape 계산
    auto inputType = mlir::cast<RankedTensorType>(input.getType());
    SmallVector<int64_t> paddedShape(inputType.getShape());
    paddedShape[2] += 2 * padH;
    paddedShape[3] += 2 * padW;
    auto paddedType = RankedTensorType::get(paddedShape, elementType);

    // PadOp 생성
    auto padOp = rewriter.create<tensor::PadOp>(
        loc, paddedType, input,
        lowPad, highPad,
        /*low=*/ValueRange{}, /*high=*/ValueRange{});

    // Body: 패딩 위치에 채울 값을 yield
    auto &region = padOp.getRegion();
    auto *block = rewriter.createBlock(&region);
    for (int i = 0; i < paddedType.getRank(); i++)
        block->addArgument(rewriter.getIndexType(), loc);
    rewriter.setInsertionPointToEnd(block);
    rewriter.create<tensor::YieldOp>(loc, padValue);  // zero 또는 negInf

    input = padOp.getResult();
    rewriter.setInsertionPointAfter(padOp);  // 삽입 위치 복원!
}
```

**padValue 선택**:
- Conv → `zero` (0으로 패딩)
- MaxPool → `negInf` (-∞로 패딩, max에서 절대 선택 안 됨)

### 레시피 5: 상수 만들기

```cpp
// 0.0 (f32)
Value zero = arith::ConstantOp::create(
    rewriter, loc, elementType, rewriter.getZeroAttr(elementType));

// -infinity
Value negInf = arith::ConstantOp::create(rewriter, loc,
    rewriter.getFloatAttr(elementType, -std::numeric_limits<double>::infinity()));

// 정수 배열 속성 (strides, dilations 등)
auto strideAttr = rewriter.getDenseI64ArrayAttr({1, 1});

// 임의의 float
Value count = arith::ConstantOp::create(rewriter, loc,
    elementType, rewriter.getFloatAttr(elementType, 49.0));
```

### 레시피 6: 타입/shape 추출 체인

```cpp
// Op의 출력 타입 → RankedTensorType → elementType, shape
auto outputType = mlir::cast<RankedTensorType>(op.getOutput().getType());
auto elementType = outputType.getElementType();      // f32
auto shape = outputType.getShape();                  // ArrayRef<int64_t>: {1, 64, 112, 112}
int64_t rank = outputType.getRank();                 // 4
int64_t H = shape[2];                               // 112

// 입력에서도 동일
auto inputType = mlir::cast<RankedTensorType>(input.getType());
```

### 레시피 7: Op 교체 (replaceOp)

```cpp
// 단일 결과 Op 교체
rewriter.replaceOp(op, newOp.getResults());

// tensor::CollapseShapeOp처럼 단일 결과면
rewriter.replaceOp(op, flattenOp.getResult());

// matchAndRewrite는 항상 success() 또는 failure() 반환
return success();
```

### 레시피 8: Pass 뼈대

새로운 Pass를 만들 때 이 뼈대를 복사한다.

```cpp
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

namespace {

// --- 패턴들 ---
struct MyOpLowering : public OpConversionPattern<MyDialect::MyOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(MyDialect::MyOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    Location loc = op.getLoc();

    // 1. adaptor에서 입력 추출
    Value input = adaptor.getInput();

    // 2. rewriter로 새 Op 생성
    // ... (레시피 1~6 활용)

    // 3. 기존 Op 교체
    rewriter.replaceOp(op, newResults);
    return success();
  }
};

// --- Pass ---
struct MyPass : public PassWrapper<MyPass, OperationPass<ModuleOp>> {
  StringRef getArgument() const override { return "my-pass-name"; }
  StringRef getDescription() const override { return "My pass description"; }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<TargetDialect>();  // 생성할 Op의 방언 등록
  }

  void runOnOperation() override {
    MLIRContext *ctx = &getContext();
    ModuleOp module = getOperation();

    // 1. 타겟 설정
    ConversionTarget target(*ctx);
    target.addLegalDialect<TargetDialect>();
    target.addIllegalDialect<SourceDialect>();

    // 2. 패턴 등록
    RewritePatternSet patterns(ctx);
    patterns.add<MyOpLowering>(ctx);

    // 3. 실행
    if (failed(applyPartialConversion(module, target, std::move(patterns))))
      signalPassFailure();
  }
};

} // namespace

// --- 등록 ---
namespace mlir::mydialect {
std::unique_ptr<Pass> createMyPass() {
  return std::make_unique<MyPass>();
}
}
```

---

# 6장. TableGen — 코드를 생성하는 코드

TableGen은 MLIR의 **코드 생성기**이다.
`.td` 파일에 "이런 Op이 있다"고 선언하면,
C++ 코드(파서, 프린터, 접근자 등)가 자동으로 생성된다.

왜 이게 필요한가?
Op 하나를 수동으로 C++로 작성하면 200줄이 넘는다.
TableGen을 쓰면 10줄이면 된다.

---

## 6.1 전체 흐름

```
GaweeDialect.td  ──→  mlir-tblgen  ──→  GaweeDialect.h.inc / GaweeDialect.cpp.inc
GaweeOps.td      ──→  mlir-tblgen  ──→  GaweeOps.h.inc / GaweeOps.cpp.inc
                                              ↓
                                    GaweeDialect.h 에서 #include
                                              ↓
                                    GaweeDialect.cpp, GaweeToLinalg.cpp 등에서 사용
```

`.td` → `.inc` → `.h`/`.cpp`

- `.td`: 내가 작성하는 선언 파일
- `.inc`: `mlir-tblgen`이 자동 생성하는 C++ 조각
- `.h`/`.cpp`: `.inc`를 include해서 사용하는 내 코드

---

## 6.2 build.sh에서 TableGen 호출

```bash
TBLGEN="$LLVM_DIR/bin/mlir-tblgen"

# 방언 선언 (class GaweeDialect : public Dialect { ... };)
$TBLGEN --gen-dialect-decls -I $LLVM_DIR/include \
  include/Gawee/GaweeDialect.td \
  -o include/Gawee/generated/GaweeDialect.h.inc

# 방언 정의 (void GaweeDialect::initialize() { ... })
$TBLGEN --gen-dialect-defs -I $LLVM_DIR/include \
  include/Gawee/GaweeDialect.td \
  -o include/Gawee/generated/GaweeDialect.cpp.inc

# Op 선언 (class ConvOp { ... }; class ReluOp { ... };)
$TBLGEN --gen-op-decls -I $LLVM_DIR/include -I include -I include/Gawee \
  include/Gawee/GaweeOps.td \
  -o include/Gawee/generated/GaweeOps.h.inc

# Op 정의 (파서, 프린터, 접근자 구현)
$TBLGEN --gen-op-defs -I $LLVM_DIR/include -I include -I include/Gawee \
  include/Gawee/GaweeOps.td \
  -o include/Gawee/generated/GaweeOps.cpp.inc
```

| 플래그 | 생성하는 것 | 비유 |
|--------|------------|------|
| `--gen-dialect-decls` | 방언 클래스 선언 (.h) | `class Dialect;` |
| `--gen-dialect-defs` | 방언 클래스 정의 (.cpp) | `Dialect::initialize() { ... }` |
| `--gen-op-decls` | Op 클래스 선언 (.h) | `class ConvOp;` |
| `--gen-op-defs` | Op 클래스 정의 (.cpp) | `ConvOp::parse() { ... }` |

---

## 6.3 .inc 파일이 사용되는 곳

### GaweeDialect.h — 선언 include

```cpp
#include "generated/GaweeDialect.h.inc"   // 방언 클래스 선언

#define GET_OP_CLASSES
#include "generated/GaweeOps.h.inc"       // Op 클래스 선언
```

`#define GET_OP_CLASSES`는 `.h.inc` 파일 안의 조건부 컴파일을 활성화한다.
이 매크로 없이 include하면 Op 클래스가 생성되지 않는다.

### GaweeDialect.cpp — 정의 include

```cpp
#include "generated/GaweeDialect.cpp.inc"  // 방언 정의

#define GET_OP_CLASSES
#include "generated/GaweeOps.cpp.inc"      // Op 정의
```

`GET_OP_CLASSES`가 두 번 나온다:
- `.h.inc`에서: 클래스 선언 (`class ConvOp { ... };`)
- `.cpp.inc`에서: 메서드 구현 (`ConvOp::verify() { ... }`)

---

## 6.4 TableGen 문법 — .td 파일 읽기

### include

```tablegen
include "mlir/IR/OpBase.td"
```

C++의 `#include`와 같다. MLIR이 제공하는 기본 정의를 가져온다.
`OpBase.td`에는 `Dialect`, `Op`, `AnyTensor`, `I64Attr` 등이 정의되어 있다.

### def — 레코드 정의

```tablegen
def Gawee_Dialect : Dialect {
  let name = "gawee";
  let cppNamespace = "::mlir::gawee";
}
```

`def`는 하나의 **레코드(record)**를 정의한다.
`Gawee_Dialect`라는 이름으로 `Dialect` 타입의 레코드를 만든다.

### class — 재사용 가능한 템플릿

```tablegen
class Gawee_Op<string mnemonic, list<Trait> traits = []> :
    Op<Gawee_Dialect, mnemonic, traits>;
```

`class`는 C++의 클래스와 비슷하다.
`Gawee_Op`을 상속하는 모든 Op은 자동으로 Gawee 방언에 속하게 된다.

매개변수:
- `mnemonic`: Op 이름 (예: `"conv"`, `"relu"`)
- `traits`: Op의 특성 (예: `NoMemoryEffect` — 부작용 없음)

### let — 필드 값 설정

```tablegen
def Gawee_ConvOp : Gawee_Op<"conv", []> {
  let summary = "2D convolution";
  let arguments = (ins ...);
  let results = (outs ...);
}
```

`let`은 레코드의 필드 값을 설정한다.

---

## 6.5 Op 정의 — 완전 해부

Gawee의 ConvOp을 한 줄씩 분석한다:

```tablegen
def Gawee_ConvOp : Gawee_Op<"conv", []> {
//  ^^^^^^^^^^^^    ^^^^^^^^^  ^^^^  ^^
//  레코드 이름      부모 클래스  니모닉  traits (비어있음)
```

**니모닉(mnemonic)**: IR에서 Op 이름. `gawee.conv`에서 `conv` 부분.

### arguments — 입력 정의

```tablegen
  let arguments = (ins
    AnyTensor:$input,               // Operand: 아무 텐서 타입
    AnyTensor:$weight,              // Operand: 아무 텐서 타입
    AnyTensor:$bias,                // Operand: 아무 텐서 타입
    DenseI64ArrayAttr:$strides,     // Attribute: i64 배열 (컴파일 타임 상수)
    DenseI64ArrayAttr:$padding,     // Attribute: i64 배열
    DenseI64ArrayAttr:$dilation     // Attribute: i64 배열
  );
```

`ins`는 "입력(inputs)"을 의미한다.

**Operand vs Attribute 구분**:
- `AnyTensor:$name` → **Operand** (SSA Value, 런타임)
- `DenseI64ArrayAttr:$name` → **Attribute** (상수, 컴파일 타임)
- `BoolAttr:$name` → **Attribute**
- `F64Attr:$name` → **Attribute**
- `I64Attr:$name` → **Attribute**

`$` 뒤의 이름이 C++ 접근자 이름이 된다:
- `$input` → `getInput()`, `adaptor.getInput()`
- `$strides` → `getStrides()` (ArrayRef), `getStridesAttr()` (Attribute)

### results — 출력 정의

```tablegen
  let results = (outs AnyTensor:$output);
```

`outs`는 "출력(outputs)"을 의미한다.
`$output` → `getOutput()`, `getOutput().getType()`

---

## 6.6 TableGen이 자동 생성하는 것들

`Gawee_ConvOp` 정의 하나로 다음이 자동 생성된다:

### 1. C++ 클래스

```cpp
// generated/GaweeOps.h.inc (자동 생성)
namespace mlir::gawee {
class ConvOp : public Op<ConvOp, ...> {
public:
  static StringRef getOperationName() { return "gawee.conv"; }
  // ...
};
}
```

### 2. 접근자 메서드

```cpp
// Operand 접근
Value getInput();
Value getWeight();
Value getBias();

// Attribute 접근 (두 가지 형태)
ArrayRef<int64_t> getStrides();           // 값으로 반환
DenseI64ArrayAttr getStridesAttr();       // Attribute 객체로 반환

// 결과 접근
Value getOutput();
```

### 3. 파서와 프린터

MLIR IR을 텍스트로 읽고 쓰는 코드가 자동 생성된다:

```mlir
// 이런 텍스트를 자동으로 파싱/출력할 수 있다
%0 = gawee.conv %input, %weight, %bias {strides = [2, 2], ...}
    : tensor<...>, tensor<...>, tensor<...> -> tensor<...>
```

### 4. Verifier

Op이 올바른지 검증하는 코드:
- operand 개수 확인 (3개여야 함)
- attribute 타입 확인 (`strides`가 DenseI64Array인지)
- result 개수 확인 (1개여야 함)

---

## 6.7 다양한 Op 예시 비교

### 단순한 Op (ReLU — operand 1개)

```tablegen
def Gawee_ReluOp : Gawee_Op<"relu", []> {
  let summary = "ReLU activation";
  let arguments = (ins AnyTensor:$input);
  let results = (outs AnyTensor:$output);
}
```

### 복잡한 Op (BatchNorm — 여러 타입의 attribute)

```tablegen
def Gawee_BatchNormOp : Gawee_Op<"batch_norm", []> {
  let arguments = (ins
    AnyTensor:$input,
    AnyTensor:$weight,          // Operand (텐서)
    AnyTensor:$bias,            // Operand (텐서)
    BoolAttr:$affine,           // Attribute (bool)
    AnyTensor:$runningMean,     // Operand (텐서)
    AnyTensor:$runningVar,      // Operand (텐서)
    F64Attr:$eps                // Attribute (float64)
  );
  let results = (outs AnyTensor:$output);
}
```

### Op 정의 시 타입 선택 가이드

| 데이터 | TableGen 타입 | C++ 접근자 반환 |
|--------|---------------|----------------|
| 텐서 입력 | `AnyTensor:$name` | `Value` |
| 정수 배열 상수 | `DenseI64ArrayAttr:$name` | `ArrayRef<int64_t>` |
| 불리언 상수 | `BoolAttr:$name` | `BoolAttr` |
| 정수 상수 | `I64Attr:$name` | `IntegerAttr` |
| 실수 상수 | `F64Attr:$name` | `FloatAttr` |

---

## 6.8 OpBase.td — 모든 것의 뿌리

우리가 쓰는 `Dialect`, `Op`, `AnyTensor`, `DenseI64ArrayAttr` 등은 모두
MLIR이 제공하는 `OpBase.td`에 정의되어 있다.

```tablegen
include "mlir/IR/OpBase.td"   // 이 한 줄이 모든 기본 정의를 가져온다
```

주요 정의들:

```
Dialect       — 방언의 기본 클래스
Op            — 연산의 기본 클래스
Trait         — Op의 특성 (NoMemoryEffect 등)
AnyTensor     — 아무 텐서 타입
RankedTensorOf — 특정 원소 타입의 랭크드 텐서
I64Attr       — 64비트 정수 속성
F64Attr       — 64비트 실수 속성
BoolAttr      — 불리언 속성
DenseI64ArrayAttr — 64비트 정수 배열 속성
ins / outs    — arguments / results 정의용 키워드
```

---

## 6.9 Trait — Op의 특성

현재 Gawee Op들은 `traits = []`로 비어있지만,
실제 MLIR 프로젝트에서는 Trait을 활용한다:

```tablegen
// 예시 (현재 Gawee에서는 미사용)
def Gawee_ReluOp : Gawee_Op<"relu", [NoMemoryEffect]> {
//                                    ^^^^^^^^^^^^^^^
//   "이 Op은 메모리에 부작용(side effect)이 없다"
//   → 최적화 시 불필요한 Op을 안전하게 제거 가능
```

자주 쓰이는 Trait:

| Trait | 의미 |
|-------|------|
| `NoMemoryEffect` | 메모리 부작용 없음 (DCE 최적화 가능) |
| `SameOperandsAndResultType` | 입출력 타입이 동일 |
| `Commutative` | 교환법칙 성립 (a+b = b+a) |

---

## 6.10 새로운 Op 추가 체크리스트

새로운 Op (예: `gawee.cat`)을 추가할 때의 절차:

```
1. GaweeOps.td에 Op 정의 추가
   def Gawee_CatOp : Gawee_Op<"cat", []> {
     let arguments = (ins ...);
     let results = (outs ...);
   }

2. build.sh 실행 (TableGen → .inc 재생성)
   → GaweeOps.h.inc, GaweeOps.cpp.inc 갱신

3. MLIREmitter.cpp에 emit 함수 추가
   emitCat(node, values)

4. GaweeToLinalg.cpp에 Lowering 패턴 추가
   struct CatOpLowering : public OpConversionPattern<gawee::CatOp> { ... }
   patterns.add<CatOpLowering>(ctx);

5. 빌드 및 테스트
```

---

## 암기 정리표

```
┌─────────────────────────────────────────────────────┐
│ TableGen 전체 흐름                                    │
│   .td 파일 작성 → mlir-tblgen 실행 → .inc 생성        │
│   → .h/.cpp에서 #include → 사용                      │
├─────────────────────────────────────────────────────┤
│ 4가지 생성 모드                                       │
│   --gen-dialect-decls  → 방언 선언 (.h.inc)           │
│   --gen-dialect-defs   → 방언 정의 (.cpp.inc)         │
│   --gen-op-decls       → Op 선언 (.h.inc)             │
│   --gen-op-defs        → Op 정의 (.cpp.inc)           │
├─────────────────────────────────────────────────────┤
│ Op 정의 구조                                          │
│   def Name : Gawee_Op<"mnemonic", [traits]> {        │
│     let arguments = (ins                              │
│       AnyTensor:$input,        // Operand             │
│       DenseI64ArrayAttr:$attr  // Attribute           │
│     );                                                │
│     let results = (outs AnyTensor:$output);           │
│   }                                                   │
├─────────────────────────────────────────────────────┤
│ Operand vs Attribute                                 │
│   AnyTensor → Operand (런타임 SSA Value)              │
│   *Attr     → Attribute (컴파일 타임 상수)             │
├─────────────────────────────────────────────────────┤
│ $name → C++ 접근자                                    │
│   $input    → getInput()                              │
│   $strides  → getStrides() / getStridesAttr()        │
│   $output   → getOutput()                             │
├─────────────────────────────────────────────────────┤
│ .h에서 include 방법                                    │
│   #include "generated/GaweeDialect.h.inc"             │
│   #define GET_OP_CLASSES                               │
│   #include "generated/GaweeOps.h.inc"                 │
├─────────────────────────────────────────────────────┤
│ 자동 생성되는 것들                                     │
│   C++ 클래스 / 접근자 / 파서 / 프린터 / Verifier       │
├─────────────────────────────────────────────────────┤
│ 새 Op 추가 순서                                       │
│   1. GaweeOps.td 정의                                 │
│   2. build.sh (tblgen 재실행)                         │
│   3. MLIREmitter.cpp (emit 함수)                      │
│   4. GaweeToLinalg.cpp (lowering 패턴)               │
│   5. 빌드 + 테스트                                    │
└─────────────────────────────────────────────────────┘
```

---

# 7장. CMake 빌드 시스템 — MLIR 프로젝트 빌드하기

Gawee 프로젝트의 `middle/mlir/CMakeLists.txt`를 기준으로 설명한다.
새 라이브러리/실행파일/의존성을 추가할 때 이 문서를 참고한다.

---

## 7.1 전체 구조 — 한눈에 보기

```
CMakeLists.txt
├── 프로젝트 설정 (cmake_minimum_required, project, C++ 표준)
├── RTTI 설정 (LLVM과 일치시키기)
├── MLIR 찾기 (find_package → include)
├── Include 경로 설정
├── 라이브러리 정의
│   ├── GaweeDialect      (방언 + Op)
│   ├── GaweeConversion    (Lowering 패턴)
│   └── GaweeEmit          (JSON → MLIR)
└── 실행파일 정의
    ├── gawee-opt           (MLIR 최적화 도구)
    └── gawee-translate     (JSON → MLIR 변환 도구)
```

---

## 7.2 줄별 해설

### 프로젝트 기본 설정

```cmake
cmake_minimum_required(VERSION 3.20)       # CMake 최소 버전
project(GaweeMLIR LANGUAGES C CXX)         # 프로젝트 이름, C/C++ 사용

set(CMAKE_CXX_STANDARD 17)                 # C++17 사용 (structured bindings 등)
set(CMAKE_CXX_STANDARD_REQUIRED ON)        # C++17 없으면 에러
add_compile_options(-Wno-reserved-user-defined-literal)  # LLVM 헤더 경고 억제
```

### RTTI 설정 — 반드시 LLVM과 일치

```cmake
if(NOT LLVM_ENABLE_RTTI)
  add_compile_options(-fno-rtti)            # LLVM이 RTTI 없이 빌드됨 → 우리도 끔
endif()
```

**왜?** LLVM은 `dynamic_cast` 대신 `isa/cast/dyn_cast`를 쓴다.
RTTI 설정이 불일치하면 링크 에러 발생:
```
undefined reference to `typeinfo for mlir::Pass`
```

### MLIR 찾기

```cmake
find_package(MLIR REQUIRED CONFIG)
# MLIR_DIR에서 MLIRConfig.cmake를 찾는다
# 이 파일이 MLIR_INCLUDE_DIRS, MLIR_CMAKE_DIR 등의 변수를 설정

list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
# CMake 모듈 경로에 MLIR/LLVM의 .cmake 파일 디렉토리 추가

include(TableGen)     # mlir_tablegen() 매크로 사용 가능
include(AddLLVM)      # add_llvm_executable() 등 사용 가능
include(AddMLIR)      # add_mlir_dialect_library() 등 사용 가능
```

### Include 경로

```cmake
include_directories(${LLVM_INCLUDE_DIRS})    # LLVM 헤더 (llvm/ADT/SmallVector.h 등)
include_directories(${MLIR_INCLUDE_DIRS})    # MLIR 헤더 (mlir/IR/Builders.h 등)
include_directories(${CMAKE_SOURCE_DIR}/include)                # 우리 헤더 루트
include_directories(${CMAKE_SOURCE_DIR}/include/Gawee)          # GaweeDialect.h
include_directories(${CMAKE_SOURCE_DIR}/include/Gawee/generated) # TableGen 생성 .inc
```

---

## 7.3 타겟 정의 패턴

### 라이브러리 (add_library)

```cmake
# 패턴: add_library(이름  소스파일들)
add_library(GaweeDialect
  lib/Gawee/GaweeDialect.cpp
)

target_link_libraries(GaweeDialect
  PUBLIC              # 이 라이브러리를 쓰는 쪽도 이 의존성을 볼 수 있음
  MLIRIR              # mlir::Operation, mlir::Value 등
  MLIRSupport         # llvm::StringRef, llvm::SmallVector 등
)
```

**PUBLIC vs PRIVATE**:
- `PUBLIC`: GaweeDialect의 헤더에서 MLIRIR의 타입을 노출 → 사용자도 필요
- `PRIVATE`: 내부 구현에서만 사용 → 사용자에게 전파 안 됨

### 실행파일 (add_executable)

```cmake
add_executable(gawee-opt
  tools/gawee-opt.cpp
)

target_link_libraries(gawee-opt
  PRIVATE             # 실행파일은 대부분 PRIVATE
  GaweeDialect        # 우리 방언
  GaweeConversion     # 우리 Lowering 패턴
  MLIROptLib          # mlir::MlirOptMain 함수
  MLIRParser          # MLIR 파서
  MLIRPass            # Pass 인프라
  # ... (필요한 모든 방언과 변환)
)
```

### 의존성 체인

```
gawee-opt
├── GaweeConversion
│   ├── GaweeDialect
│   │   ├── MLIRIR
│   │   └── MLIRSupport
│   ├── MLIRLinalgDialect
│   ├── MLIRTensorDialect
│   ├── MLIRArithDialect
│   └── MLIRTransforms
├── MLIROptLib
├── MLIRParser
└── ... (모든 lowering 대상 방언)
```

`GaweeConversion`이 `GaweeDialect`를 `PUBLIC`으로 링크하므로,
`gawee-opt`는 `GaweeDialect`를 명시적으로 추가하지 않아도 접근 가능.
(하지만 명시성을 위해 보통 적어둔다.)

---

## 7.4 실전 레시피 — 새 항목 추가할 때

### 새 라이브러리 추가

```cmake
# 예: GaweeOptimize라는 새 최적화 라이브러리
add_library(GaweeOptimize
  lib/Optimize/FuseConvBN.cpp
  lib/Optimize/FoldConstants.cpp
)

target_link_libraries(GaweeOptimize
  PUBLIC
  GaweeDialect          # Gawee Op 사용
  MLIRTransforms        # 변환 유틸리티
)
```

### 새 실행파일 추가

```cmake
add_executable(gawee-run
  tools/gawee-run.cpp
)

target_link_libraries(gawee-run
  PRIVATE
  GaweeDialect
  GaweeConversion
  GaweeOptimize         # 새로 만든 라이브러리
  MLIRExecutionEngine   # JIT 실행 시
  ${llvm_libs}
)
```

### 새 방언 의존성 추가

gawee-opt에서 새 방언(예: SCF)을 사용하려면:
```cmake
target_link_libraries(gawee-opt
  PRIVATE
  # 기존 항목들...
  MLIRSCFDialect        # ← 추가
  MLIRSCFToControlFlow  # ← 변환 패스도 추가
)
```

그리고 C++ 코드에서도 등록:
```cpp
registry.insert<scf::SCFDialect>();  // DialectRegistry에 등록
```

### 새 소스 파일 추가

기존 라이브러리에 파일을 추가:
```cmake
add_library(GaweeConversion
  lib/Conversion/GaweeToLinalg.cpp
  lib/Conversion/GaweeToTosa.cpp     # ← 새 파일 추가
)
```

---

## 7.5 에러 → 원인 → 해결

| 에러 메시지 | 원인 | 해결 |
|---|---|---|
| `undefined reference to typeinfo for mlir::Pass` | RTTI 불일치 | `add_compile_options(-fno-rtti)` 확인 |
| `undefined reference to mlir::linalg::GenericOp::create` | 라이브러리 미링크 | `target_link_libraries`에 `MLIRLinalgDialect` 추가 |
| `fatal error: 'Gawee/GaweeDialect.h' file not found` | include 경로 누락 | `include_directories`에 경로 추가 |
| `Could not find a package configuration file provided by "MLIR"` | MLIR_DIR 미설정 | `cmake -DMLIR_DIR=/path/to/mlir/lib/cmake/mlir ..` |
| `error: use of undeclared identifier 'GaweeDialect'` | 헤더 미포함 또는 방언 미등록 | `#include` 확인 + `registry.insert<>()` 확인 |
| `multiple definition of ...` | 같은 .cpp를 여러 타겟에 포함 | 라이브러리로 분리 후 링크 |
| `undefined reference to mlir::gawee::createGaweeToLinalgPass()` | GaweeConversion 미링크 | `target_link_libraries`에 `GaweeConversion` 추가 |

---

## 7.6 LLVM 라이브러리 찾기

```cmake
llvm_map_components_to_libnames(llvm_libs Support Core)
# LLVM의 Support, Core 컴포넌트에 해당하는 실제 라이브러리 이름을 llvm_libs에 저장
# 플랫폼마다 라이브러리 이름이 다를 수 있으므로 이 함수가 해결

target_link_libraries(gawee-opt
  PRIVATE
  ${llvm_libs}    # LLVMSupport, LLVMCore 등으로 확장됨
)
```

---

## 암기표

```
┌──────────────────────────────────────────────────────┐
│ CMake 빌드 체크리스트                                  │
│                                                      │
│ 1. find_package(MLIR) → MLIR 변수들 설정              │
│ 2. include(TableGen/AddLLVM/AddMLIR)                 │
│ 3. include_directories → 헤더 찾기 경로               │
│ 4. add_library → .cpp → .a (정적 라이브러리)          │
│ 5. add_executable → .cpp → 실행파일                  │
│ 6. target_link_libraries → 의존성 연결               │
├──────────────────────────────────────────────────────┤
│ PUBLIC vs PRIVATE                                    │
│   PUBLIC  = 헤더에서 노출 → 사용자도 필요             │
│   PRIVATE = 내부만 사용 → 사용자에게 전파 안 됨       │
├──────────────────────────────────────────────────────┤
│ 새 항목 추가 시                                       │
│   라이브러리: add_library + target_link_libraries     │
│   실행파일: add_executable + target_link_libraries    │
│   새 방언: target_link_libraries에 추가 +             │
│           registry.insert<>()                        │
│   새 파일: add_library의 소스 목록에 추가             │
├──────────────────────────────────────────────────────┤
│ 빈출 에러                                             │
│   typeinfo → RTTI 불일치 (-fno-rtti)                  │
│   undefined ref → 라이브러리 미링크                    │
│   file not found → include 경로 누락                  │
│   package not found → MLIR_DIR 미설정                 │
└──────────────────────────────────────────────────────┘
```

---

