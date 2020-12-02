#include <mbed.h>
#include "pindef.h"

// #define TESTING // only defined when using test functions
#define PRINTING // only defined when using printf functions

// This uses a lot of ROM!!!
// BufferedSerial device(USBTX, USBRX, 38400);

AnalogIn cell_volt(CELL_VOLTAGE);
AnalogIn cell_temp(TEMPERATURE_DATA);
DigitalOut* balance_out;
int charge_estimation_state = 0; //0: Initial State, 1: Transitional State, 2: Charge State, 3: Discharge State, 4: Equilibrium, 5: Fully Charged, 6: Fully Discharged
float SOC = 100.0;
float SOH = 100.0;
float SOC_error = 0.0;
float SOH_error = 0.0;
float DOD = 0.0;

float min_current_thresh = 0.001; // if current is < min_current_thresh and current > - min_current_thresh, current is essentially 0
float min_current_charging_thresh = 0.1;
float min_voltage_diff = 0.001;
float last_voltage = 0.0;

int constant_voltage_count = 0;
int constant_voltage_count_thresh = 5;

float rated_capacity = 12060; //3350 mAh in Coulombs
float dt = 0.1; //seconds

float max_voltage = 4.2;
float min_voltage = 2.5;

float VDD = 3.3;

int callibrate_length = 7;
float voltage_callibrate [7] = {0, 2.5, 2.8, 3.3, 3.6, 4.2, 10};
float SOC_callibrate [7] = {0, 0, 20, 80, 90, 100, 100};

int temperature_length = 8;
float t_voltage_output [8] = {1.299, 1.034, 0.925, 0.871, 0.816, 0.760, 0.476, 0.183};
float temperature [8] = {-50, 0, 20, 30, 40, 50, 100, 150};

void init_battery_estimation(float voltage){
    
}
void charge_estimation(float current, float voltage){ //TODO: Check does positive current mean charging or discharging
    if(charge_estimation_state==0){ //Initial State
        if(current>min_current_thresh){
            charge_estimation_state = 3;
        }
        else if(current<-min_current_thresh){
            charge_estimation_state = 2;
        }
        else{
            charge_estimation_state = 1;
        }
    }
    else if(charge_estimation_state==1){ //Transitional State
        if(current>min_current_thresh){
            charge_estimation_state = 3;
        }
        else if(current<-min_current_thresh){
            charge_estimation_state = 2;
        }
        else{
            float voltage_diff = voltage-last_voltage;
            if(voltage_diff<=min_voltage_diff && voltage_diff>=-min_voltage_diff)
            {
                constant_voltage_count++;
            }
            else{
                constant_voltage_count = 0;
            }
            if(constant_voltage_count>=constant_voltage_count_thresh){
                charge_estimation_state = 4;
            }
        }
    }
    else if(charge_estimation_state==2){ //Charge State , assumes negative current
        if(current>min_current_thresh){
            charge_estimation_state = 3;
        }
        else if(current>-min_current_thresh){ // Current is essentially 0
            charge_estimation_state = 1;
        }
        else if(current>-min_current_charging_thresh || voltage>=max_voltage){ //Current is low or voltage is high, so charging is done
            charge_estimation_state = 5;
        }
        else{ // calculate DOD and SOC
            DOD = DOD + (current*dt)/rated_capacity;
            SOC = SOH - DOD;
        }
    }
    else if(charge_estimation_state==3){ //Discharge State , assumes positive current
        if(current<-min_current_thresh){
            charge_estimation_state = 2;
        }
        else if(current<min_current_thresh){ // Current is essentially 0
            charge_estimation_state = 1;
        }
        else if(voltage<=min_voltage){ //Finished Discharging
            charge_estimation_state = 6;
        }
        else{
            DOD = DOD + (current*dt)/rated_capacity;
            SOC = SOH - DOD;
        }
    }
    else if(charge_estimation_state==4){ //Equilibrium, check voltage
        if(current>min_current_thresh){
            charge_estimation_state = 3;
        }
        else if(current<-min_current_thresh){
            charge_estimation_state = 2;
        }
        else{
            for(int i =0;i<callibrate_length-1;i++){ //Voltage calibration
                if(voltage>=voltage_callibrate[i] && voltage<=voltage_callibrate[i+1]){
                    float SOC_voltage = ((voltage-voltage_callibrate[i])*(SOC_callibrate[i+1]-SOC_callibrate[i])/(voltage_callibrate[i+1]-voltage_callibrate[i]))+SOC_callibrate[i]; //SOC based on voltage calibration
                    
                    SOC = SOC_voltage;
                    DOD = SOH-SOC;
                }
            }
        }
    }
    else if(charge_estimation_state==5){ //Fully Ccharged
        //TODO: Send signal to turn off Charging contactors
        //TODO: Possibly add voltage calibration
        SOH = SOC;
        if(current>min_current_thresh){
            charge_estimation_state = 3;
        }
        else if(current>-min_current_thresh){
            charge_estimation_state = 1;
        }
    }
    else if(charge_estimation_state==6){ //Fully discharged
    //TODO: Send signal to turn off Disharging contactors
        SOH = DOD;
        if(current<-min_current_thresh){
            charge_estimation_state = 2; //Go to charging state
        }
        else if(current<min_current_thresh){
            charge_estimation_state = 1; //Transition state
        }
    }
    last_voltage = voltage;
}

// DigitalOut* led2;

CAN* can1;

uint16_t current_cell_volt;
int16_t current_cell_temp;

// multiplier from AnalogIn reading [0, 1] to Cell Voltage (V) [0, 5]
#define CELL_VOLT_MULT  (6.75f)     // experimental value
// multiplier from AnalogIn reading [0, 1] to voltage (mV) used for Cell Temperature formula [0, 3300]
#define CELL_TEMP_MULT  (3300.0f)
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

#ifdef PRINTING
// Input: an integer representing a float with 2 digits past decimal multiplied by 100
// Output: print num as a float
void printFloat(int num) {
    float numFloat = (float)(num);
    int right = (int)(numFloat);
    numFloat /= 100;
    int left = (int)(numFloat);
    right -= left * 100;
    printf("%d.%d", left, right);
}
#endif

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
    int16_t the_cell_temp = (int16_t)(t*100);
#ifdef PRINTING
    int temp = (int)(t_direct * 100);
    if(temp >= 100) {
        temp -= 100;
    }
    printf("Direct Cell Temperature: %d.%d\r\n", (int)(t_direct), temp);
    printf("Cell Temperature: ");
    printFloat(the_cell_temp);
    printf(" degrees C\r\n");
#endif
    return the_cell_temp;
}

uint16_t get_cell_voltage() {
    float v_direct = cell_volt.read();
    float v = v_direct * CELL_VOLT_MULT;
    uint16_t the_cell_volt = (uint16_t)(v*100);
#ifdef PRINTING
    int temp = (int)(v_direct * 100);
    if(temp > 100) {
        temp -= 100;
    }
    printf("Direct Cell Voltage: %d.%d\r\n", (int)(v_direct), temp);
    printf("Cell Voltage: ");
    printFloat(the_cell_volt);
    printf(" V\r\n");
#endif
    return the_cell_volt;
}

int main() {
#ifdef STM32F042x6
    __HAL_REMAP_PIN_ENABLE(HAL_REMAP_PA11_PA12);
#endif

    CAN theRealCan1(CAN_RX, CAN_TX);
    can1 = &theRealCan1;

    DigitalOut theRealBalanceOut(BALANCING_CONTROL);
    balance_out = &theRealBalanceOut;

    DigitalOut led2(LED2);

#ifdef PRINTING
    printf("start of main()\r\n");
#endif

    while(1) {
        // do nothing
        led2 = led2 ^ 1;
        printf("Hello! \r\n");
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
