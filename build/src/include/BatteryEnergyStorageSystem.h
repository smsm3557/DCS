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
};

#endif // BATTERYENERGYSTORAGESYSTEM_H_INCLUDED
