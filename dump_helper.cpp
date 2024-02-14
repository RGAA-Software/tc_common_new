#include "dump_helper.h"
#include <time.h>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib,"Dbghelp.lib")
#include <io.h>
#include <direct.h> 
#include <Shlwapi.h>
#include <psapi.h>
#include <ShlObj.h>
#include <shlobj_core.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif
#include <stdint.h>
#include <string>
#define MAX_PATH_LEN 256
#ifdef _WIN32
#define ACCESS(fileName,accessMode) _access(fileName,accessMode)
#define MKDIR(path) _mkdir(path)
#else
#define ACCESS(fileName,accessMode) access(fileName,accessMode)
#define MKDIR(path) mkdir(path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#endif

#ifdef WIN32
#pragma comment(lib,"Shlwapi.lib")
#endif
#include "string_ext.h"

namespace tc {

static std::string _directory = ".\\";
static int _fileCount = 3;
#ifdef WIN32
typedef BOOL
(WINAPI
	* MyMiniDumpWriteDump)(
		IN HANDLE hProcess,
		IN DWORD ProcessId,
		IN HANDLE hFile,
		IN MINIDUMP_TYPE DumpType,
		IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL
		IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL
		IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL
		);


LONG __stdcall MyUnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo)
{
	MINIDUMP_EXCEPTION_INFORMATION ExpParam;

	HANDLE hCurrentProcess = GetCurrentProcess();
	TCHAR szFileName[MAX_PATH] = { 0 };
	GetModuleFileNameEx(hCurrentProcess, NULL, szFileName, _countof(szFileName));
	LPTSTR szDumpFileName = PathFindFileName(szFileName);
	PathRemoveExtension(szDumpFileName);
	PathAddExtension(szDumpFileName,TEXT(".dmp"));
	TCHAR szPath[MAX_PATH] = { 0 };
	GetModuleFileNameEx(hCurrentProcess, NULL, szPath, _countof(szPath));
	PathRemoveFileSpec(szPath);
	PathAppend(szPath, TEXT("dmp"));
	SHCreateDirectoryEx(NULL, szPath, NULL);
	PathAppend(szPath, szDumpFileName);

	HANDLE hFile = CreateFile(szPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		ExpParam.ThreadId = GetCurrentThreadId();
		ExpParam.ExceptionPointers = pExceptionInfo;
		ExpParam.ClientPointers = TRUE;

		HMODULE hModule = LoadLibrary(TEXT("dbghelp.dll"));
		if (hModule)
		{
			MyMiniDumpWriteDump _MiniDumpWriteDump = NULL;
			_MiniDumpWriteDump = (MyMiniDumpWriteDump)GetProcAddress(hModule, "MiniDumpWriteDump");
			if (_MiniDumpWriteDump)
			{
				_MiniDumpWriteDump(hCurrentProcess, GetCurrentProcessId(), hFile, MiniDumpWithFullMemory, &ExpParam, NULL, NULL);
			}
		}
		CloseHandle(hFile);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

void DumpHelper::WatchDump()
{
#ifdef WIN32
	SetErrorMode(SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX | SEM_NOALIGNMENTFAULTEXCEPT | SEM_FAILCRITICALERRORS);
	SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
#endif
	std::set_terminate([]() {
		printf("CRT terminate.");
		abort();
	});
}


#ifdef WIN32
static int GenerateDump(EXCEPTION_POINTERS* exceptionPointers, const std::string& name)
{
	HANDLE hCurrentProcess = GetCurrentProcess();
	TCHAR szFileName[MAX_PATH] = { 0 };
	GetModuleFileNameEx(hCurrentProcess, NULL, szFileName, _countof(szFileName));
	LPTSTR szDumpFileName = PathFindFileName(szFileName);
	PathRemoveExtension(szDumpFileName);
	PathAddExtension(szDumpFileName, TEXT(".snapshoot.dmp"));
	TCHAR szPath[MAX_PATH] = { 0 };
	GetModuleFileNameEx(NULL, NULL, szPath, _countof(szPath));
	PathRemoveFileSpec(szPath);
	PathAppend(szPath, TEXT("dmp"));
	SHCreateDirectoryEx(NULL, szPath, NULL);
	if(name.empty()) {
        PathAppend(szPath, szDumpFileName);
    }
	else {
        std::wstring wname = StringExt::ToWString(name);
		PathAppend(szPath, wname.c_str());
    }
	HANDLE hFile = ::CreateFile(szPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE != hFile)
	{
		MINIDUMP_EXCEPTION_INFORMATION minidumpExceptionInformation;
		minidumpExceptionInformation.ThreadId = GetCurrentThreadId();
		minidumpExceptionInformation.ExceptionPointers = exceptionPointers;
		minidumpExceptionInformation.ClientPointers = TRUE;
		bool isMiniDumpGenerated = MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			hFile,
			MiniDumpWithFullMemory,
			&minidumpExceptionInformation,
			nullptr,
			nullptr);

		CloseHandle(hFile);
		if (!isMiniDumpGenerated)
		{
			printf("MiniDumpWriteDump failed\n");
		}
	}
	else
	{
		printf("Failed to create dump file\n");
	}
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

void DumpHelper::Snapshot(const std::string& name)
{
#ifdef WIN32
	__try
	{
		//通过触发异常获取堆栈
		RaiseException(0xE0000001, 0, 0, 0);
	}
	__except (GenerateDump(GetExceptionInformation(),name)) {}
#endif
}
}