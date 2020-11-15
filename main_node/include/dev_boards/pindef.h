#ifndef PINDEF_H
#define PINDEF_H

/***************************/
/* pin definitions for CAN */
/***************************/
#define CAN1_RX      PD_0   // Rx for MCU
#define CAN1_TX      PD_1
#define CAN2_RX      PB_5   // Rx for MCU
#define CAN2_TX      PB_6

#define PACK_VOLTAGE    PA_0
#define PACK_CURRENT    PA_1

#define FAN_CTRL                    PB_4
#define FAN_PWM                     PB_5
#define CHARGE_CONTACTOR_CTRL       PB_9
#define DISCHARGE_CONTACTOR_CTRL    PB_10
#define UNUSED_PIN_0    PA_2
#endif // PINDEF_H