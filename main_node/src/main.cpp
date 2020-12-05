#include <mbed.h>
#include "pindef.h"
#include "Printing.h"

// #define TESTING     // only defined if using test functions

#define NUM_ADC_SAMPLES 10
#define NUM_CELL_NODES  2
#define NUM_PARALLEL_CELL 3
// #define VDD 3.3f

BufferedSerial device(USBTX, USBRX);

AnalogIn pack_volt(PACK_VOLTAGE);
AnalogIn pack_current(PACK_CURRENT);

DigitalOut fan_ctrl(FAN_CTRL);
PwmOut fan_pwm(FAN_PWM);

DigitalOut charge_contactor(CHARGE_CONTACTOR_CTRL);
DigitalOut discharge_contactor(DISCHARGE_CONTACTOR_CTRL);

CAN intCan(INT_CAN_RX, INT_CAN_TX);
CAN extCan(EXT_CAN_RX, EXT_CAN_TX);

DigitalOut intCanStby(INT_CAN_STBY);
DigitalOut extCanStby(EXT_CAN_STBY);

Ticker intCanTxTicker;
int charge_estimation_state[NUM_CELL_NODES]; //0: Initial State, 1: Transitional State, 2: Charge State, 3: Discharge State, 4: Equilibrium, 5: Fully Charged, 6: Fully Discharged
float SOC[NUM_CELL_NODES];
float SOH[NUM_CELL_NODES];
float DOD[NUM_CELL_NODES];

float min_current_thresh = 0.001; // if current is < min_current_thresh and current > - min_current_thresh, current is essentially 0
float min_current_charging_thresh = 0.1;
float min_voltage_diff = 0.001;
float last_voltage[NUM_CELL_NODES];

int constant_voltage_count = 0;
int constant_voltage_count_thresh = 5;

float rated_capacity = 12060*NUM_PARALLEL_CELL; //3350 mAh in Coulombs
float dt = 0.1; //seconds

float max_voltage = 4.2;
float min_voltage = 2.5;

float VDD = 3.3;

int callibrate_length = 7;
float voltage_callibrate [7] = {0, 2.5, 2.8, 3.3, 3.6, 4.2, 10};
float SOC_callibrate [7] = {0, 0, 20, 80, 90, 100, 100};

float cell_voltages[NUM_CELL_NODES];
float cell_balancing_thresh = 0.3f;
float cell_temperatures [NUM_CELL_NODES];
float temperature_thresh = 30.0f;
float zero_current_ADC = 0.5f;  // the ADC value that represent 0A for the current sensor, calibrated at startup

#ifdef TESTING
DigitalOut test_point_0(UNUSED_PIN_0);
#endif //TESTING

#ifdef TESTING
bool test_pack_voltage(float test_min, float test_max){
    float v = pack_volt.read();
    PRINT("ADC pack voltage: ");
    printFloat(v, 2);
    PRINT("\r\n");
    if(v>=test_min && v<=test_max){
        PRINT("Pack Voltage Test PASSED \n\r");
        return true;
    }
    else
    {
        PRINT("Pack Voltage Test FAILED \n\r");
        return false; 
    }
}

bool test_pack_current(float test_min, float test_max){
    float i = pack_current.read();
    PRINT("ADC pack current: ");
    printFloat(i, 2);
    PRINT("\r\n");
    if(i>=test_min && i<=test_max){
        PRINT("Pack Current Test PASSED \n\r");
        return true;
    }
    else
    {
        PRINT("Pack Current Test FAILED \n\r");
        return false; 
    }
}

bool test_fan_output(){
    char c;
    fan_ctrl.write(0);
    fan_pwm.write(0.0);
    PRINT("Fan ctrl set to Low, press any key to continue...  \n\r");
    device.read(&c, 1);
    fan_ctrl.write(1);
    fan_pwm.write(0.1);
    PRINT("Fan ctrl set to High, PWM set to Low, press any key to continue...  \n\r");
    device.read(&c, 1);
    fan_ctrl.write(1);
    fan_pwm.write(0.9);
    PRINT("Fan ctrl set to High, PWM set to High, Did test pass (y/n)? \n\r");
    device.read(&c, 1);
    fan_ctrl.write(0);
    fan_pwm.write(0.0);
    if(c=='y'){
        return true;
    }

    return false;
}

bool test_discharge_contactor(){
    discharge_contactor.write(0); // switch balancing off
    PRINT("Discharge Contactor Output set to Low, measure current and then press any key to continue... \n\r");
    char c;
    //while(1)
    //{
    device.read(&c, 1);
    discharge_contactor.write(1); // switch balancing on
    PRINT("Discharge Contactor Output set to High, measure current. Did test pass (y/n)? \n\r");
    device.read(&c, 1);
    discharge_contactor.write(0); // switch balancing off
    if(c=='y'){
        return true;
    }

    return false;
}

bool test_charge_contactor(){
    charge_contactor.write(0); // switch balancing off
    PRINT("Charge Contactor Output set to Low, measure current and then press any key to continue... \n\r");
    char c;
    //while(1)
    //{
    device.read(&c, 1);
    charge_contactor.write(1); // switch balancing on
    PRINT("Ccharge Contactor Output set to High, measure current. Did test pass (y/n)? \n\r");
    device.read(&c, 1);
    charge_contactor.write(0); // switch balancing off
    if(c=='y'){
        return true;
    }

    return false;
}

void test_sleep()
{
    char c;
    PRINT("Board not sleeping right now, press any key to go to sleep...  \n\r");
    device.read(&c, 1);
    sleep();

}
#endif //TESTING
void init_cell_SOC(int index)
{
    charge_estimation_state[index] = 0;
    SOC[index] = 100.0;
    SOH[index] = 100.0;
    DOD[index] = 0.0;
    last_voltage[index] = 0.0;
}
void SOC_estimation_update(float current, float voltage, int index) //TODO: Check does positive current mean charging or discharging
{
    if(charge_estimation_state[index]==0){ //Initial State
        if(current>min_current_thresh){
            charge_estimation_state[index] = 3;
        }
        else if(current<-min_current_thresh){
            charge_estimation_state[index] = 2;
        }
        else{
            charge_estimation_state[index] = 1;
        }
    }
    else if(charge_estimation_state[index]==1){ //Transitional State
        if(current>min_current_thresh){
            charge_estimation_state[index] = 3;
        }
        else if(current<-min_current_thresh){
            charge_estimation_state[index] = 2;
        }
        else{
            float voltage_diff = voltage-last_voltage[index];
            if(voltage_diff<=min_voltage_diff && voltage_diff>=-min_voltage_diff)
            {
                constant_voltage_count++;
            }
            else{
                constant_voltage_count = 0;
            }
            if(constant_voltage_count>=constant_voltage_count_thresh){
                charge_estimation_state[index] = 4;
            }
        }
    }
    else if(charge_estimation_state[index]==2){ //Charge State , assumes negative current
        if(current>min_current_thresh){
            charge_estimation_state[index] = 3;
        }
        else if(current>-min_current_thresh){ // Current is essentially 0
            charge_estimation_state[index] = 1;
        }
        else if(current>-min_current_charging_thresh || voltage>=max_voltage){ //Current is low or voltage is high, so charging is done
            charge_estimation_state[index] = 5;
        }
        else{ // calculate DOD and SOC
            DOD[index] = DOD[index] + (current*dt)/rated_capacity;
            SOC[index] = SOH[index] - DOD[index];
        }
    }
    else if(charge_estimation_state[index]==3){ //Discharge State , assumes positive current
        if(current<-min_current_thresh){
            charge_estimation_state[index] = 2;
        }
        else if(current<min_current_thresh){ // Current is essentially 0
            charge_estimation_state[index] = 1;
        }
        else if(voltage<=min_voltage){ //Finished Discharging
            charge_estimation_state[index] = 6;
        }
        else{
            DOD[index] = DOD[index] + (current*dt)/rated_capacity;
            SOC[index] = SOH[index] - DOD[index];
        }
    }
    else if(charge_estimation_state[index]==4){ //Equilibrium, check voltage
        if(current>min_current_thresh){
            charge_estimation_state[index] = 3;
        }
        else if(current<-min_current_thresh){
            charge_estimation_state[index] = 2;
        }
        else{
            for(int i =0;i<callibrate_length-1;i++){ //Voltage calibration
                if(voltage>=voltage_callibrate[i] && voltage<=voltage_callibrate[i+1]){
                    float SOC_voltage = ((voltage-voltage_callibrate[i])*(SOC_callibrate[i+1]-SOC_callibrate[i])/(voltage_callibrate[i+1]-voltage_callibrate[i]))+SOC_callibrate[i]; //SOC based on voltage calibration
                    
                    SOC[index] = SOC_voltage;
                    DOD[index] = SOH[index]-SOC[index];
                }
            }
        }
    }
    else if(charge_estimation_state[index]==5){ //Fully Ccharged
        //TODO: Send signal to turn off Charging contactors
        //TODO: Possibly add voltage calibration
        SOH[index] = SOC[index];
        if(current>min_current_thresh){
            charge_estimation_state[index] = 3;
        }
        else if(current>-min_current_thresh){
            charge_estimation_state[index] = 1;
        }
    }
    else if(charge_estimation_state[index]==6){ //Fully discharged
    //TODO: Send signal to turn off Disharging contactors
        SOH[index] = DOD[index];
        if(current<-min_current_thresh){
            charge_estimation_state[index] = 2; //Go to charging state
        }
        else if(current<min_current_thresh){
            charge_estimation_state[index] = 1; //Transition state
        }
    }
    last_voltage[index] = voltage;
}
void cell_balancing_logic(){ //Cell balancing based on voltage
    float min_cell_voltage = cell_voltages[0];
    for(int i = 0; i < NUM_CELL_NODES; ++i)
    {
        if(cell_voltages[i] < min_cell_voltage){
            min_cell_voltage = cell_voltages[i];
        }
    }
    for(int i = 0; i < NUM_CELL_NODES; ++i)
    {
        if(cell_voltages[i] > (min_cell_voltage+cell_balancing_thresh)){
            //Turn cell balancing on
        }
        else{
            //turn cell balancing off
        }
    }
}

void fan_logic(){
    bool fanOn = false;
    float max_cell_temp = cell_temperatures[0];
    for(int i = 0; i < NUM_CELL_NODES; ++i)
    {
        if(cell_temperatures[i] > temperature_thresh){
            fanOn = true;
        }
        if(cell_temperatures[i] > max_cell_temp){
            max_cell_temp = cell_temperatures[i];
        }
    }
    if(fanOn)
    {
        fan_ctrl.write(1);
        float fan_power = (max_cell_temp-max_cell_temp)/20; 
        if(fan_power > 1.0){
            fan_power = 1.0;
        }
        else if(fan_power < 0.0){
            fan_power = 0.0;
        }
        fan_pwm.write(fan_power);
    }
    else{
        fan_ctrl.write(0);
        fan_pwm.write(0.0); 
    }
}

float get_pack_voltage(){
    // return (pack_volt.read())*VDD*(2+100)/2*10/15;
    // return pack_volt.read()*VDD*12/0.3;
    // return pack_volt.read()*12/0.089;

    float v = 0;
    for (int j = 0; j < NUM_ADC_SAMPLES; ++j)
        v += pack_volt.read();
    v /= NUM_ADC_SAMPLES;

    PRINT("ADC pack voltage: ");
    printFloat(v, 5);
    PRINT("\r\n");

    v = v*12/0.09;

    PRINT("Pack Voltage: ");
    printFloat(v, 2);
    PRINT(" V\r\n");
    
    return v;
}

float get_pack_current()
{
    // return (pack_current.read()*VDD*2.5/2.14-2.5)*1000/1.5;
    // return (pack_current.read()*2.5/0.658-2.5)*1000/1.5;

    float i = 0;
    for (int j = 0; j < NUM_ADC_SAMPLES; ++j)
        i += pack_current.read();
    i /= NUM_ADC_SAMPLES;
    
    PRINT("ADC pack current: ");
    printFloat(i, 5);
    PRINT("\r\n");
    PRINT("ADC pack current offset: ");
    printFloat(i-zero_current_ADC, 5);
    PRINT("\r\n");

    // i = (i*2.5/0.65-2.5)*1000/1.5;
    // i = (i*1.65/0.5-1.65)*100/1.65;
    // i = (i-0.65f)*VDD*100/1.65f;
    // i = (i-0.52f)*VDD*100/1.65f;
    // i = (i-zero_current_ADC)*VDD*100/1.65f;
    // i = (i*2.5/zero_current_ADC-2.5)*1000/1.5;
    // i = (i*1.65/zero_current_ADC-1.65)*100/1.65;
    // i = (i*2.5/zero_current_ADC-2.5)*300/0.625;
    // i = (i*1.65/zero_current_ADC-1.65)*300/0.625/11;
    
    // i = (i-zero_current_ADC)*1.93/0.009;
    i = (i-zero_current_ADC)*300/0.625f/(100/15+1)*3.3f;
    
    PRINT("Pack Current: ");
    printFloat(i, 2);
    PRINT(" A\r\n");

    return i;
}

// WARNING: This method is NOT safe to call in an ISR context (if RTOS is enabled)
// This method is Thread safe (CAN is Thread safe)
bool sendCANMessage(const char *data, const unsigned char len = 8)
{
    if (len > 8 || !intCan.write(CANMessage(2, data, len)))
        return false;

    return true;
}

// WARNING: This method will be called in an ISR context
void canTxIrqHandler()
{
    string toSend = "MainSend";
    if (sendCANMessage(toSend.c_str(), toSend.length()))
    {
        PRINT("Message sent: %s\r\n", toSend.c_str()); // This should be removed except for testing CAN
    }
}

// WARNING: This method will be called in an ISR context
void canRxIrqHandler()
{
    CANMessage receivedCANMessage;
    while (intCan.read(receivedCANMessage))
    {
        PRINT("Message received: %s\r\n", receivedCANMessage.data); // This should be changed to copying the CAN data to a global variable, except for testing CAN
    }
}

void canInit()
{
    intCanTxTicker.attach(&canTxIrqHandler, 1s);
    intCan.attach(&canRxIrqHandler, CAN::RxIrq);
    intCanStby = 0;
}

void currentSensorInit()
{
    float i = 0;
    for (int j = 0; j < NUM_ADC_SAMPLES; ++j)
        i += pack_current.read();
    i /= NUM_ADC_SAMPLES;

    zero_current_ADC = i;

    PRINT("Calibrated Current sensor to ADC value of: ");
    printFloat(zero_current_ADC, 5);
    PRINT("\r\n");
}

int main() {
    // This is only necessary if using software reset (not NRST pin)
    HAL_DBGMCU_EnableDBGSleepMode();
    HAL_DBGMCU_EnableDBGStandbyMode();
    HAL_DBGMCU_EnableDBGStopMode();

    // device.set_baud(38400);
    PRINT("start main() \n\r");

    canInit();
    thread_sleep_for(2000);
    currentSensorInit();

    while(1){
        PRINT("main thread loop\r\n");
#ifdef TESTING
        test_point_0 = test_point_0 ^ 1;
        test_pack_voltage(0, 1);
        test_pack_current(0, 1);
        // test_fan_output();
#endif //TESTING
        get_pack_voltage();
        get_pack_current();
        thread_sleep_for(1000);
        PRINT("\r\n");
    }
}
