cmake_minimum_required(VERSION 3.22)
project(tc_common)

option(OPTION_ENABLE_JEMALLOC "Enable feature for LibraryA" OFF)
message("common cmake, OPTION_ENABLE_JEMALLOC: " ${OPTION_ENABLE_JEMALLOC})
set(CMAKE_CXX_STANDARD 20)

find_package(leveldb REQUIRED)
#find_package(spdlog CONFIG REQUIRED)
#find_package(fmt CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/tc_3rdparty/)
include_directories(${CMAKE_SOURCE_DIR}/tc_3rdparty/eventbus/src)
set(VCPKG_STATIC_LIB_PATH ${VCPKG_ROOT}/installed/x64-windows-static/lib)
link_directories(VCPKG_STATIC_LIB_PATH)
message("==--== " ${CMAKE_SOURCE_DIR}/tc_3rdparty/eventbus/src)
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

    ##
#    concurrent_hashmap.h
#    concurrent_vector.h
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
#leveldb::leveldb
target_link_libraries(tc_common Dexode::EventBus ${VCPKG_STATIC_LIB_PATH}/leveldb.lib ${Boost_LIBRARIES})
if (WIN32)
    if(OPTION_ENABLE_JEMALLOC)
        target_compile_definitions(tc_common PRIVATE ENABLE_JEMALLOC)
        target_link_libraries(tc_common jemalloc)
    endif()
    target_link_libraries(tc_common DirectXTex DXGI D3D11 OpenSSL::SSL OpenSSL::Crypto)
endif ()
if (ANDROID)
    target_link_libraries(tc_common log)
endif()

target_include_directories(tc_common PUBLIC ${CMAKE_SOURCE_DIR}/tc_3rdparty/eventbus/src)

add_subdirectory(tests)
