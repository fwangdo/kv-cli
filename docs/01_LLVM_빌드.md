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
