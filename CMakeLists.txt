cmake_minimum_required(VERSION 3.16)
project(ICPC-Management-System-2025 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 将可执行文件输出到 build 根目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(icpc_manager
    src/main.cpp
)
target_include_directories(icpc_manager PRIVATE include)

# 自定义测试目标：编译后运行对拍
add_custom_target(run_tests
    COMMAND ${CMAKE_COMMAND} -E env bash ${CMAKE_SOURCE_DIR}/scripts/run_tests.sh
            ${CMAKE_BINARY_DIR}/icpc_manager
            ${CMAKE_SOURCE_DIR}/data
    DEPENDS icpc_manager
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running data/*.in vs *.out diff tests"
)
