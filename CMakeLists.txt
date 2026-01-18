cmake_minimum_required(VERSION 3.16)
project(ICPC-Management-System-2025 LANGUAGES CXX)

# Default to Release for better runtime performance when not explicitly set.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 将可执行文件输出到仓库根目录并命名为 code
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(icpc_manager
    src/main.cpp
)
target_include_directories(icpc_manager PRIVATE include)
set_target_properties(icpc_manager PROPERTIES OUTPUT_NAME code)

# Aggressive optimization flags for GCC/Clang; MSVC keeps its defaults.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(icpc_manager PRIVATE -O3 -march=native -pipe -flto -fno-plt)
    target_link_options(icpc_manager PRIVATE -O3 -march=native -pipe -flto -fno-plt)
endif()

# Enable LTO in Release if the generator supports it.
set_property(TARGET icpc_manager PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

# 自定义测试目标：编译后运行对拍
add_custom_target(run_tests
    COMMAND ${CMAKE_COMMAND} -E env bash ${CMAKE_SOURCE_DIR}/scripts/run_tests.sh
            ${CMAKE_SOURCE_DIR}/code
            ${CMAKE_SOURCE_DIR}/data
    DEPENDS icpc_manager
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running data/*.in vs *.out diff tests"
)
