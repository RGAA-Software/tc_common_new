cmake_minimum_required(VERSION 3.26)

set(CMAKE_CXX_STANDARD 20)

find_package(leveldb REQUIRED)
find_package(spdlog CONFIG REQUIRED)

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
)

add_library(tc_common ${SRCS})
target_link_libraries(tc_common spdlog::spdlog_header_only leveldb::leveldb ${Boost_LIBRARIES})
if (WIN32)
    target_link_libraries(tc_common jemalloc)
endif ()

add_subdirectory(tests)
