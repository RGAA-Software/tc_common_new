//
// Created by RGAA on 28/07/2025.
//

#include "auto_start.h"
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <comutil.h>
#include <atlcomcli.h>
#include "tc_common_new/log.h"

#define AUTO_RUN "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define AUTO_RUN_ADMIN "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run"

namespace tc
{

    static void SetAutoStartInternal(const QString& reg_path, const QString& exe_path, bool enabled) {
        QSettings settings(reg_path, QSettings::NativeFormat);

        //以程序名称作为注册表中的键,根据键获取对应的值（程序路径）
        QFileInfo fInfo(exe_path);
        QString name = fInfo.baseName();    //键-名称

        //如果注册表中的路径和当前程序路径不一样，则表示没有设置自启动或本自启动程序已经更换了路径
        QString oldPath = settings.value(name).toString(); //获取目前的值-绝对路劲
        QString newPath = QDir::toNativeSeparators(exe_path);    //toNativeSeparators函数将"/"替换为"\"
        if(enabled) {
            if (oldPath != newPath) {
                settings.setValue(name, newPath);
            }
        }
        else {
            settings.remove(name);
        }
    }

    void AutoStart::SetAutoStart(const QString& exe_path, bool enabled) {
        SetAutoStartInternal(AUTO_RUN, exe_path, enabled);
    }

    void AutoStart::SetAutoStartAdmin(const QString& exe_path, bool enabled) {
        SetAutoStartInternal(AUTO_RUN_ADMIN, exe_path, enabled);
    }

    AutoStart::AutoStart() {
        m_lpITS = NULL;
        m_lpRootFolder = NULL;

        HRESULT hr = CoInitialize(NULL);
        if (FAILED(hr)) {
            LOGE("AutoStart 初始化COM接口环境失败!");
            return;
        }

        //创建任务服务对象
        hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (LPVOID*)(&m_lpITS));
        if (FAILED(hr)) {
            LOGE("AutoStart 创建任务服务对象失败!");
            return;
        }

        //连接到任务服务
        hr = m_lpITS->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        if (FAILED(hr)) {
            LOGI("AutoStart 连接到任务服务失败!");
            return;
        }

        //获取根任务的指针
        //获取Root Task Folder 的指针 ，这个指针指向的是新注册的任务
        hr = m_lpITS->GetFolder(_bstr_t("\\"), &m_lpRootFolder);
        if (FAILED(hr)) {
            LOGI("AutoStart 获取根任务的指针失败!");
            return;
        }
    }

    AutoStart::~AutoStart() {
        if (m_lpITS) {
            m_lpITS->Release();
        }
        if (m_lpRootFolder) {
            m_lpRootFolder->Release();
        }
        CoUninitialize();
    }

    //创建任务计划
    BOOL AutoStart::NewTask(char* lpszTaskName, char* lpszProgramPath, char* lpszParameters, char* lpszAuthor) {
        if (NULL == m_lpRootFolder) {
            return FALSE;
        }

        //如果存在相同的计划任务，则删除
        Delete(lpszTaskName);

        //创建任务定义对象来创建任务
        ITaskDefinition *pTaskDefinition = NULL;
        HRESULT hr = m_lpITS->NewTask(0, &pTaskDefinition);
        if (FAILED(hr)) {
            LOGE("AutoStart 创建任务失败!");
            return FALSE;
        }
        //设置注册信息
        IRegistrationInfo *pRegInfo = NULL;
        CComVariant variantAuthor(NULL);
        variantAuthor = lpszAuthor;
        hr = pTaskDefinition->get_RegistrationInfo(&pRegInfo);
        if (FAILED(hr)) {
            LOGE("AutoStart 设置注册信息失败!");
            return FALSE;
        }
        // 设置作者信息
        hr = pRegInfo->put_Author(variantAuthor.bstrVal);
        pRegInfo->Release();
        // 设置登录类型和运行权限
        IPrincipal *pPrincipal = NULL;
        hr = pTaskDefinition->get_Principal(&pPrincipal);
        if (FAILED(hr)) {
            LOGE("设置登录类型和运行权限失败!");
            return FALSE;
        }
        // 设置登录类型
        hr = pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
        // 设置运行权限
        // 最高权限
        hr = pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
        pPrincipal->Release();
        // 设置其他信息
        ITaskSettings *pSettting = NULL;
        hr = pTaskDefinition->get_Settings(&pSettting);
        if (FAILED(hr)) {
            LOGE("AutoStart 设置其他信息失败!");
            return FALSE;
        }
        // 设置其他信息
        hr = pSettting->put_StopIfGoingOnBatteries(VARIANT_FALSE);
        hr = pSettting->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
        hr = pSettting->put_AllowDemandStart(VARIANT_TRUE);
        hr = pSettting->put_StartWhenAvailable(VARIANT_FALSE);
        hr = pSettting->put_MultipleInstances(TASK_INSTANCES_PARALLEL);

        //
        IIdleSettings* pIdleSettings = NULL;
        hr = pSettting->get_IdleSettings(&pIdleSettings);
        if (SUCCEEDED(hr)) {
            pIdleSettings->Release();
        }

        pSettting->put_StartWhenAvailable(VARIANT_TRUE);
        pSettting->put_RestartInterval(_bstr_t(L"PT1M"));
        pSettting->put_RestartCount(9999);

        pSettting->Release();
        // 创建执行动作
        IActionCollection *pActionCollect = NULL;
        hr = pTaskDefinition->get_Actions(&pActionCollect);
        if (FAILED(hr)) {
            LOGE("AutoStart 创建执行动作失败!");
            return FALSE;
        }
        IAction *pAction = NULL;
        // 创建执行操作
        hr = pActionCollect->Create(TASK_ACTION_EXEC, &pAction);
        pActionCollect->Release();
        // 设置执行程序路径和参数
        CComVariant variantProgramPath(NULL);
        CComVariant variantParameters(NULL);
        IExecAction *pExecAction = NULL;
        hr = pAction->QueryInterface(IID_IExecAction, (PVOID *)(&pExecAction));
        if (FAILED(hr)) {
            pAction->Release();
            LOGE("AutoStart 创建执行动作失败!");
            return FALSE;
        }
        pAction->Release();
        // 设置程序路径和参数
        variantProgramPath = lpszProgramPath;
        variantParameters = lpszParameters;
        pExecAction->put_Path(variantProgramPath.bstrVal);
        pExecAction->put_Arguments(variantParameters.bstrVal);
        pExecAction->Release();
        // 设置触发器信息，包括用户登录时触发
        ITriggerCollection *pTriggers = NULL;
        hr = pTaskDefinition->get_Triggers(&pTriggers);
        if (FAILED(hr)) {
            LOGE("AutoStart 设置触发器信息失败!");
            return FALSE;
        }
        // 创建触发器
        ITrigger *pTrigger = NULL;
        hr = pTriggers->Create(TASK_TRIGGER_LOGON, &pTrigger);
        if (FAILED(hr)) {
            LOGE("AutoStart 创建触发器失败!");
            return FALSE;
        }
        // 注册任务计划
        IRegisteredTask *pRegisteredTask = NULL;
        CComVariant variantTaskName(NULL);
        variantTaskName = lpszTaskName;
        hr = m_lpRootFolder->RegisterTaskDefinition(variantTaskName.bstrVal,
                                                    pTaskDefinition,
                                                    TASK_CREATE_OR_UPDATE,
                                                    _variant_t(),
                                                    _variant_t(),
                                                    TASK_LOGON_INTERACTIVE_TOKEN,
                                                    _variant_t(""),
                                                    &pRegisteredTask);
        if (FAILED(hr)) {
            pTaskDefinition->Release();
            LOGE("AutoStart 注册任务计划失败!");
            return FALSE;
        }
        pTaskDefinition->Release();
        pRegisteredTask->Release();
        return TRUE;
    }

    //删除计划任务
    BOOL AutoStart::Delete(char* lpszTaskName) {
        if(NULL == m_lpRootFolder) {
            return FALSE;
        }
        CComVariant variantTaskName(NULL);
        variantTaskName = lpszTaskName;
        HRESULT hr = m_lpRootFolder->DeleteTask(variantTaskName.bstrVal, 0);
        if (FAILED(hr)) {
            return FALSE;
        }
        return TRUE;
    }
}