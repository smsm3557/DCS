#include <iostream>
#include <cmath>
#include <cctype>  // isdigit
#include <bitset>

#include "include/SunSpecModel.h"

namespace pt = boost::property_tree;

SunSpecModel::SunSpecModel (unsigned int did, unsigned int offset)
    : offset_(offset) {
    // create filename using a base path, then pad the did number and append
    // to the base path. The sunspec models are provided as xml so that will be
    // the file type that is appended ot the end of the filename.
    std::stringstream ss;
    ss << "/home/tylor/dev/Workspace/SunSpec/data/models/smdx/smdx_";
    ss << std::setfill ('0') << std::setw(5) << did;
    ss << ".xml";
    std::string filename = ss.str();
    std::cout << filename << std::endl;

    // Use boosts xml parser to read file and store as member variable.
    pt::xml_parser::read_xml(filename, smdx_);
    did_ = smdx_.get <unsigned int> ("sunSpecModels.model.<xmlattr>.id", 0);
    std::string name;
    name = smdx_.get <std::string> ("sunSpecModels.model.<xmlattr>.name", "");
    length_ = smdx_.get <unsigned int> ("sunSpecModels.model.<xmlattr>.len", 0);

    std::cout << "SunSpec Model Found"
        << "\n\tDID: " << did_
        << "\n\tName: " << name
        << "\n\tLength: " << length_ << std::endl;

    SunSpecModel::GetScalers ();
}

SunSpecModel::~SunSpecModel() {
};


// Block To Points
// - convert raw modbus register block to it's corresponding SunSpec points
std::map <std::string, std::string> SunSpecModel::BlockToPoints (
    const std::vector <uint16_t>& register_block) {
    std::map <std::string, std::string> point_map;
    std::string id, type, scaler;
    unsigned int offset;

     // Traverse property tree
    BOOST_FOREACH (pt::ptree::value_type const& node,
                   smdx_.get_child ("sunSpecModels.model.block")) {
        pt::ptree subtree = node.second;
        if( node.first == "point" ) {
            id = subtree.get <std::string> ("<xmlattr>.id", "");
            type = subtree.get <std::string> ("<xmlattr>.type", "");
            scaler = subtree.get <std::string> ("<xmlattr>.sf", "default");
            offset = subtree.get <unsigned int> ("<xmlattr>.offset", 0);

            // TODO (TS): this should be configured by the smdx file
            // - the scaled values are a decimal place shift so I use the
            // - pow() function to raise 10 to the scale value for scaling.
            if (type == "int16") {
                float value = register_block[offset];
                float scale = SunSpecModel::BlockToScaler(register_block,
                                                          scaler);
                value = value * scale;
                point_map[id] = std::to_string(value);
            } else if (type == "uint16") {
                float value = register_block[offset];
                float scale = SunSpecModel::BlockToScaler(register_block,
                                                          scaler);
                value = value * scale;
                point_map[id] = std::to_string(value);
            } else if (type == "count") {
                float value = register_block[offset];
                float scale = SunSpecModel::BlockToScaler(register_block,
                                                          scaler);
                value = value * scale;
                point_map[id] = std::to_string(value);
            } else if (type == "acc16") {
                float value = register_block[offset];
                float scale = SunSpecModel::BlockToScaler(register_block,
                                                          scaler);
                value = value * scale;
                point_map[id] = std::to_string(value);
            } else if (type == "int32") {
                float value = SunSpecModel::GetUINT32(register_block,offset);
                float scale = SunSpecModel::BlockToScaler(register_block,
                                                          scaler);
                value = value * scale;
                point_map[id] = std::to_string(value);
            } else if (type == "float32") {
                float value = SunSpecModel::GetUINT32(register_block,offset);
                float scale = SunSpecModel::BlockToScaler(register_block,
                                                          scaler);
                value = value * scale;
                point_map[id] = std::to_string(value);
            } else if (type == "acc32") {
                float value = SunSpecModel::GetUINT32(register_block,offset);
                float scale = SunSpecModel::BlockToScaler(register_block,
                                                          scaler);
                value = value * scale;
                point_map[id] = std::to_string(value);
            } else if (type == "enum16") {
                BOOST_FOREACH (pt::ptree::value_type const& subsubtree,
                               subtree) {
                    pt::ptree symbol = subsubtree.second;
                    std::string reg = std::to_string(register_block[offset]);
                    if( subsubtree.first == "symbol" && symbol.data() == reg) {
                        std::string sym;
                        sym = symbol.get <std::string> ("<xmlattr>.id", "");
                        point_map[id] = sym;
                    }
                }
            } else if (type == "enum32") {
                BOOST_FOREACH (pt::ptree::value_type const& subsubtree,
                               subtree) {
                    pt::ptree symbol = subsubtree.second;
                    std::string reg = std::to_string(
                        SunSpecModel::GetUINT32(register_block,offset)
                    );
                    if( subsubtree.first == "symbol" && symbol.data() == reg) {
                        std::string sym;
                        sym = symbol.get <std::string> ("<xmlattr>.id", "");
                        point_map[id] = sym;
                    }
                }
            } else if (type == "bitfield16") {
                std::vector <std::string> symbols;
                std::string sym;

                // collect each bits symbol value
                BOOST_FOREACH (pt::ptree::value_type const& subsubtree,
                               subtree) {
                    pt::ptree symbol = subsubtree.second;
                    if( subsubtree.first == "symbol") {
                        sym = symbol.get <std::string> ("<xmlattr>.id", "");
                        symbols.push_back(sym);
                    }
                }

                if (!symbols.empty()) {
                    sym.clear();

                    // for each bit add symbol if it is set;
                    std::bitset<16> bits (register_block[offset]);
                    for (unsigned int i = 0; i < symbols.size(); i++) {
                        if (bits[i]) {
                            sym = sym + symbols[i] + ",";
                        }
                    }
                    sym.pop_back ();  // remove last comma
                    point_map[id] = sym;
                }
            } else if (type == "bitfield32") {
                std::vector <std::string> symbols;
                std::string sym;

                // collect each bits symbol value
                BOOST_FOREACH (pt::ptree::value_type const& subsubtree,
                               subtree) {
                    pt::ptree symbol = subsubtree.second;
                    if( subsubtree.first == "symbol") {
                        sym = symbol.get <std::string> ("<xmlattr>.id", "");
                        symbols.push_back(sym);
                    }
                }
                if (!symbols.empty()) {
                    sym.clear();

                    // for each bit add symbol if it is set;
                    std::bitset<32> bits (
                        SunSpecModel::GetUINT32(register_block, offset)
                    );

                    for (unsigned int i = 0; i < symbols.size(); i++) {
                        if (bits[i]) {
                            sym = sym + symbols[i] + ",";
                        }
                    }
                    sym.pop_back ();  // remove last comma
                    point_map[id] = sym;
                }
            } else if (type == "sunssf") {
                int16_t value = register_block[offset];
                point_map[id] = std::to_string(value);
            } else if (type == "string") {
                unsigned int length;
                length = subtree.get <unsigned int> ("<xmlattr>.len", 0);
                std::string value = SunSpecModel::GetString(
                    register_block,offset, length
                );
                point_map[id] = value;
            } else if (type == "pad") {
                //TODO (TS): determine how this value is implemented.
            } else if (type == "ipaddr") {
                //TODO (TS): determine how this value is implemented.
            } else if (type == "ipv6addr") {
                //TODO (TS): determine how this value is implemented.
            } else if (type == "eui48") {
                //TODO (TS): determine how this value is implemented.
            }
        }
    }

    return point_map;
};


// Points To Block
// - translated sunspec points into register block for writing to device
std::vector <uint16_t> SunSpecModel::PointsToBlock (
    std::map <std::string, std::string>& points) {
    std::vector <uint16_t> register_block (length_, 0);  // initialize block
    std::string id, type, scaler;
    unsigned int offset;

     // Traverse property tree
    BOOST_FOREACH (pt::ptree::value_type const& node,
                   smdx_.get_child ("sunSpecModels.model.block")) {
        pt::ptree subtree = node.second;
        if( node.first == "point" ) {
            id = subtree.get <std::string> ("<xmlattr>.id", "");
            type = subtree.get <std::string> ("<xmlattr>.type", "");
            scaler = subtree.get <std::string> ("<xmlattr>.sf", "default");
            offset = subtree.get <unsigned int> ("<xmlattr>.offset", 0);

            // TODO (TS): this should be configured by the smdx file
            if (type == "int16") {
                float value = std::stof(points[id]);
                float scale = SunSpecModel::BlockToScaler(register_block,
                                                          scaler);
                register_block[offset] = value / scale;
            } else if (type == "uint16") {
                float value = std::stof(points[id]);
                float scale = SunSpecModel::BlockToScaler(register_block,
                                                          scaler);
                register_block[offset] = value / scale;
            } else if (type == "count") {
                float value = std::stof(points[id]);
                float scale = SunSpecModel::BlockToScaler(register_block,
                                                          scaler);
                register_block[offset] = value / scale;
            } else if (type == "acc16") {
                float value = std::stof(points[id]);
                float scale = SunSpecModel::BlockToScaler(register_block,
                                                          scaler);
                register_block[offset] = value / scale;
            } else if (type == "int32") {
                float value = std::stof(points[id]);
                float scale = SunSpecModel::BlockToScaler(register_block,
                                                          scaler);
                value = value / scale;
                SunSpecModel::SetUINT32(&register_block,offset, value);
            } else if (type == "float32") {
                float value = std::stof(points[id]);
                float scale = SunSpecModel::BlockToScaler(register_block,
                                                          scaler);
                value = value / scale;
                SunSpecModel::SetUINT32(&register_block,offset, value);
            } else if (type == "acc32") {
                float value = std::stof(points[id]);
                float scale = SunSpecModel::BlockToScaler(register_block,
                                                          scaler);
                value = value / scale;
                SunSpecModel::SetUINT32(&register_block,offset, value);
            } else if (type == "enum16") {
                //TODO (TS): I don't believe these values can be written to
            } else if (type == "enum32") {
                //TODO (TS): I don't believe these values can be written to
            } else if (type == "bitfield16") {
                //TODO (TS): I don't believe these values can be written to
            } else if (type == "bitfield32") {
                //TODO (TS): I don't believe these values can be written to
            } else if (type == "string") {
                //TODO (TS): I don't believe these values can be written to
            } else if (type == "pad") {
                //TODO (TS): I don't believe these values can be written to
            } else if (type == "ipaddr") {
                //TODO (TS): determine how this value is implemented.
            } else if (type == "ipv6addr") {
                //TODO (TS): determine how this value is implemented.
            } else if (type == "eui48") {
                //TODO (TS): determine how this value is implemented.
            }
        }
    }

    return register_block;
};


void SunSpecModel::GetScalers() {
    scalers_["default"] = 0;
     // Traverse property tree
    BOOST_FOREACH (pt::ptree::value_type const& node,
                   smdx_.get_child ("sunSpecModels.model.block")) {
        pt::ptree subtree = node.second;
        std::string id;
        unsigned int offset;
        if( node.first == "point" ) {

            if (subtree.get <std::string> ("<xmlattr>.type") == "sunssf") {
                id = subtree.get <std::string> ("<xmlattr>.id");
                offset = subtree.get <int> ("<xmlattr>.offset");
                scalers_[id] = offset;
            }
        }
    }
};

// Block To Scaler
// - this function is just a hack to get non-sunspec complient devices to be
// - interpreted.
float SunSpecModel::BlockToScaler (const std::vector <uint16_t>& register_block,
                                   std::string scaler) {
    if (std::isdigit (*scaler.c_str())) {
        return std::stof(scaler);
    } else {
        return std::pow(10, register_block[scalers_[scaler]]);
    }
};

// Point To Scaler
// - this function is just a hack to get non-sunspec complient devices to be
// - interpreted.
float SunSpecModel::PointToScaler (std::map <std::string, std::string>& points,
                                   std::string scaler) {
    if (std::isdigit (*scaler.c_str())) {
        return std::stof(scaler);
    } else {
        return std::stof(points[scaler]);
    }
};


void SunSpecModel::DescalePoints (std::map <std::string, std::string>* points) {

};


uint32_t SunSpecModel::GetUINT32 (const std::vector <uint16_t>& block,
                                  const unsigned int index) {
    // bitshift the first register 16 bits and then append second register
    // this process is dependant on modbus
    return (block[index] << 16) | block[index];
};


uint64_t SunSpecModel::GetUINT64 (const std::vector <uint16_t>& block,
                                  const unsigned int index) {
    uint64_t value = static_cast<uint64_t>(block[index]) << 48
                     | static_cast<uint64_t>(block[index+1]) << 32
                     | static_cast<uint64_t>(block[index+2]) << 16
                     | block[index+3];
    return value;
};


std::string SunSpecModel::GetString (const std::vector <uint16_t>& block,
                                     const unsigned int index,
                                     const unsigned int length) {
    std::stringstream ss;
    for (unsigned int i = index; i < length; i++) {
        ss << static_cast <char> (block[i] >> 8);
        ss << static_cast <char> (block[i]);
    }
    return ss.str();
};


void SunSpecModel::SetUINT32 (std::vector <uint16_t>* block,
                              const unsigned int index,
                              const uint32_t value) {
    std::vector <uint16_t>& deref_block = *block;
    deref_block[index] = static_cast <uint16_t> (value >> 16);
    deref_block[index+1] = static_cast <uint16_t> (value);
};


void SunSpecModel::SetUINT64 (std::vector <uint16_t>* block,
                              const unsigned int index,
                              const uint64_t value) {
    std::vector <uint16_t>& deref_block = *block;
    deref_block[index] = static_cast <uint16_t> (value >> 48);
    deref_block[index+1] = static_cast <uint16_t> (value >> 32);
    deref_block[index+2] = static_cast <uint16_t> (value >> 16);
    deref_block[index+3] = static_cast <uint16_t> (value);
};

