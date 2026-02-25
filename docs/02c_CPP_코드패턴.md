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
