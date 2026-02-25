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
