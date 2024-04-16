cmake_minimum_required(VERSION 3.22)
project(tc_common_new)

option(OPTION_ENABLE_JEMALLOC "Enable feature for LibraryA" OFF)
message("common cmake, OPTION_ENABLE_JEMALLOC: " ${OPTION_ENABLE_JEMALLOC})
set(CMAKE_CXX_STANDARD 20)

find_package(leveldb REQUIRED)
#find_package(spdlog CONFIG REQUIRED)
#find_package(fmt CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem thread random)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    MESSAGE(STATUS "Boost_INCLUDE_DIRS = ${Boost_INCLUDE_DIRS}.")
    MESSAGE(STATUS "Boost_LIBRARIES = ${Boost_LIBRARIES}.")
    MESSAGE(STATUS "Boost_LIB_VERSION = ${Boost_LIB_VERSION}.")
endif()

include_directories(${CMAKE_SOURCE_DIR}/tc_3rdparty/)
include_directories(${CMAKE_SOURCE_DIR}/tc_3rdparty/eventbus/src)
if (WIN32)
    set(VCPKG_STATIC_LIB_PATH ${VCPKG_ROOT}/installed/x64-windows-static/lib)
elseif (ANDROID)
    set(VCPKG_STATIC_LIB_PATH ${VCPKG_ROOT}/installed/arm64-android/lib)
endif ()
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
    folder_util.cpp
    ##
#    concurrent_hashmap.h
#    concurrent_vector.h
)

if(WIN32)
    list(APPEND SRCS
        win32/d3d_debug_helper.cpp
        win32/d3d_render.cc
        win32/process_helper.cpp
        win32/win_helper.cpp
    )
endif()

add_library(tc_common_new ${SRCS})
#fmt::fmt-header-only
#spdlog::spdlog_header_only
#leveldb::leveldb
target_link_libraries(tc_common_new Dexode::EventBus ${Boost_LIBRARIES})
if (WIN32)
    if(OPTION_ENABLE_JEMALLOC)
        target_compile_definitions(tc_common_new PRIVATE ENABLE_JEMALLOC)
        target_link_libraries(tc_common_new jemalloc)
    endif()
    target_link_libraries(tc_common_new DirectXTex DXGI D3D11 OpenSSL::SSL OpenSSL::Crypto)
    target_link_libraries(tc_common_new ${VCPKG_STATIC_LIB_PATH}/leveldb.lib)
endif ()
if (ANDROID)
    target_link_libraries(tc_common_new log ${VCPKG_STATIC_LIB_PATH}/libleveldb.a)
endif()

target_include_directories(tc_common_new PUBLIC ${CMAKE_SOURCE_DIR}/tc_3rdparty/eventbus/src)

add_subdirectory(tests)
