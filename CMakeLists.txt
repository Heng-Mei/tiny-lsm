cmake_minimum_required(VERSION 3.25)
project(toni_lsm VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 配置 Debug/Release 宏
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DLSM_DEBUG)
endif()

# ---------------------
# 外部依赖
# ---------------------
find_package(spdlog REQUIRED)
find_package(toml11 REQUIRED)
find_package(Threads REQUIRED)
find_package(pybind11 REQUIRED)
find_package(GTest REQUIRED)
# find_package(ASIO REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ---------------------
# 静态库
# ---------------------
file(GLOB LOGGER_SRC src/logger/*.cpp)
add_library(logger STATIC ${LOGGER_SRC})
target_link_libraries(logger PRIVATE spdlog::spdlog)
target_include_directories(logger PUBLIC include)

file(GLOB CONFIG_SRC src/config/*.cpp)
add_library(config STATIC ${CONFIG_SRC})
target_link_libraries(config PRIVATE spdlog::spdlog toml11::toml11)
target_include_directories(config PUBLIC include)

file(GLOB UTILS_SRC src/utils/*.cpp)
add_library(utils STATIC ${UTILS_SRC})
target_link_libraries(utils PRIVATE spdlog::spdlog toml11::toml11)
target_include_directories(utils PUBLIC include)

file(GLOB ITERATOR_SRC src/iterator/*.cpp)
add_library(iterator STATIC ${ITERATOR_SRC})
target_link_libraries(iterator PRIVATE spdlog::spdlog toml11::toml11)
target_include_directories(iterator PUBLIC include)

file(GLOB SKIPLIST_SRC src/skiplist/*.cpp)
add_library(skiplist STATIC ${SKIPLIST_SRC})
target_link_libraries(skiplist PRIVATE spdlog::spdlog toml11::toml11)
target_include_directories(skiplist PUBLIC include)

file(GLOB BLOCK_SRC src/block/*.cpp)
add_library(block STATIC ${BLOCK_SRC})
target_link_libraries(block PRIVATE spdlog::spdlog toml11::toml11 config)
target_include_directories(block PUBLIC include)

file(GLOB SST_SRC src/sst/*.cpp)
add_library(sst STATIC ${SST_SRC})
target_link_libraries(sst PRIVATE spdlog::spdlog toml11::toml11 block utils iterator)
target_include_directories(sst PUBLIC include)

file(GLOB MEMTABLE_SRC src/memtable/*.cpp)
add_library(memtable STATIC ${MEMTABLE_SRC})
target_link_libraries(memtable PRIVATE spdlog::spdlog toml11::toml11 skiplist iterator config sst)
target_include_directories(memtable PUBLIC include)

file(GLOB WAL_SRC src/wal/*.cpp)
add_library(wal STATIC ${WAL_SRC})
target_link_libraries(wal PRIVATE spdlog::spdlog toml11::toml11 sst memtable)
target_include_directories(wal PUBLIC include)

file(GLOB LSM_SRC src/lsm/*.cpp)
add_library(lsm STATIC ${LSM_SRC})
target_link_libraries(lsm PRIVATE spdlog::spdlog toml11::toml11 sst memtable wal logger)
target_include_directories(lsm PUBLIC include)

file(GLOB REDIS_SRC src/redis_wrapper/*.cpp)
add_library(redis STATIC ${REDIS_SRC})
target_link_libraries(redis PRIVATE spdlog::spdlog toml11::toml11 lsm)
target_include_directories(redis PUBLIC include)

# ---------------------
# 动态库
# ---------------------
file(GLOB_RECURSE LSM_SHARED_SRC "src/*.cpp")
add_library(lsm_shared SHARED ${LSM_SHARED_SRC})
target_link_libraries(lsm_shared PRIVATE spdlog::spdlog toml11::toml11
    logger config utils iterator skiplist block sst memtable wal lsm redis)
target_include_directories(lsm_shared PUBLIC include)
set_target_properties(lsm_shared PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)


# ---------------------
# Python 绑定
# ---------------------
pybind11_add_module(lsm_pybind sdk/lsm_pybind.cpp)
target_link_libraries(lsm_pybind PRIVATE lsm_shared)
target_include_directories(lsm_pybind PUBLIC include)
set_target_properties(lsm_pybind PROPERTIES
    OUTPUT_NAME "lsm_pybind"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)
target_compile_definitions(lsm_pybind PRIVATE TONILSM_EXPORT="__attribute__((visibility(\"default\")))")
target_compile_options(lsm_pybind PRIVATE -fvisibility=hidden)
target_link_options(lsm_pybind PRIVATE "-Wl,-rpath,$ORIGIN")

# ---------------------
# 示例程序
# ---------------------
add_executable(example example/main.cpp)
target_link_libraries(example PRIVATE lsm_shared)
target_include_directories(example PUBLIC include)
set_target_properties(example PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(debug example/debug.cpp)
target_link_libraries(debug PRIVATE lsm_shared)
target_include_directories(debug PUBLIC include)
set_target_properties(debug PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------------------
# Server
# ---------------------
file(GLOB SERVER_SRC server/src/*.cpp)
add_executable(server ${SERVER_SRC})
target_link_libraries(server PRIVATE redis)
target_include_directories(server PUBLIC include)
set_target_properties(server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------------------
# 测试
# ---------------------
macro(add_test_target name src deps)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE ${deps} GTest::GTest GTest::Main GTest::gmock GTest::gmock_main spdlog::spdlog toml11::toml11)
    target_include_directories(${name} PUBLIC include)
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
