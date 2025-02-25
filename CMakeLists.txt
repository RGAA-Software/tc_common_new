cmake_minimum_required(VERSION 3.22)
project(tc_common_new)

option(OPTION_ENABLE_JEMALLOC "Enable feature for LibraryA" OFF)
message("common cmake, OPTION_ENABLE_JEMALLOC: " ${OPTION_ENABLE_JEMALLOC})
set(CMAKE_CXX_STANDARD 20)

#find_package(OpenSSL REQUIRED)
include_directories(${CMAKE_SOURCE_DIR}/deps/tc_3rdparty/)
include_directories(${CMAKE_SOURCE_DIR}/deps/tc_3rdparty/openssl/x64/include)
include_directories(${CMAKE_SOURCE_DIR}/deps/tc_3rdparty/eventbus/src)

if (WIN32)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Core5Compat Widgets Network)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Core5Compat Widgets Network)
    find_package(FFTW3 CONFIG REQUIRED)
    #set(VCPKG_STATIC_LIB_PATH ${VCPKG_ROOT}/installed/x64-windows-static/lib)
    set(VCPKG_STATIC_LIB_PATH )
elseif (ANDROID)
    set(VCPKG_STATIC_LIB_PATH ${VCPKG_ROOT}/installed/arm64-android/lib)
endif ()
link_directories(VCPKG_STATIC_LIB_PATH)

message("==--== " ${CMAKE_SOURCE_DIR}/tc_3rdparty/eventbus/src)

add_definitions("-DUNICODE" "-D_UNICODE")
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
if (ANDROID)
    add_compile_options(-Wno-error=deprecated-declarations)
endif ()

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
        file_ext.cpp
        process_util.cpp
        dump_helper.cpp
        folder_util.cpp
        num_formatter.cpp
        monitors.cpp
        fft_32.cpp
        key_helper.cpp
        url_helper.cpp
        ws_server.cpp
        message_looper.cpp
)

if(WIN32)
    list(APPEND SRCS
        win32/d3d_debug_helper.cpp
        win32/d3d_render.cc
        win32/process_helper.cpp
        win32/win_helper.cpp
        win32/audio_device_helper.cpp
        win32/firewall_helper.cpp
        qrcode/qr_generator.cpp
        qrcode/qrcodegen.cpp
        qrcode/qrcodegen.hpp
        win32/dxgi_mon_detector.cpp
        hardware.cpp
        ip_util.cpp
    )
endif()

add_library(tc_common_new ${SRCS})
target_link_libraries(tc_common_new Dexode::EventBus)
if (WIN32)
    add_subdirectory(tests)
    if(OPTION_ENABLE_JEMALLOC)
        target_compile_definitions(tc_common_new PRIVATE ENABLE_JEMALLOC)
        target_link_libraries(tc_common_new jemalloc)
    endif()
    target_link_libraries(tc_common_new DirectXTex DXGI D3D11 libssl64MT.lib libcrypto64MT.lib Shlwapi.lib)#OpenSSL::SSL OpenSSL::Crypto
    target_link_libraries(tc_common_new Qt6::Core Qt6::Core5Compat Qt6::Widgets Qt6::Network FFTW3::fftw3)
endif ()
target_link_libraries(tc_common_new leveldb)

target_include_directories(tc_common_new PUBLIC ${CMAKE_SOURCE_DIR}/tc_3rdparty/eventbus/src)
