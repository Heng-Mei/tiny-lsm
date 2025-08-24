cmake_minimum_required(VERSION 3.25)
project(toni_lsm VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configure Debug/Release Macros
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DLSM_DEBUG)
endif()

# Set the include directory for spdlog (header-only)
set(SPDLOG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/spdlog-1.15.3/include")
include_directories(${SPDLOG_INCLUDE_DIR})

# Set the include directory for toml11 (header-only)
set(TOML11_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/toml11-4.4.0/include")
include_directories(${TOML11_INCLUDE_DIR})

# External dependencies
find_package(Threads REQUIRED)
# find_package(ASIO REQUIRED)

add_subdirectory(3rdparty/pybind11)
add_subdirectory(3rdparty/googletest-1.17.0)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Static Libraries
file(GLOB LOGGER_SRC src/logger/*.cpp)
add_library(logger STATIC ${LOGGER_SRC})
target_include_directories(logger PUBLIC ${SPDLOG_INCLUDE_DIR} ${TOML11_INCLUDE_DIR} include)

file(GLOB CONFIG_SRC src/config/*.cpp)
add_library(config STATIC ${CONFIG_SRC})
target_include_directories(config PUBLIC ${SPDLOG_INCLUDE_DIR} ${TOML11_INCLUDE_DIR} include)

file(GLOB UTILS_SRC src/utils/*.cpp)
add_library(utils STATIC ${UTILS_SRC})
target_include_directories(utils PUBLIC ${SPDLOG_INCLUDE_DIR} ${TOML11_INCLUDE_DIR} include)

file(GLOB ITERATOR_SRC src/iterator/*.cpp)
add_library(iterator STATIC ${ITERATOR_SRC})
target_include_directories(iterator PUBLIC ${SPDLOG_INCLUDE_DIR} ${TOML11_INCLUDE_DIR} include)

file(GLOB SKIPLIST_SRC src/skiplist/*.cpp)
add_library(skiplist STATIC ${SKIPLIST_SRC})
target_include_directories(skiplist PUBLIC ${SPDLOG_INCLUDE_DIR} ${TOML11_INCLUDE_DIR} include)

file(GLOB BLOCK_SRC src/block/*.cpp)
add_library(block STATIC ${BLOCK_SRC})
target_include_directories(block PUBLIC ${SPDLOG_INCLUDE_DIR} ${TOML11_INCLUDE_DIR} include)

file(GLOB SST_SRC src/sst/*.cpp)
add_library(sst STATIC ${SST_SRC})
target_include_directories(sst PUBLIC ${SPDLOG_INCLUDE_DIR} ${TOML11_INCLUDE_DIR} include)

file(GLOB MEMTABLE_SRC src/memtable/*.cpp)
add_library(memtable STATIC ${MEMTABLE_SRC})
target_include_directories(memtable PUBLIC ${SPDLOG_INCLUDE_DIR} ${TOML11_INCLUDE_DIR} include)

file(GLOB WAL_SRC src/wal/*.cpp)
add_library(wal STATIC ${WAL_SRC})
target_include_directories(wal PUBLIC ${SPDLOG_INCLUDE_DIR} ${TOML11_INCLUDE_DIR} include)

file(GLOB LSM_SRC src/lsm/*.cpp)
add_library(lsm STATIC ${LSM_SRC})
target_include_directories(lsm PUBLIC ${SPDLOG_INCLUDE_DIR} ${TOML11_INCLUDE_DIR} include)

file(GLOB REDIS_SRC src/redis_wrapper/*.cpp)
add_library(redis STATIC ${REDIS_SRC})
target_include_directories(redis PUBLIC ${SPDLOG_INCLUDE_DIR} ${TOML11_INCLUDE_DIR} include)

# Dynamic Libraries
file(GLOB_RECURSE LSM_SHARED_SRC "src/*.cpp")
add_library(lsm_shared SHARED ${LSM_SHARED_SRC})
target_include_directories(lsm_shared PUBLIC ${SPDLOG_INCLUDE_DIR} ${TOML11_INCLUDE_DIR} include)

# Python Bindings
pybind11_add_module(lsm_pybind sdk/lsm_pybind.cpp)
target_include_directories(lsm_pybind PUBLIC ${SPDLOG_INCLUDE_DIR} ${TOML11_INCLUDE_DIR} include)

# Example Programs
add_executable(example example/main.cpp)
target_link_libraries(example PRIVATE lsm_shared)
target_include_directories(example PUBLIC ${SPDLOG_INCLUDE_DIR} ${TOML11_INCLUDE_DIR} include)
set_target_properties(example PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(debug example/debug.cpp)
target_link_libraries(debug PRIVATE lsm_shared)
target_include_directories(debug PUBLIC ${SPDLOG_INCLUDE_DIR} ${TOML11_INCLUDE_DIR} include)
set_target_properties(debug PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Server
file(GLOB SERVER_SRC server/src/*.cpp)
add_executable(server ${SERVER_SRC})
target_include_directories(server PUBLIC ${SPDLOG_INCLUDE_DIR} ${TOML11_INCLUDE_DIR} include)
target_link_libraries(server PRIVATE redis lsm_shared)
set_target_properties(server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Tests
macro(add_test_target name src deps)
    add_executable(${name} ${src})
    target_include_directories(${name} PUBLIC ${SPDLOG_INCLUDE_DIR} ${TOML11_INCLUDE_DIR} include)
    target_link_libraries(${name} PRIVATE lsm_shared gtest gtest_main gmock gmock_main ${deps})
    add_test(NAME ${name} COMMAND ${name})
endmacro()

add_test_target(test_config test/test_config.cpp "logger;config")
add_test_target(test_skiplist test/test_skiplist.cpp "logger;skiplist")
add_test_target(test_memtable test/test_memtable.cpp "logger;memtable")
add_test_target(test_block test/test_block.cpp "logger;block")
add_test_target(test_blockmeta test/test_blockmeta.cpp "logger;block")
add_test_target(test_utils test/test_utils.cpp "logger;utils")
add_test_target(test_sst test/test_sst.cpp "logger;sst")
add_test_target(test_lsm test/test_lsm.cpp "logger;lsm;memtable;iterator")
add_test_target(test_block_cache test/test_block_cache.cpp "logger;block")
add_test_target(test_compact test/test_compact.cpp "logger;lsm;memtable;iterator")
add_test_target(test_redis test/test_redis.cpp "logger;redis;memtable;iterator")
add_test_target(test_wal test/test_wal.cpp "logger;wal")

enable_testing()
