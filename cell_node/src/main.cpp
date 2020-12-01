#include <mbed.h>
#include "pindef.h"

// #define TESTING // only defined when using test functions

// This uses a lot of ROM!!!
// BufferedSerial device(USBTX, USBRX, 38400);

AnalogIn cell_volt(CELL_VOLTAGE);
AnalogIn cell_temp(TEMPERATURE_DATA);
DigitalOut* balance_out;

// DigitalOut* led2;

CAN* can1;

uint16_t current_cell_volt;
int16_t current_cell_temp;

// multiplier from AnalogIn reading [0, 1] to Cell Voltage [0, 5]
#define CELL_VOLT_MULT  (5.0f)
// multiplier from AnalogIn reading [0, 1] to voltage used for Cell Temperature formula [0, 3.3]
#define CELL_TEMP_MULT  (3.3f)
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

int main() {
#ifdef STM32F042x6
    __HAL_REMAP_PIN_ENABLE(HAL_REMAP_PA11_PA12);
#endif

    CAN theRealCan1(CAN_RX, CAN_TX);
    can1 = &theRealCan1;

    DigitalOut theRealBalanceOut(BALANCING_CONTROL);
    balance_out = &theRealBalanceOut;

    DigitalOut led2(LED2);


    printf("start of main()\r\n");



    while(1)
    {
        // do nothing
        led2 = led2 ^ 1;
        printf("Hello! \r\n");
#ifdef TESTING
        test_cell_voltage(0,1);
#endif
        float v = cell_volt.read() * CELL_VOLT_MULT;
        current_cell_volt = (uint16_t)(v*100);
        printf("Cell Voltage: %d\r\n", current_cell_volt);

        float t = T(cell_temp.read() * CELL_TEMP_MULT);
        current_cell_temp = (int16_t)(t*100);
        printf("Cell Temperature: %d\r\n", current_cell_temp);

        thread_sleep_for(1000);
        printf("\r\n");
    }
}
