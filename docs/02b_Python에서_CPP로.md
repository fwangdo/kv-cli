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
