#ifndef BATTERYENERGYSTORAGESYSTEM_H_INCLUDED
#define BATTERYENERGYSTORAGESYSTEM_H_INCLUDED

#include <string>
#include <map>
#include "DistributedEnergyResource.h"
#include "SunSpecModbus.h"
#include "tsu.h"

class BatteryEnergyStorageSystem : public DistributedEnergyResource {
    public:
        // constructor / destructor
        BatteryEnergyStorageSystem (tsu::config_map& map);
        virtual ~BatteryEnergyStorageSystem ();

        // overwrite public methods of DER
        void Loop (float delta_time);
        void Display ();

    private:
        // class composition
        SunSpecModbus inverter_;
        SunSpecModbus bms_;

    private:
        // overwrite private methods of DER
        void ImportPower ();
        void ExportPower ();
        void IdleLoss ();
        void Log ();
        void Query ();
        void SetChargeProfile ();
        void SetDischargeProfile ();
        void CheckBMSErrors (block_map& block);
        void CheckRadianErrors (block_map& block);

    private:
        // dynamic properties
        std::string bms_faults_;
        std::string bms_warnings_;
        std::string radian_faults_;
        std::string radian_warnings_;

};

#endif // BATTERYENERGYSTORAGESYSTEM_H_INCLUDED
