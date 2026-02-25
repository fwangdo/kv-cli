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
