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
