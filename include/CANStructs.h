#ifndef CANSTRUCTS_H
#define CANSTRUCTS_H

#include <mbed.h>

// define any shared structs to be sent over CAN here

//********** INTERNAL CAN MESSAGES **********

// Main Node -> Cell Node message
// Balancing outputs, 1 bit per each ID
// Note: Bit correspondences are only for Main Node's Message 1 (IDs 0-63)
// For bit correspondences for Main Node's Message 2, add 64 to the ID (IDs 64-127)
typedef struct Balancing {
    uint32_t ID_63_downto_32;   // Bit 31 (left-most bit) corresponds to ID 63,
                                // Bit 0 (right-most bit) corresponds to ID 32
    uint32_t ID_31_downto_0;    // Bit 31 (left-most bit) corresponds to ID 31,
                                // Bit 0 (right-most bit) corresponds to ID 0 
                                // Note: ID 0 (main node) is not used, always set to 0
} Balancing;

// Cell Node -> Main Node message
// Cell data (analog inputs)
typedef struct CellData {
    uint16_t CellVoltage;   // 0V to 6.5535V, units of 0.0001V
    int8_t CellTemperature; // -40degC to +80degC, units of 1degC
                            // offset of 20degC so
                            // if value sent on CAN = -60degC,
                            // actual cell temperature = -40degC
} CellData;

//********** END INTERNAL CAN MESSAGES **********

#endif // CANSTRUCTS_H