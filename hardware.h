//
// Created by RGAA on 2024-01-18.
//

#ifndef CONTROLLER_HARDWARE_H
#define CONTROLLER_HARDWARE_H

#include <string>
#include <vector>

namespace tc
{

    class HwCPU {
    public:
        std::string id_;
    };

    class HwDisk {
    public:
        std::string name_;
        std::string model_;
        std::string status_;
        std::string serial_number_;
        std::string interface_type_;
    };

    class SysDriver {
    public:
        std::string name_;
        std::string display_name_;
        std::string state_;
    };

    class Hardware {
    public:

        static Hardware* Instance() {
            static Hardware hw;
            return &hw;
        }

        int Detect(bool cpu, bool disk, bool driver);
        void Dump();
        std::string GetHardwareDescription();
        std::vector<SysDriver> GetDrivers() { return drivers_; }

    private:
        void DetectMac();

    public:
        HwCPU hw_cpu_;
        std::vector<HwDisk> hw_disks_;
        std::vector<SysDriver> drivers_;
        std::string mac_address_;
    };

}

#endif //PLC_CONTROLLER_HARDWARE_H
