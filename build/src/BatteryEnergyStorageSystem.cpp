#include "include/BatteryEnergyStorageSystem.h"
#include "include/logger.h"

// map <property, value>>
// since i will use alot of string maps I created and alias
typedef std::map <std::string, std::string> block_map;

BatteryEnergyStorageSystem::BatteryEnergyStorageSystem (
	tsu::config_map& map) : 
	inverter_(map["Radian"]),
	bms_(map["BMS"]) {
	// start constructor
	SetLogPath (config_map["log_path"]);
	SetLogIncrement (config_map["log_inc"]);
};

BatteryEnergyStorageSystem::~BatteryEnergyStorageSystem () {
	// do nothing
};

void BatteryEnergyStorageSystem::Loop (float delta_time){

};

void BatteryEnergyStorageSystem::Display (){

};

void BatteryEnergyStorageSystem::ImportPower () {

};

void BatteryEnergyStorageSystem::ExportPower () {

};

void BatteryEnergyStorageSystem::IdleLoss (){

};

void BatteryEnergyStorageSystem::Log () {

};

// Query
// - read specific device information an update member properties
void BatteryEnergyStorageSystem::Query () {
	block_map ss_inverter = inverter_.ReadBlock (102);
	block_map radian_configs = inverter_.ReadBlock (64116);
	block_map aquion_bms = bms_.ReadBlock (64201);

	// update error/warning/event properties
	BatteryEnergyStorageSystem::CheckBMSErrors (aquion_bms);
	BatteryEnergyStorageSystem::CheckRadianErrors (ss_inverter);

	// sunspec power and energy properties
	float ac_amps = stof (ss_inverter["A"]);
	float ac_volts = stof (ss_inverter["PPVphAB"]);
	float ac_watts = stof (ss_inverter["W"]);
	float dc_volts = stof (ss_inverter["DCV"]);

	// radian power and energy properties
	float acl1_buy_amps 
		= stof (radian_configs["GS_Split_L1_Inverter_Buy_Current"]);
	float acl2_buy_amps 
		= stof (radian_configs["GS_Split_L2_Inverter_Buy_Current"]);
	float acl1_sell_amps 
		= stof (radian_configs["GS_Split_L1_Inverter_Sell_Current"]);
	float acl2_sell_amps 
		= stof (radian_configs["GS_Split_L2_Inverter_Sell_Current"]);
	float output_watts = stof (radian_configs["GS_Split_Output_kW"]) * 1000;
	float buy_watts = stof (radian_configs["GS_Split_Buy_kW"]) * 1000;
	float sell_watts = stof (radian_configs["GS_Split_Sell_kW"]) * 1000;
	float charge_watts = stof (radian_configs["GS_Split_Charge_kW"]) * 1000;
	float load_watts = stof (radian_configs["GS_Split_Load_kW"]) * 1000;

	// bms power and energy properties
	float soc = stof (aquion_bms["soc"]) / 100;
	float rated_watt_hours = stof (aquion_bms["rated_energy"]) * 1000;

};  // end Query


// Set Charge Profile
// - sets the battery charge profile and inverter charge configs at the start
// - of the program. Most of the values will not be changed after initialization
void BatteryEnergyStorageSystem::SetChargeProfile () {
	block_map point;
	// each point must be created, written, then cleared
	point ["GSconfig_Absorb_Volts"] = "57.6";
	inverter_.WritePoint (64116, point);
	point.clear();

	point ["GSconfig_Absorb_Time_Hours"] = "10";
	inverter_.WritePoint (64116, point);
	point.clear();

	point ["GSconfig_Float_Volts"] = "54.4";
	inverter_.WritePoint (64116, point);
	point.clear();

	point ["GSconfig_Float_Time_Hours"] = "10";
	inverter_.WritePoint (64116, point);
	point.clear();

	point ["GSconfig_ReFloat_Volts"] = "42";
	inverter_.WritePoint (64116, point);
	point.clear();

	point ["GSconfig_Sell_Volts"] = "60";
	inverter_.WritePoint (64116, point);
	point.clear();

	point ["GSconfig_Charger_AC_Input_Current_Limit"] = "500";
	inverter_.WritePoint (64116, point);
	point.clear();

	point ["GSconfig_Charger_Operating_Mode"] 
		= "ALL_INVERTER_CHARGING_DISABLED";
	inverter_.WritePoint (64116, point);
	point.clear();

	point ["GSconfig_Module_Control"] = "BOTH";
	inverter_.WritePoint (64116, point);
	point.clear();

	point ["GSconfig_Model_Select"] = "FULL";
	inverter_.WritePoint (64116, point);
	point.clear();
};  // end Set Charge Profile

void BatteryEnergyStorageSystem::SetDischargeProfile () {

};

// Check BMS Errors
// - check the current fault/warning codes agains previous and if they are the
// - same then do not log again.
void BatteryEnergyStorageSystem::CheckBMSErrors (block_map& block) {
	if (block["fault_code"] != bms_faults_) {
		bms_faults_ = block["fault_code"];
		Logger ("ERROR", GetLogPath ()) << "BMS Fault:" << bms_faults_;
	}

	if (block["warning_code"] != bms_warnings_) {
		bms_warnings_ = block["fault_code"];
		Logger ("ERROR", GetLogPath ()) << "BMS Warning:" << bms_warnings_;
	}
};  // end Check BMS Errors

// Check Radian Errors
// - check the current fault/warning codes against previous and if they are the
// - same the do not log again.
void BatteryEnergyStorageSystem::CheckRadianErrors (block_map& block) {
	if (block["Evt1"] != radian_events_) {
		radian_errors_ = block["Evt1"];
		Logger ("ERROR", GetLogPath ()) << "Radian Event:" << radian_events_
	}
};  // end Check Radian Errors