#include <mbed.h>
#include "pindef.h"
#include "CANStructs.h"

// #define TESTING // only defined when using test functions
#define PRINTING // only defined when using printf functions

#ifdef PRINTING
#include "Printing.h"
#endif

// This uses a lot of ROM!!!
// BufferedSerial device(USBTX, USBRX, 38400);

DigitalOut* balance_out;

int temperature_length = 8;
float t_voltage_output [8] = {1.299, 1.034, 0.925, 0.871, 0.816, 0.760, 0.476, 0.183};
float temperature [8] = {-50, 0, 20, 30, 40, 50, 100, 150};


// DigitalOut* led2;

CAN* can1;

Ticker canTxTicker;


AnalogIn cell_volt(CELL_VOLTAGE);
AnalogIn cell_temp(TEMPERATURE_DATA);

uint16_t current_cell_volt; // 0V to 5V, units of 0.0001V
int8_t current_cell_temp;  // -40degC to +80degC, units of 1degC

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
    printf("start test_cell_voltage()\r\n");
    float v = cell_volt.read();
    int v_int = (int)v;
    int v_dec = (int)(v*100);
    printf("Cell Voltage: %d.%d\r\n", v_int, v_dec);

    // printf("TEST PRINT after analog in\r\n");

    if((v>=test_min) && (v<=test_max)){
        printf("Cell Voltage Test PASSED \r\n");
        return true;
    }
    else
    {
        printf("Cell Voltage Test FAILED \r\n");
        return false; 
    }
}

bool test_balance_output(){
    balance_out->write(0); // switch balancing off
    printf("Balance Output set to Low, measure current and then press any key to continue... \n\r");
    // char c;
    // device.read(&c, 1);
    thread_sleep_for(2000);
    balance_out->write(1); // switch balancing on
    printf("Balance Output set to High, measure current. Did test pass (y/n)? \n\r");
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
    printf("%d.%d\n\r", t_int, t_dec);
    if(t>=test_min && t<=test_max){
        printf("Pack Current Test PASSED \n\r");
        return true;
    }
    else
    {
        printf("Pack Current Test FAILED \n\r");
        return false; 
    }
}

void test_sleep()
{
    // char c;
    printf("Board not sleeping right now, press any key to go to sleep...  \n\r");
    // device.read(&c, 1);
    sleep();

}
#endif

//********** CAN **********
// WARNING: This method is NOT safe to call in an ISR context (if RTOS is enabled)
// This method is Thread safe (CAN is Thread safe)
bool sendCANMessage(const char *data, const unsigned char len = 8) {
    if (len > 8 || !can1->write(CANMessage(2, data, len))) {
        return false;
    }
    return true;
}

// WARNING: This method will be called in an ISR context
void canTxIrqHandler() {
    CellData toSend;
    toSend.CellVolt = current_cell_volt;
    toSend.CellTemp = current_cell_temp;
    if (sendCANMessage((char*)&toSend, sizeof(toSend))) {
#ifdef PRINTING
        printf("Message sent!\n"); // This should be removed except for testing CAN
#endif //PRINTING
    }
}

// WARNING: This method will be called in an ISR context
void canRxIrqHandler() {
    CANMessage receivedCANMessage;
    while (can1->read(receivedCANMessage)) {
// #ifdef PRINTING
        printf("Message received: %s\n", receivedCANMessage.data); // This should be changed to copying the CAN data to a global variable, except for testing CAN
// #endif //PRINTING
    }
}

void canInit() {
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
#ifdef PRINTING
    printf("Direct Cell Temperature: ");
    printFloat(t_direct, 4);
    printf("\r\n");
    printf("Cell Temperature: %d degrees C\r\n", the_cell_temp);
#endif
    return the_cell_temp;
}

uint16_t get_cell_voltage() {
    float v_direct = cell_volt.read();
    float v = v_direct * CELL_VOLT_MULT;
    uint16_t the_cell_volt = (uint16_t)(v*10000);
#ifdef PRINTING
    printf("Direct Cell Voltage: ");
    printFloat(v_direct, 4);
    printf("\r\n");
    printf("Cell Voltage: ");
    printIntegerAsFloat(the_cell_volt, 4);
    printf(" V\r\n");
#endif
    return the_cell_volt;
}
//********** End Cell Measurements **********

int main() {
#ifdef STM32F042x6
    __HAL_REMAP_PIN_ENABLE(HAL_REMAP_PA11_PA12);
#endif

    CAN theRealCan1(CAN_RX, CAN_TX);
    can1 = &theRealCan1;
    canInit();

    DigitalOut theRealBalanceOut(BALANCING_CONTROL);
    balance_out = &theRealBalanceOut;

    DigitalOut led2(LED2);

#ifdef PRINTING
    printf("start of main()\r\n");
#endif

    while(1) {
        // do nothing
        led2 = led2 ^ 1;
#ifdef PRINTING
        printf("Hello! \r\n");
#endif
#ifdef TESTING
        test_cell_voltage(0,1);
#endif
        current_cell_volt = get_cell_voltage();
        current_cell_temp = get_cell_temperature();
        thread_sleep_for(1000);
#ifdef PRINTING
        printf("\r\n");
#endif
    }
}
