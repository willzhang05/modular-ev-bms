#include <mbed.h>
#include "pindef.h"
#include "CANStructs.h"
#include "Printing.h"

// #define TESTING // only defined when using test functions

// This uses a lot of ROM!!!
// BufferedSerial device(USBTX, USBRX, 38400);

DigitalOut* balance_out;

// int temperature_length = 8;
// float t_voltage_output [8] = {1.299, 1.034, 0.925, 0.871, 0.816, 0.760, 0.476, 0.183};
// float temperature [8] = {-50, 0, 20, 30, 40, 50, 100, 150};


// DigitalOut* led2;

CAN* can1;

Ticker canTxTicker;


AnalogIn cell_volt(CELL_VOLTAGE);
AnalogIn cell_temp(TEMPERATURE_DATA);

CellData cellData;  // stores cell volt and cell temp

// multiplier from AnalogIn reading [0, 1] to Cell Voltage (V) [0, 5]
#define CELL_VOLT_MULT  (6.75f)     // experimental value
// multiplier from AnalogIn reading [0, 1] to voltage (mV) used for Cell Temperature formula [0, 3300]
#define CELL_TEMP_MULT  (3800.0f)   // experimental value for AnalogIn reading 0.23 = 25 C Temperature
// Formula taken from LMT84 datasheet, section 8.3
// (T1, V1) are the minimum temperature's coordinates
// (T2, V2) are the maximum temperature's coordinates
// Given formula is: V - V1 = (V2 - V1) / (T2 - T1) * (T - T1)
// Rewritten formula is: T = (V - V1) * (T2 - T1) / (V2 - V1) + T1
#define T1      (-40.0f)
#define V1      (1247.0f)
#define T2      (80.0f)
#define V2      (591.0f)
#define T(V)    ((V - V1) * (T2 - T1) / (V2 - V1) + T1)

#ifdef TESTING
bool test_cell_voltage(uint16_t test_min, uint16_t test_max){
    PRINT("start test_cell_voltage()\r\n");
    float v = cell_volt.read();
    int v_int = (int)v;
    int v_dec = (int)(v*100);
    PRINT("Cell Voltage: %d.%d\r\n", v_int, v_dec);

    // PRINT("TEST PRINT after analog in\r\n");

    if((v>=test_min) && (v<=test_max)){
        PRINT("Cell Voltage Test PASSED \r\n");
        return true;
    }
    else
    {
        PRINT("Cell Voltage Test FAILED \r\n");
        return false; 
    }
}

bool test_balance_output(){
    balance_out->write(0); // switch balancing off
    PRINT("Balance Output set to Low, measure current and then press any key to continue... \n\r");
    // char c;
    // device.read(&c, 1);
    thread_sleep_for(2000);
    balance_out->write(1); // switch balancing on
    PRINT("Balance Output set to High, measure current. Did test pass (y/n)? \n\r");
    // device.read(&c, 1);
    thread_sleep_for(2000);
    balance_out->write(0); // switch balancing off
    // if(c=='y'){
    //     return true;
    // }

    return false;
}

bool test_cell_temperature(float test_min, float test_max){
    float t = cell_temp.read();
    int t_int = (int)t;
    int t_dec = (int)(t*100);
    PRINT("%d.%d\n\r", t_int, t_dec);
    if(t>=test_min && t<=test_max){
        PRINT("Pack Current Test PASSED \n\r");
        return true;
    }
    else
    {
        PRINT("Pack Current Test FAILED \n\r");
        return false; 
    }
}

void test_sleep()
{
    // char c;
    PRINT("Board not sleeping right now, press any key to go to sleep...  \n\r");
    // device.read(&c, 1);
    sleep();

}
#endif //TESTING

//********** CAN **********
#define DEVICE_ID           2   // hard-coded ID for each cell node
#define CELL_DATA_PRIORITY  2

// WARNING: This method is NOT safe to call in an ISR context (if RTOS is enabled)
// This method is Thread safe (CAN is Thread safe)
bool sendCANMessage(int messageID, const char *data, const unsigned char len = 8) {
    if (len > 8 || !can1->write(CANMessage(messageID, data, len))) {
        return false;
    }
    return true;
}

// WARNING: This method will be called in an ISR context
void canTxIrqHandler() {
    CellData toSend = cellData;
    if (sendCANMessage(GET_CAN_MESSAGE_ID(DEVICE_ID, CELL_DATA_PRIORITY), (char*)&toSend, sizeof(toSend))) {
        PRINT("Message sent!\n");
    }
}

// WARNING: This method will be called in an ISR context
void canRxIrqHandler() {
    CANMessage receivedCANMessage;
    while (can1->read(receivedCANMessage)) {
        uint32_t messageID = receivedCANMessage.id;
        PRINT("Message received: %s\n", receivedCANMessage.data);
    }
}

void canInit() {
    CAN theRealCan1(CAN_RX, CAN_TX);
    can1 = &theRealCan1;

    canTxTicker.attach(&canTxIrqHandler, 1s); // float, in seconds
    can1->attach(&canRxIrqHandler, CAN::RxIrq);
}
//********** End CAN **********


//********** Cell Measurements **********
// float get_cell_temperature(){
//     float t_voltage = cell_temp.read()*VDD;
//     for(int i =0;i<temperature_length-1;i++){ //Voltage calibration
//         if(t_voltage>=t_voltage_output[i] && t_voltage<=t_voltage_output[i+1]){
//             return ((t_voltage-t_voltage_output[i])*(temperature[i+1]-temperature[i])/(t_voltage_output[i+1]-t_voltage_output[i]))+temperature[i]; //SOC based on voltage calibration
//         }
//     }
//     return -100.0;
// }
// float get_cell_voltage(){
//     float v = cell_volt.read()*VDD*(18+33)/33;
//     return v;
// }

int16_t get_cell_temperature() {
    float t_direct = cell_temp.read();
    float t = T(t_direct * CELL_TEMP_MULT);
    int8_t the_cell_temp = (int8_t)(t);

    PRINT("Direct Cell Temperature: ");
    printFloat(t_direct, 4);
    PRINT("\r\n");
    PRINT("Cell Temperature: %d degrees C\r\n", the_cell_temp);

    return the_cell_temp;
}

uint16_t get_cell_voltage() {
    float v_direct = cell_volt.read();
    float v = v_direct * CELL_VOLT_MULT;
    uint16_t the_cell_volt = (uint16_t)(v*10000);

    PRINT("Direct Cell Voltage: ");
    printFloat(v_direct, 4);
    PRINT("\r\n");
    PRINT("Cell Voltage: ");
    printIntegerAsFloat(the_cell_volt, 4);
    PRINT(" V\r\n");

    return the_cell_volt;
}
//********** End Cell Measurements **********

int main() {
#ifdef STM32F042x6
    __HAL_REMAP_PIN_ENABLE(HAL_REMAP_PA11_PA12);
#endif //STM32F042x6
    canInit();

    DigitalOut theRealBalanceOut(BALANCING_CONTROL);
    balance_out = &theRealBalanceOut;

    DigitalOut led2(LED2);

    PRINT("start of main()\r\n");

    while(1) {
        // do nothing
        led2 = led2 ^ 1;

        PRINT("Hello! \r\n");

#ifdef TESTING
        test_cell_voltage(0,1);
#endif //TESTING
        cellData.CellVolt = get_cell_voltage();
        cellData.CellTemp = get_cell_temperature();
        thread_sleep_for(1000);

        PRINT("\r\n");
    }
}
