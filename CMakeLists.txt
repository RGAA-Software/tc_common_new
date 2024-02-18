cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 20)

find_package(leveldb REQUIRED)
#find_package(spdlog CONFIG REQUIRED)
#find_package(fmt CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/tc_3rdparty/)
include_directories(${CMAKE_SOURCE_DIR}/tc_3rdparty/eventbus/src)

message("==--== " ${CMAKE_SOURCE_DIR}/tc_3rdparty/eventbus/src)

# C:\source\vcpkg\installed\x64-windows-static\lib
# C:\source\vcpkg\packages\jemalloc_x64-windows-static\lib\jemalloc_s.lib
#link_directories(${VCPKG_ROOT}/packages/jemalloc_x64-windows-static/lib)
link_directories(${VCPKG_ROOT}/installed/x64-windows-static/lib)
message(" static lib folder =====> ${VCPKG_ROOT}/installed/x64-windows-static/lib" )
message(" static lib folder =====> ${VCPKG_ROOT}/packages/jemalloc_x64-windows-static/lib" )

add_definitions("-DUNICODE" "-D_UNICODE")
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

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
    timer.cpp
    file_ext.cpp
    process_util.cpp
    dump_helper.cpp
)

if(WIN32)
    list(APPEND SRCS
        win32/d3d_debug_helper.cpp
        win32/d3d_render.cc
    )
endif()

add_library(tc_common ${SRCS})
#fmt::fmt-header-only
#spdlog::spdlog_header_only
target_link_libraries(tc_common Dexode::EventBus leveldb::leveldb ${Boost_LIBRARIES})
if (WIN32)
    target_link_libraries(tc_common jemalloc DirectXTex DXGI D3D11 OpenSSL::SSL OpenSSL::Crypto)
#    target_link_libraries(tc_common ${VCPKG_ROOT}/packages/jemalloc_x64-windows-static/lib/jemalloc_s.lib)
endif ()

target_include_directories(tc_common PUBLIC ${CMAKE_SOURCE_DIR}/tc_3rdparty/eventbus/src)

add_subdirectory(tests)
