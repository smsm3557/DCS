#ifndef SUNSPECMODBUS_H
#define SUNSPECMODBUS_H

// INCLUDES
#include <string>
#include <vector>
#include <modbus/modbus-tcp.h>
#include "SunSpecModel.h"

class SunSpecModbus {
public:
    SunSpecModbus (std::string ip, unsigned int port);
    ~SunSpecModbus ();
    void Querry ();
    void ReadRegisters (unsigned int offset,
                        unsigned int length,
                        uint16_t *reg_ptr);
    void WriteRegisters (unsigned int offset,
                         unsigned int length,
                         const uint16_t *reg_ptr);

    void ReadBlock (std::string name);

    void WriteBlock (std::string name);

private:
    modbus_t* context_ptr_;
    std::vector <std::shared_ptr <SunSpecModel>> models_;
};

#endif // SUNSPECMODBUS_H
