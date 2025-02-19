//
// Created by RGAA on 2023/8/16.
//

#include "ip_util.h"

#include "tc_common_new/log.h"

#ifdef _OS_WINDOWS_

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <iphlpapi.h>
#include <iptypes.h>
#include <cstdlib>
#include <cstring>
#include <map>

#pragma comment(lib, "IPHLPAPI.lib")

#endif

#include <QString>
#include <QtNetwork/QNetworkInterface>

namespace tc
{

    static bool NeedIgnoreNetwork(const QString& str_card_name) {
        if (-1 != str_card_name.indexOf("VMware")
            || -1 != str_card_name.indexOf("Loopback")
            || -1 != str_card_name.indexOf("VirtualBox")
            || -1 != str_card_name.indexOf("WSL")) {
            return true;
        }
        return false;
    }

    // 0 - wireless , 1 - wire
    static void GetIps(std::vector<EthernetInfo>& all_et_info) {
        QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
        QList<QNetworkAddressEntry> entry;
        for(QNetworkInterface inter : interfaces) {
            LOGI("human readable name: {}", inter.humanReadableName().toStdString());
            if (NeedIgnoreNetwork(inter.humanReadableName())) {
                continue;
            }

            if (inter.flags().testFlag(QNetworkInterface::IsLoopBack)
                || inter.flags().testFlag(QNetworkInterface::IsPointToPoint)) {
                continue;
            }

            auto mac_address = inter.hardwareAddress().toStdString();

            if (inter.flags() & (QNetworkInterface::IsUp | QNetworkInterface::IsRunning)) {
                entry = inter.addressEntries();
                int cnt = entry.size() - 1;
                for (int i = 1; i <= cnt; ++i) {
                    if (entry.at(i).ip().protocol() == QAbstractSocket::IPv4Protocol) {
                        auto ip = entry.at(i).ip().toString().toStdString();
                        auto broadcast = entry.at(i).broadcast().toString().toStdString();
                        LOGI("IP: {}, broadcast: {}", ip, broadcast);
                        if (-1 != inter.name().indexOf("wireless")) {
                            all_et_info.push_back(EthernetInfo{
                                .human_readable_name_ = inter.humanReadableName().toStdString(),
                                .ip_addr_ = ip,
                                .nt_type_ = IPNetworkType::kWireless,
                                .mac_address_ = mac_address,
                            });
                        } else if (-1 != inter.name().indexOf("ethernet")) {
                            LOGI("Net Name: {}", inter.name().toStdString());
                            all_et_info.push_back(EthernetInfo{
                                    .human_readable_name_ = inter.humanReadableName().toStdString(),
                                    .ip_addr_ = entry.at(i).ip().toString().toStdString(),
                                    .nt_type_ = IPNetworkType::kWired,
                                    .mac_address_ = mac_address,
                            });
                        }
                    }
                }
                entry.clear();
            }
        }
    }

    std::vector<EthernetInfo> IPUtil::ScanIPs() {
        std::vector<EthernetInfo> ips;
        GetIps(ips);
        return ips;
    }

}
