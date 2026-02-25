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
