#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include "include/SunSpecModbus.h"

SunSpecModbus::SunSpecModbus (std::map <std::string, std::string>& configs) 
    : model_path_(configs["path"]), sunspec_key_(stoul(configs["key"])) {
    // create modbus context pointer and connect to device at
    // given ip address and port number.
    const char* ip = configs["ip"].c_str ();
    unsigned int port = stoul(configs["port"]);
    context_ptr_ = modbus_new_tcp(ip, port);
    if (modbus_connect(context_ptr_) == -1) {
        std::cout << "[ERROR] : " << modbus_strerror(errno) << '\n';
    }

    unsigned int did = stoul(configs["did"]);
    SunSpecModbus::Query (did);
}

SunSpecModbus::~SunSpecModbus () {
    models_.clear ();
    modbus_close(context_ptr_);
    modbus_free(context_ptr_);
}

// Querry
// - Read all available registers to find sunspec compliant blocks.
// - The first step is to read the first two register and see if the modbus
// - device is a sunspec complient device.
// - Then traverse register array and find sunspec DID values and create the
// - sunspec model.
// - If it is not a sunspec complient device then use the given did to simulate
// - a sunspec model.
void SunSpecModbus::Query (unsigned int did) {
    uint16_t sunspec_id[2];
    // TODO (TS): sunspec states the holding register start can be
    // - 30, 40, or 50,000.
    // - 40000 is the preferece starting location
    unsigned int id_offset = 0;
    SunSpecModbus::ReadRegisters(id_offset, 2, sunspec_id);
    uint32_t device_key = MODBUS_GET_INT32_FROM_INT16(sunspec_id,0);

    // If match then increment offset by id length and read next two registers
    // to get DID and lenth of sunspec block
    if (device_key == sunspec_key_) {
        id_offset =+ 2; 
        uint16_t did_and_length[2];
        SunSpecModbus::ReadRegisters(id_offset, 2, did_and_length);
        std::string filepath = SunSpecModbus::FormatModelPath (
                did_and_length[0]
        );

        // while the next model file exists, create model and increment offset
        struct stat buffer;  // used to check if file exists
        while ((stat (filepath.c_str (), &buffer) == 0)) {
            std::shared_ptr <SunSpecModel> model (
                new SunSpecModel (did_and_length[0], id_offset, filepath)
            );
            models_.push_back (std::move (model));
            id_offset += did_and_length[1] + 1; // block length not model length
            std::cout << "offset " << id_offset << std::endl;
            SunSpecModbus::ReadRegisters(id_offset, 2, did_and_length);
            filepath = SunSpecModbus::FormatModelPath (did_and_length[0]);
        }

    } else {
        std::string filepath = SunSpecModbus::FormatModelPath (did);
        std::shared_ptr <SunSpecModel> model (
            new SunSpecModel (did, id_offset, filepath)
        );
        models_.push_back (std::move (model));
    }

}

// Read Registers
// - the register array is passed to the function as a pointer so the
// - modbus method call can operate on them.
void SunSpecModbus::ReadRegisters (unsigned int offset,
                                   unsigned int length,
                                   uint16_t *reg_ptr) {
    int status = modbus_read_registers (context_ptr_,
                                        offset,
                                        length,
                                        reg_ptr);
    if (status == -1) {
        std::cout << "[ERROR]\t" 
            << "Read Registers: " << modbus_strerror(errno) << '\n';
        *reg_ptr = 0;
    }
}

// Write Registers
// - the registers to write are passed by reference to reduce memory
void SunSpecModbus::WriteRegisters (unsigned int offset,
                                    unsigned int length,
                                    const uint16_t *reg_ptr) {
    int status = modbus_write_registers (context_ptr_,
                                         offset,
                                         length,
                                         reg_ptr);
    if (status == -1) {
        std::cout << "[ERROR]\t" 
            << "Write Registers: " << modbus_strerror(errno) << '\n';
    }
}

std::map <std::string, std::string> SunSpecModbus::ReadBlock (unsigned int did){
    for (const auto model : models_) {
        if (*model == did) {
            // read register block
            unsigned int offset = model->GetOffset ();
            unsigned int length = model->GetLength ();
            uint16_t raw[length];
            SunSpecModbus::ReadRegisters (offset, length, raw);

            // convert block to vector
            std::vector <uint16_t> block (raw, raw + length);
            return model->BlockToPoints (block);
        }
    }
}

void SunSpecModbus::WriteBlock (unsigned int did) {
}

std::string SunSpecModbus::FormatModelPath (unsigned int did) {
    // create filename using a base path, then pad the did number and append
    // to the base path. The sunspec models are provided as xml so that will be
    // the file type that is appended ot the end of the filename.
    std::stringstream ss;
    ss << model_path_ << "smdx_";
    ss << std::setfill ('0') << std::setw(5) << did;
    ss << ".xml";
    return ss.str();
}
