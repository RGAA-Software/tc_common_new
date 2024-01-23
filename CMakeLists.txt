cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 20)

find_package(leveldb REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../tc_3rdparty/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../tc_3rdparty/eventbus/src)

set(SRCS
    data.cpp
    shared_preference.cpp
    string_ext.cpp
    time_ext.cpp
    thread.cpp
    task_runtime.cpp
    base64.cpp
    file.cpp
    image.cpp
    ipc_shm.cpp
    ipc_msg_queue.cpp
    message_notifier.cpp
    http_client.cpp
    http_client.cpp
)

if(WIN32)
    list(APPEND SRCS
        win32/d3d_debug_helper.cpp
        win32/d3d_render.cc
    )
endif()

add_library(tc_common ${SRCS})
target_link_libraries(tc_common spdlog::spdlog_header_only leveldb::leveldb ${Boost_LIBRARIES} fmt::fmt-header-only)
if (WIN32)
    target_link_libraries(tc_common jemalloc DirectXTex DXGI D3D11 OpenSSL::SSL OpenSSL::Crypto)
endif ()
add_subdirectory(tests)
