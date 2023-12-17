cmake_minimum_required(VERSION 3.26)

set(CMAKE_CXX_STANDARD 20)

find_package(leveldb REQUIRED)

add_library(tc_common library.cpp)
target_link_libraries(tc_common leveldb::leveldb ${Boost_LIBRARIES})
