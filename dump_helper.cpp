#include "dump_helper.h"

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <shellapi.h>
#include <DbgHelp.h>
#include <Psapi.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <shlobj_core.h>
#include <tchar.h>
#include <tlhelp32.h>
#include <wtsapi32.h>
#include <winternl.h>

#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "ntdll.lib")

namespace tc
{
    typedef BOOL (WINAPI *FuncMiniDumpWriteDump)(
            IN HANDLE hProcess,
            IN DWORD ProcessId,
            IN HANDLE hFile,
            IN MINIDUMP_TYPE DumpType,
            IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL
            IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL
            IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL
    );


    LONG __stdcall UnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo) {
        MINIDUMP_EXCEPTION_INFORMATION ExpParam;
        HANDLE hCurrentProcess = GetCurrentProcess();
        TCHAR szFileName[MAX_PATH] = {0};
        GetModuleFileNameEx(hCurrentProcess, NULL, szFileName, _countof(szFileName));
        LPTSTR szDumpFileName = PathFindFileName(szFileName);
        PathRemoveExtension(szDumpFileName);
        PathAddExtension(szDumpFileName, L".dmp");
        TCHAR szPath[MAX_PATH] = {0};
        GetModuleFileNameEx(hCurrentProcess, NULL, szPath, _countof(szPath));
        PathRemoveFileSpec(szPath);
        PathAppend(szPath, L"dmp");
        SHCreateDirectoryEx(NULL, szPath, NULL);
        PathAppend(szPath, szDumpFileName);

        HANDLE hFile = CreateFile(szPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0,
                                  CREATE_ALWAYS, 0, 0);
        if (hFile != INVALID_HANDLE_VALUE) {
            ExpParam.ThreadId = GetCurrentThreadId();
            ExpParam.ExceptionPointers = pExceptionInfo;
            ExpParam.ClientPointers = TRUE;

            HMODULE hModule = LoadLibraryA("dbghelp.dll");
            if (hModule) {
                FuncMiniDumpWriteDump dumpWriteFunc = NULL;
                dumpWriteFunc = (FuncMiniDumpWriteDump) GetProcAddress(hModule, "MiniDumpWriteDump");
                if (dumpWriteFunc) {
                    dumpWriteFunc(hCurrentProcess, GetCurrentProcessId(), hFile, MiniDumpWithFullMemory, &ExpParam,
                                  NULL, NULL);
                }
            }
            CloseHandle(hFile);
        }
        return EXCEPTION_EXECUTE_HANDLER;
    }

    void CaptureDump() {
        SetErrorMode(
                SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX | SEM_NOALIGNMENTFAULTEXCEPT | SEM_FAILCRITICALERRORS);
        SetUnhandledExceptionFilter(UnhandledExceptionFilter);
    }
}
#endif