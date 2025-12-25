cmake_minimum_required(VERSION 3.10.0)
project(MemorySimulator VERSION 1.0.0 LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(MSVC)
    add_compile_options(/W4)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files (excluding main.cpp for library)
set(LIB_SOURCES
    src/MemoryBlock.cpp
    src/MemoryPool.cpp
    src/HistoryManager.cpp
    src/TUI.cpp
)

# Main executable
set(MAIN_SOURCES
    src/main.cpp
    ${LIB_SOURCES}
)

# Executable
add_executable(${PROJECT_NAME} ${MAIN_SOURCES})

# Output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ==================== 测试配置 ====================

# 测试可执行文件
set(TEST_SOURCES
    test/MemoryPoolTest.cpp
    ${LIB_SOURCES}
)

add_executable(MemoryPoolTest ${TEST_SOURCES})

set_target_properties(MemoryPoolTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 启用测试
enable_testing()
add_test(NAME MemoryPoolTest COMMAND MemoryPoolTest)
