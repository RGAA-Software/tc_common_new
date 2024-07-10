cmake_minimum_required(VERSION 3.22)
#message("=-=-=-=--= ${Boost_LIBRARIES}")
find_package(GTest CONFIG REQUIRED)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Core5Compat)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Core5Compat)
add_executable(test_common test.cpp)
target_link_libraries(test_common PRIVATE tc_common_new ${Boost_LIBRARIES} GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main Qt6::Core Qt6::Core5Compat)
add_custom_command(TARGET test_common POST_BUILD
        COMMAND ${QT_ROOT}/bin/windeployqt.exe $<TARGET_FILE:test_common> --no-translations --release
)