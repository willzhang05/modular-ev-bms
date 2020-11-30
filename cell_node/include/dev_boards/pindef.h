#ifndef PINDEF_H
#define PINDEF_H

/***************************/
/* pin definitions for CAN */
/***************************/
#define CAN_RX      PD_0    // Rx for MCU
#define CAN_TX      PD_1
#define CAN2_RX     PB_5    // Rx for MCU
#define CAN2_TX     PB_6

/*************************************/
/* pin definitions for Analog Inputs */
/*************************************/
#define CELL_VOLTAGE        PA_0
#define TEMPERATURE_DATA    PA_1

/***************************************/
/* pin definitions for Digital Outputs */
/***************************************/
#define BALANCING_CONTROL   PA_4

/**********************************/
/* pin definitions for Unused I/O */
/**********************************/
#define UNUSED_PIN_0    PA_5
#define UNUSED_PIN_1    PA_6
#define UNUSED_PIN_2    PA_7
#define UNUSED_PIN_3    PB_1

#endif // PINDEF_H
