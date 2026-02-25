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
