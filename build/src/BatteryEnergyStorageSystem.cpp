// INCLUDES
#include <iostream>
#include <ctime>
#include "include/BatteryEnergyStorageSystem.h"
#include "include/logger.h"

// Constructor
BatteryEnergyStorageSystem::BatteryEnergyStorageSystem (
	tsu::config_map& map) : 
	inverter_(map["Radian"]),
	bms_(map["BMS"]),
	last_control_(0),
	last_log_(0) {
	// start constructor
	SetLogPath (map["BESS"]["log_path"]);
	SetLogIncrement (stoul(map["BESS"]["log_inc"]));

	// set rated properties and query for dynamic properties
	BatteryEnergyStorageSystem::GetRatedProperties ();
	BatteryEnergyStorageSystem::Query ();
	BatteryEnergyStorageSystem::SetRadianConfigurations ();
};

// Destructor
BatteryEnergyStorageSystem::~BatteryEnergyStorageSystem () {
	block_map point;

	// reset charge current to default
	point ["GSconfig_Charger_AC_Input_Current_Limit"] = "30";
	inverter_.WritePoint (64116, point);
	point.clear();

	// reset sell current to default
	point ["OB_Set_Radian_Inverter_Sell_Current_Limit"] = "30";
	inverter_.WritePoint (64120, point);
	point.clear();
};


// Loop
// - this is the main process function that will run in its own thread from main
// - use limit checks to restrict function calls to a specific frequency
void BatteryEnergyStorageSystem::Loop (float delta_time){
	(void)delta_time;  // not used in this function for now

	unsigned int utc = time (0);
	bool five_seconds = (utc % 5 == 0);  // the inverter is slow to process 
	bool one_minute = (utc % 60 == 0);	 // logging once a minute should be fine

	// the frequency bool must be checked with the last control/log because the
	// main thread freqency is 0.5 seconds which leads to 2ish calls per second
	if (five_seconds && utc != last_control_) {
		last_control_ = utc;
		BatteryEnergyStorageSystem::Query ();
		if (GetImportWatts () > 0) {
			BatteryEnergyStorageSystem::ImportPower ();
		} else if (GetExportWatts () > 0) {
			BatteryEnergyStorageSystem::ExportPower ();
		} else {
			BatteryEnergyStorageSystem::Idleloss ();
		}
	}
	if (one_minute && utc != last_log_) {
		last_log_ = utc;
		BatteryEnergyStorageSystem::Log ();
	}
};  // end Loop

// Display
void BatteryEnergyStorageSystem::Display (){
	std::cout << "[Properties]"
		<< "\n\tExport Watts:\t" << GetExportWatts ()
		<< "\n\tExport Power:\t" << GetExportPower ()
		<< "\n\tExport Energy:\t" << GetRatedExportEnergy ()
		<< "\n\tImport Watts:\t" << GetImportWatts ()
		<< "\n\tImport Watts:\t" << GetImportPower ()
		<< "\n\tImport Watts:\t" << GetRatedImportEnergy ()
		<< "\n\tRadian Mode:\t" << radian_mode_ << std::endl;;
};  // end Display

// Import Power
// - check the mode and if it is not charging then set the required registers
// - for a charge. Then calculate the required current setting for the control
// - watts.
void BatteryEnergyStorageSystem::ImportPower () {
	block_map point;
	std::cout << "MODE: " << radian_mode_ << std::endl;
	if (radian_mode_ != "CHARGING") {
		// each point must be created, written, then cleared
		point ["GSconfig_Sell_Volts"] = "64";
		inverter_.WritePoint (64116, point);
		point.clear();

		point ["GSconfig_Charger_Operating_Mode"] 
			= "BULK_AND_FLOAT_CHARGING_ENABLED";
		inverter_.WritePoint (64116, point);
		point.clear();

		point ["OB_Bulk_Charge_Enable_Disable"] = "START_BULK";
		inverter_.WritePoint (64120, point);
		point.clear();
	}

	unsigned int real_watts = GetImportPower ();
	unsigned int control_watts = GetImportWatts ();
	if (real_watts > 1.1*control_watts || real_watts < 0.9*control_watts) {
		float current_limit = GetImportWatts () / split_vac_;
		point ["GSconfig_Charger_AC_Input_Current_Limit"] 
			= std::to_string (current_limit);
		inverter_.WritePoint (64116, point);
		point.clear();
	}
};  // end Import Power

// Export Power
// - check the mode and if it is not selling then set the required registers
// - for a discharge. Then calculate the required current setting for the
// - control watts.
void BatteryEnergyStorageSystem::ExportPower () {
	block_map point;
	if (radian_mode_ != "SELLING") {
		// each point must be created, written, then cleared
		point ["GSconfig_Charger_Operating_Mode"] 
			= "ALL_INVERTER_CHARGING_DISABLED";
		inverter_.WritePoint (64116, point);
		point.clear();

		point ["GSconfig_Sell_Volts"] = "44";
		inverter_.WritePoint (64116, point);
		point.clear();
	}

	unsigned int real_watts = GetExportPower ();
	unsigned int control_watts = GetExportWatts ();
	if (real_watts > 1.1*control_watts || real_watts < 0.9*control_watts) {
		float current_limit = GetImportWatts () / split_vac_;
		point ["OB_Set_Radian_Inverter_Sell_Current_Limit"] 
			= std::to_string (current_limit);
		inverter_.WritePoint (64120, point);
		point.clear();
	}
};  // end Export Power

// Idle Loss
// - This function disables both importing from and exporting to the grid
void BatteryEnergyStorageSystem::IdleLoss (){
	block_map point;
	if (radian_mode_ == "CHARGING" || radian_mode_ == "SELLING") {
		// each point must be created, written, then cleared
		point ["GSconfig_Charger_Operating_Mode"] 
			= "ALL_INVERTER_CHARGING_DISABLED";
		inverter_.WritePoint (64116, point);
		point.clear();

		point ["GSconfig_Sell_Volts"] 
			= "64";
		inverter_.WritePoint (64116, point);
		point.clear();
	}
};  // end Idle Loss

// Log
void BatteryEnergyStorageSystem::Log () {
	Logger ("DATA", GetLogPath ()) 
		<< "E: W, P, E, I: W, P, E, M"
		<< GetExportWatts ()
		<< GetExportPower ()
		<< GetRatedExportEnergy ()
		<< GetImportWatts ()
		<< GetImportPower ()
		<< GetRatedImportEnergy ()
		<< radian_mode_;
};  // end Log

// Get Rated Properties
// - read specific device information an update member properties
void BatteryEnergyStorageSystem::GetRatedProperties () {
	block_map radian_configs = inverter_.ReadBlock (64116);
	block_map aquion_bms = bms_.ReadBlock (64201);

	// radian power and energy properties
	float rated_ac_amps 
		= stof (radian_configs["GSconfig_Charger_AC_Input_Current_Limit"]);
	float rated_ac_volts = stof (radian_configs["GSconfig_AC_Output_Voltage"]);
	split_vac_ = rated_ac_volts;

	// assume import and export are the same
	unsigned int rated_power = rated_ac_volts * rated_ac_amps;
	SetRatedImportPower (rated_power);
	SetRatedExportPower (rated_power);

	// bms power and energy properties
	float rated_watt_hours = stof (aquion_bms["rated_energy"]) * 1000;

	// assume import/export are the same
	unsigned int rated_energy = rated_watt_hours * 100;  // fully charged
	SetRatedImportEnergy (rated_energy);
	SetRatedExportEnergy (rated_energy);

	// the ramp is still not clearly defined by register values, so I will just
	// use the recored ramp time for now.
	SetImportRamp (100);
	SetExportRamp (100);
};  // end Get Rated Properties

// Query
// - read specific device information an update member properties
void BatteryEnergyStorageSystem::Query () {
	block_map ss_inverter = inverter_.ReadBlock (102);
	block_map radian_split = inverter_.ReadBlock (64115);
	block_map aquion_bms = bms_.ReadBlock (64201);

	// update error/warning/event properties
	BatteryEnergyStorageSystem::CheckBMSErrors (aquion_bms);
	BatteryEnergyStorageSystem::CheckRadianErrors (ss_inverter);

	// sunspec power and energy properties
	float ac_amps = stof (ss_inverter["A"]);
	float ac_volts = stof (ss_inverter["PPVphAB"]);
	float ac_watts = stof (ss_inverter["W"]);
	float dc_volts = stof (ss_inverter["DCV"]);

	std::cout << "DEBUG (sunspec values):"
		<< "\n\tAac: " << ac_amps
		<< "\n\tVac: " << ac_volts
		<< "\n\tWac: " << ac_watts
		<< "\n\tVdc: " << dc_volts << std::endl;

	// radian power and energy properties
	float acl1_buy_amps 
		= stof (radian_split["GS_Split_L1_Inverter_Buy_Current"]);
	float acl2_buy_amps 
		= stof (radian_split["GS_Split_L2_Inverter_Buy_Current"]);
	float acl1_sell_amps 
		= stof (radian_split["GS_Split_L1_Inverter_Sell_Current"]);
	float acl2_sell_amps 
		= stof (radian_split["GS_Split_L2_Inverter_Sell_Current"]);
	float output_watts = stof (radian_split["GS_Split_Output_kW"]) * 1000;
	float buy_watts = stof (radian_split["GS_Split_Buy_kW"]) * 1000;
	float sell_watts = stof (radian_split["GS_Split_Sell_kW"]) * 1000;
	float charge_watts = stof (radian_split["GS_Split_Charge_kW"]) * 1000;
	float load_watts = stof (radian_split["GS_Split_Load_kW"]) * 1000;
	radian_mode_ = radian_split["GS_Split_Inverter_Operating_mode"];

	std::cout << "DEBUG (radian values):"
		<< "\n\tL1 buy Aac: " << acl1_buy_amps
		<< "\n\tL2 buy Aac: " << acl2_buy_amps
		<< "\n\tL1 sell Aac: " << acl1_sell_amps
		<< "\n\tL2 sell Aac: " << acl2_sell_amps
		<< "\n\toutput watts: " << output_watts 
		<< "\n\tbuy watts: " << buy_watts
		<< "\n\tsell watts: " << sell_watts
		<< "\n\tcharge watts: " << charge_watts
		<< "\n\tload watts: " << load_watts << std::endl;

	// bms power and energy properties
	float soc = stof (aquion_bms["soc"]) / 100;

	// set DER properties
	SetImportPower (buy_watts);
	SetExportPower (sell_watts);
	float import_energy = soc * GetRatedImportEnergy ();
	float export_energy = (1 - soc) * GetRatedExportEnergy ();
	SetImportEnergy (import_energy);
	SetExportEnergy (export_energy);

};  // end Query


// Set Radian Configurations
// - sets the battery charge profile and inverter charge configs at the start
// - of the program. Most of the values will not be changed after initialization
void BatteryEnergyStorageSystem::SetRadianConfigurations () {
	// each point must be created, written, then cleared
	block_map point;

	// OutBack has ranges fore most config registers found in Table 16 of the
	// manual.
	point ["GSconfig_Absorb_Volts"] = "57.6";  // range: 44 to 64 volts
	inverter_.WritePoint (64116, point);

	point ["GSconfig_Absorb_Time_Hours"] = "10";  // range: 0 to 24 hours
	inverter_.WritePoint (64116, point);
	point.clear();

	point ["GSconfig_Float_Volts"] = "54.4";  // range: 44 to 64 volts
	inverter_.WritePoint (64116, point);
	point.clear();

	point ["GSconfig_Float_Time_Hours"] = "10";  // range: 0 to 24/7 hours
	inverter_.WritePoint (64116, point);
	point.clear();

	point ["GSconfig_ReFloat_Volts"] = "44";  // range: 44 to 64 volts
	inverter_.WritePoint (64116, point);
	point.clear();

	point ["GSconfig_Sell_Volts"] = "64";  // range: 44 to 64 volts
	inverter_.WritePoint (64116, point);
	point.clear();

	point ["GSconfig_Low_Battery_Cut_Out_Voltage"] = "64";  // range: 36 to 48
	inverter_.WritePoint (64116, point);
	point.clear();
	
	point ["GSconfig_Low_Battery_Cut_In_Voltage"] = "64";  // range: 40 - 56
	inverter_.WritePoint (64116, point);
	point.clear();

	point ["GSconfig_Charger_AC_Input_Current_Limit"] = "30";  // range: 0 - 30
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

	point ["GSconfig_Grid_Tie_Enable"] = "YES";
	inverter_.WritePoint (64116, point);
	point.clear();
};  // end Set Radian Configurations

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
		radian_events_ = block["Evt1"];
		Logger ("ERROR", GetLogPath ()) << "Radian Event:" << radian_events_;
	}
};  // end Check Radian Errors