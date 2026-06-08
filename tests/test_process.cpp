//
// Created by RGAA on 2024/5/24.
//
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <map>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <windows.h>
#include "../process_util.h"
#include "../string_util.h"

using namespace tc;

TEST(TestProcess, StartProcessAndWait_Success) {
    auto ret = ProcessUtil::StartProcessAndWait("cmd.exe", {"/c", "exit", "/b", "0"});
    EXPECT_TRUE(ret);
}

TEST(TestProcess, StartProcessAndWait_Failure) {
    auto ret = ProcessUtil::StartProcessAndWait("cmd.exe", {"/c", "exit", "/b", "1"});
    EXPECT_FALSE(ret);
}

TEST(TestProcess, StartProcessAndWait_NotExists) {
    auto ret = ProcessUtil::StartProcessAndWait("this_exe_should_not_exist_12345.exe", {});
    EXPECT_FALSE(ret);
}

TEST(TestProcess, StartProcess_Detach) {
    auto pid = ProcessUtil::StartProcess("cmd.exe", {"/c", "timeout", "/t", "1"}, true, false);
    EXPECT_GT(pid, 0);
    if (pid > 0) {
        // Give it a moment to start, then kill it
        Sleep(100);
        auto killed = ProcessUtil::KillProcess(pid);
        EXPECT_TRUE(killed);
    }
}

TEST(TestProcess, StartProcess_Wait) {
    auto ret = ProcessUtil::StartProcess("cmd.exe", {"/c", "exit", "/b", "0"}, false, true);
    EXPECT_EQ(ret, 0);
}

TEST(TestProcess, StartProcess_NoDetachNoWait) {
    auto pid = ProcessUtil::StartProcess("cmd.exe", {"/c", "timeout", "/t", "1"}, false, false);
    EXPECT_GT(pid, 0);
    if (pid > 0) {
        Sleep(100);
        ProcessUtil::KillProcess(pid);
    }
}

TEST(TestProcess, StartProcessAndOutput_CaptureStdout) {
    auto output = ProcessUtil::StartProcessAndOutput("cmd.exe", {"/c", "echo", "hello_world_test"});
    EXPECT_FALSE(output.empty());
    bool found = false;
    for (const auto& line : output) {
        if (line.find("hello_world_test") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST(TestProcess, StartProcessAndOutput_MultipleLines) {
    auto output = ProcessUtil::StartProcessAndOutput("cmd.exe", {"/c", "echo", "line1", "&&", "echo", "line2"});
    EXPECT_GE(output.size(), 1);
    bool found_line1 = false;
    bool found_line2 = false;
    for (const auto& line : output) {
        if (line.find("line1") != std::string::npos) found_line1 = true;
        if (line.find("line2") != std::string::npos) found_line2 = true;
    }
    EXPECT_TRUE(found_line1);
    EXPECT_TRUE(found_line2);
}

TEST(TestProcess, KillProcess_ValidPid) {
    auto pid = ProcessUtil::StartProcess("cmd.exe", {"/c", "timeout", "/t", "10"}, true, false);
    ASSERT_GT(pid, 0);
    auto killed = ProcessUtil::KillProcess(pid);
    EXPECT_TRUE(killed);
}

TEST(TestProcess, KillProcess_InvalidPid) {
    auto killed = ProcessUtil::KillProcess(99999);
    EXPECT_FALSE(killed);
}

TEST(TestProcess, GetPidByExeName_Explorer) {
    auto pid = ProcessUtil::GetPidByExeName("explorer.exe");
    EXPECT_GT(pid, 0);
}

TEST(TestProcess, GetPidByExeName_NotExists) {
    auto pid = ProcessUtil::GetPidByExeName("this_should_not_exist_12345.exe");
    EXPECT_EQ(pid, 0);
}

TEST(TestProcess, GetCurrentSessionId) {
    auto sid = ProcessUtil::GetCurrentSessionId();
    EXPECT_NE(sid, static_cast<uint32_t>(-1));
}

TEST(TestProcess, GetProcessSessionId_Current) {
    auto sid = ProcessUtil::GetProcessSessionId(GetCurrentProcessId());
    EXPECT_NE(sid, static_cast<uint32_t>(-1));
}

TEST(TestProcess, GetProcessSessionId_Invalid) {
    auto sid = ProcessUtil::GetProcessSessionId(99999);
    EXPECT_EQ(sid, static_cast<uint32_t>(-1));
}

TEST(TestProcess, GetThreadCount) {
    auto count = ProcessUtil::GetThreadCount();
    EXPECT_GT(count, 0);
}

TEST(TestProcess, SetProcessInHighLevel) {
    // Just verify it doesn't crash
    EXPECT_NO_THROW(ProcessUtil::SetProcessInHighLevel());
}

TEST(TestProcess, RunAsAdminWithShell_Notepad) {
    // This test requires user interaction (UAC dialog), so we skip it in automated runs.
    GTEST_SKIP() << "RunAsAdminWithShell requires UAC interaction, skip automated test.";
}

TEST(TestProcess, StartProcessInWorkDir_Cmd) {
    auto temp_dir = std::filesystem::temp_directory_path().string();
    auto ret = ProcessUtil::StartProcessInWorkDir(temp_dir, "cmd.exe /c exit /b 0", {});
    EXPECT_TRUE(ret);
}

TEST(TestProcess, StartProcessAndOutput_ChineseOutput) {
    // chcp 65001 切换到 UTF-8 代码页，确保 echo 输出 UTF-8 编码
    auto output = ProcessUtil::StartProcessAndOutput("cmd.exe", {"/c", "chcp 65001 && echo 你好世界_测试"});
    EXPECT_FALSE(output.empty());
    bool found = false;
    for (const auto& line : output) {
        if (line.find("你好世界_测试") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Chinese output should be captured correctly";
}

TEST(TestProcess, StartProcessAndWait_ChinesePath) {
    auto temp_dir = std::filesystem::temp_directory_path() / "测试目录_中文_path";
    std::filesystem::create_directories(temp_dir);
    auto test_file = temp_dir / "中文文件.txt";
    auto cmd = std::string("cmd.exe /c echo hello > \"") + StringUtil::ToUTF8(test_file.wstring()) + "\"";
    auto ret = ProcessUtil::StartProcessAndWait("cmd.exe", {"/c", "echo", "hello", ">", StringUtil::ToUTF8(test_file.wstring())});
    EXPECT_TRUE(ret);
    EXPECT_TRUE(std::filesystem::exists(test_file)) << "Chinese path file should be created";
    std::filesystem::remove_all(temp_dir);
}

TEST(TestProcess, StartProcessInWorkDir_ChinesePath) {
    auto temp_dir = std::filesystem::temp_directory_path() / "中文工作目录_work_dir";
    std::filesystem::create_directories(temp_dir);
    // 在工作目录下创建标记文件，通过 cmd 验证工作目录是否生效
    auto marker = temp_dir / "marker.txt";
    {
        std::ofstream ofs(marker);
        ofs << "test";
    }
    auto ret = ProcessUtil::StartProcessInWorkDir(
        StringUtil::ToUTF8(temp_dir.wstring()),
        "cmd.exe /c if exist marker.txt (exit /b 0) else (exit /b 1)",
        {});
    EXPECT_TRUE(ret) << "Process should succeed in Chinese workdir";
    std::filesystem::remove_all(temp_dir);
}

TEST(TestProcess, StartProcess_ChineseExeName) {
    auto temp_dir = std::filesystem::temp_directory_path() / "中文exe测试目录";
    std::filesystem::create_directories(temp_dir);
    auto bat_file = temp_dir / "中文脚本_测试.bat";
    {
        std::ofstream ofs(bat_file, std::ios::binary);
        ofs << "@echo off\r\necho 中文脚本执行成功\r\n";
    }
    auto bat_path = StringUtil::ToUTF8(bat_file.wstring());
    auto output = ProcessUtil::StartProcessAndOutput("cmd.exe", {"/c", bat_path});
    bool found = false;
    for (const auto& line : output) {
        if (line.find("中文脚本执行成功") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Chinese batch script should execute and produce expected output";
    std::filesystem::remove_all(temp_dir);
}
