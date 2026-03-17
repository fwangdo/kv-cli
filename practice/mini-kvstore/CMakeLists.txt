cmake_minimum_required(VERSION 3.20)
project(kvstore VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(kv_cli 
    src/main.cpp
)

add_library(kv_core
    src/store.cpp
    src/exporter.cpp
    src/command.cpp
)

target_link_libraries(kv_cli PRIVATE kv_core)

target_include_directories(kv_core
    PUBLIC include
)