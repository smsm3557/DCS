#include "include/BatteryEnergyStorageSystem.h"

// map <property, value>>
// since i will use alot of string maps I created and alias
typedef std::map <std::string, std::string> block_map;

BatteryEnergyStorageSystem::BatteryEnergyStorageSystem (
	tsu::config_map& map) : 
	inverter_(map["Radian"]),
	bms_(map["BMS"]) {
	// start constructor

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
};  // end Query

void BatteryEnergyStorageSystem::SetChargeProfile () {

};