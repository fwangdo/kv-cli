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
