//
// Created by RGAA on 2024-01-18.
//

#include "hardware.h"

#include <iostream>
#include <Windows.h>
#include <wbemidl.h>
#include <comdef.h>
#include "tc_common_new/log.h"
#include "tc_common_new/string_ext.h"
#include <QList>
#include <QtNetwork/QNetworkInterface>

#pragma comment(lib, "wbemuuid.lib")

namespace tc
{

    int Hardware::Detect(bool cpu, bool disk, bool driver) {
        CoUninitialize();
        HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
        if (FAILED(hres)) {
            LOGE("COM library initialization failed.");
            return -1;
        }

        hres = CoInitializeSecurity(
                NULL,
                -1,
                NULL,
                NULL,
                RPC_C_AUTHN_LEVEL_DEFAULT,
                RPC_C_IMP_LEVEL_IMPERSONATE,
                NULL,
                EOAC_NONE,
                NULL
        );
        if (FAILED(hres)) {
            LOGE("Failed to initialize security: {:x}", hres);
            CoUninitialize();
            return -1;
        }

        IWbemLocator *pLoc = NULL;
        hres = CoCreateInstance(
                CLSID_WbemLocator,
                0,
                CLSCTX_INPROC_SERVER,
                IID_IWbemLocator,
                (LPVOID *) &pLoc
        );
        if (FAILED(hres)) {
            LOGE("Failed to create IWbemLocator object.");
            CoUninitialize();
            return -1;
        }

        IWbemServices *pSvc = NULL;
        hres = pLoc->ConnectServer(
                _bstr_t(L"ROOT\\CIMV2"),
                NULL,
                NULL,
                0,
                NULL,
                0,
                0,
                &pSvc
        );
        if (FAILED(hres)) {
            LOGE("Could not connect to WMI server.");
            pLoc->Release();
            CoUninitialize();
            return -1;
        }

        hres = CoSetProxyBlanket(
                pSvc,
                RPC_C_AUTHN_WINNT,
                RPC_C_AUTHZ_NONE,
                NULL,
                RPC_C_AUTHN_LEVEL_CALL,
                RPC_C_IMP_LEVEL_IMPERSONATE,
                NULL,
                EOAC_NONE
        );
        if (FAILED(hres)) {
            LOGE("Could not set proxy blanket.");
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            return -1;
        }

        IEnumWbemClassObject *pEnumerator = NULL;
        IWbemClassObject *pclsObj = NULL;
        ULONG uReturn = 0;

        hw_disks_.clear();
        drivers_.clear();

        // CPU
        if (cpu) {
            hres = pSvc->ExecQuery(
                    bstr_t("WQL"),
                    bstr_t("SELECT * FROM Win32_Processor"),
                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                    NULL,
                    &pEnumerator
            );

            if (FAILED(hres)) {
                LOGE("Query for processor information failed.");
                pSvc->Release();
                pLoc->Release();
                CoUninitialize();
                return -1;
            }

            while (pEnumerator) {
                HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (0 == uReturn || FAILED(hr)) {
                    break;
                }

                VARIANT vtProp;
                hr = pclsObj->Get(L"ProcessorId", 0, &vtProp, 0, 0);  // "ProcessorId" 是属性名，不同的信息需要查询不同的属性名
                if (SUCCEEDED(hr)) {
                    hw_cpu_.id_ = StringExt::ToUTF8(vtProp.bstrVal);
                }
                VariantClear(&vtProp);
                pclsObj->Release();
            }
        }

        if (disk) {
            hres = pSvc->ExecQuery(
                    bstr_t("WQL"),
                    bstr_t("SELECT * FROM Win32_DiskDrive"),
                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                    NULL,
                    &pEnumerator);

            if (FAILED(hres)) {
                LOGE("Query for operating system name failed, Error code = {}", hres);
                pSvc->Release();
                pLoc->Release();
                CoUninitialize();
                return -1; // Program has failed.
            }

            uReturn = 0;
            while (pEnumerator) {
                HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (0 == uReturn || FAILED(hr)) {
                    break;
                }

                HwDisk disk{};

                VARIANT vtProp;

                // Get the value of the Name property
                hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
//            std::wcout << " Disk Name : " << vtProp.bstrVal << std::endl;
                disk.name_ = StringExt::ToUTF8(vtProp.bstrVal);
                VariantClear(&vtProp);

                hr = pclsObj->Get(L"Model", 0, &vtProp, 0, 0);
//            std::wcout << " Disk Model : " << vtProp.bstrVal << std::endl;
                disk.model_ = StringExt::ToUTF8(vtProp.bstrVal);
                VariantClear(&vtProp);

                hr = pclsObj->Get(L"Status", 0, &vtProp, 0, 0);
//            std::wcout << " Status : " << vtProp.bstrVal << std::endl;
                disk.status_ = StringExt::ToUTF8(vtProp.bstrVal);
                VariantClear(&vtProp);

                hr = pclsObj->Get(L"DeviceID", 0, &vtProp, 0, 0);
//            std::wcout << " Device ID : " << vtProp.bstrVal << std::endl;
                VariantClear(&vtProp);


                hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
//            std::wcout << " SerialNumber : " << vtProp.bstrVal << std::endl;
                disk.serial_number_ = StringExt::ToUTF8(vtProp.bstrVal);
                VariantClear(&vtProp);

                //InterfaceType
                hr = pclsObj->Get(L"InterfaceType", 0, &vtProp, 0, 0);
                disk.interface_type_ = StringExt::ToUTF8(vtProp.bstrVal);
                VariantClear(&vtProp);

                pclsObj->Release();

                if (disk.interface_type_ == "IDE") {
                    hw_disks_.push_back(disk);
                }
            }
        }

        // system driver
        if (driver) {
            hres = pSvc->ExecQuery(
                    bstr_t("WQL"),
                    bstr_t("SELECT * FROM Win32_SystemDriver"),
                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                    NULL,
                    &pEnumerator);

            if (FAILED(hres)) {
                LOGE("Query for SystemDriver name failed.  Error code = {}", hres);
                pSvc->Release();
                pLoc->Release();
                CoUninitialize();
                return -1;
            }

            uReturn = 0;
            while (pEnumerator) {
                HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (0 == uReturn || FAILED(hr)) {
                    break;
                }

                VARIANT vtProp;
                SysDriver driver;

                hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
                if (FAILED(hr)) {
                    continue;
                }
                auto name = StringExt::ToUTF8(vtProp.bstrVal);
                driver.name_ = name;
                VariantClear(&vtProp);

                hr = pclsObj->Get(L"DisplayName", 0, &vtProp, 0, 0);
                if (FAILED(hr)) {
                    continue;
                }
                auto display_name = StringExt::ToUTF8(vtProp.bstrVal);
                driver.display_name_ = display_name;
                VariantClear(&vtProp);

                hr = pclsObj->Get(L"State", 0, &vtProp, 0, 0);
                if (FAILED(hr)) {
                    continue;
                }
                auto state = StringExt::ToUTF8(vtProp.bstrVal);
                driver.state_ = state;
                VariantClear(&vtProp);

                drivers_.push_back(driver);

                pclsObj->Release();
            }
        }

        // 清理资源
        pSvc->Release();
        pLoc->Release();
        pEnumerator->Release();
        CoUninitialize();

        //DetectMac();

        return 0;
    }

    void Hardware::Dump() {
        LOGI("==> CPU id: {}", hw_cpu_.id_);
        LOGI("==> Total disks: {}", hw_disks_.size());
        for (auto &disk: hw_disks_) {
            LOGI("  name: {}", disk.name_);
            LOGI("  model: {}", disk.model_);
            LOGI("  status: {}", disk.status_);
            LOGI("  serial number: {}", disk.serial_number_);
            LOGI("  interface type: {}", disk.interface_type_);
            LOGI("-------------------------------------");
        }
        //LOGI("Mac address: {}", mac_address_);
    }

    std::string Hardware::GetHardwareDescription() {
        std::stringstream ss;
        for (auto &disk: hw_disks_) {
            ss << disk.serial_number_;
        }
        //ss << mac_address_;
        return ss.str();
    }

    void Hardware::DetectMac() {
        QList<QNetworkInterface> nets = QNetworkInterface::allInterfaces();// 获取所有网络接口列表
        int nCnt = nets.count();
        QString strMacAddr = "";
        for(int i = 0; i < nCnt; i ++) {
            if(nets[i].flags().testFlag(QNetworkInterface::IsUp)
                && nets[i].flags().testFlag(QNetworkInterface::IsRunning)
                && !nets[i].flags().testFlag(QNetworkInterface::IsLoopBack))
            {
                strMacAddr = nets[i].hardwareAddress();
                mac_address_ = strMacAddr.toStdString();
                LOGI("Net adapter name: {}, MAC: {}", nets[i].name().toStdString(), mac_address_);
                //break;
            }
        }
    }

}