#ifndef SUNSPECMODEL_H
#define SUNSPECMODEL_H

#include <string>
#include <vector>
#include <map>

// BOOST Libs
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

// SunSpec Model
// - This model uses boosts property tree to parse the xml models for
// - modbus register blocks.
// - https://www.technical-recipes.com/2014/using-boostproperty_tree/
class SunSpecModel {
public:
    SunSpecModel (unsigned int did, unsigned int offset);
    virtual ~SunSpecModel ();

public:
    std::map <std::string, std::string> BlockToPoints (
        const std::vector <uint16_t>& register_block
    );

    std::vector <uint16_t> PointsToBlock (
        std::map <std::string, std::string>& points
    );

    // this operator will be used when looking for specific models
    bool operator == (const unsigned int& did) {
        return did_ == did;
    };

public:
    // utility methods
    void GetScalers ();

    float BlockToScaler (
        const std::vector <uint16_t>& register_block,
        std::string scaler
    );

    float PointToScaler (
        std::map <std::string, std::string>& points,
        std::string scaler
    );

    void DescalePoints (std::map <std::string, std::string>* points);

    // converter registers to larger values
    uint32_t GetUINT32 (const std::vector <uint16_t>& block,
                        const unsigned int index);
    uint64_t GetUINT64 (const std::vector <uint16_t>& block,
                        const unsigned int index);
    std::string GetString (const std::vector <uint16_t>& block,
                           const unsigned int index,
                           const unsigned int length);
    void SetUINT32 (std::vector <uint16_t>* block,
                    const unsigned int index,
                    const uint32_t value);
    void SetUINT64 (std::vector <uint16_t>* block,
                    const unsigned int index,
                    const uint64_t value);

public:
    unsigned int offset_;
    unsigned int length_;
    unsigned int did_;
    boost::property_tree::ptree smdx_;
    std::map <std::string, int16_t> scalers_;
};

#endif // SUNSPECMODEL_H
