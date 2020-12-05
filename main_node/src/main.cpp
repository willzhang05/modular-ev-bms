#include <mbed.h>
#include "pindef.h"

// #define TESTING     // only defined if using test functions
#define PRINTING    // only defined if using printf functions

#define NUM_ADC_SAMPLES 10
#define NUM_CELL_NODES  2
// #define VDD 3.3f

#ifdef PRINTING
#include "Printing.h"
#endif

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
#ifdef PRINTING
    printf("ADC pack voltage: ");
    printFloat(v);
    printf("\r\n");
#endif //PRINTING
    if(v>=test_min && v<=test_max){
#ifdef PRINTING
        printf("Pack Voltage Test PASSED \n\r");
#endif //PRINTING
        return true;
    }
    else
    {
#ifdef PRINTING
        printf("Pack Voltage Test FAILED \n\r");
#endif //PRINTING
        return false; 
    }
}

bool test_pack_current(float test_min, float test_max){
    float i = pack_current.read();
#ifdef PRINTING
    printf("ADC pack current: ");
    printFloat(i);
    printf("\r\n");
#endif //PRINTING
    if(i>=test_min && i<=test_max){
#ifdef PRINTING
        printf("Pack Current Test PASSED \n\r");
#endif //PRINTING
        return true;
    }
    else
    {
#ifdef PRINTING
        printf("Pack Current Test FAILED \n\r");
#endif //PRINTING
        return false; 
    }
}

bool test_fan_output(){
    char c;
    fan_ctrl.write(0);
    fan_pwm.write(0.0);
#ifdef PRINTING
    printf("Fan ctrl set to Low, press any key to continue...  \n\r");
#endif //PRINTING
    device.read(&c, 1);
    fan_ctrl.write(1);
    fan_pwm.write(0.1);
#ifdef PRINTING
    printf("Fan ctrl set to High, PWM set to Low, press any key to continue...  \n\r");
#endif //PRINTING
    device.read(&c, 1);
    fan_ctrl.write(1);
    fan_pwm.write(0.9);
#ifdef PRINTING
    printf("Fan ctrl set to High, PWM set to High, Did test pass (y/n)? \n\r");
#endif //PRINTING
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
#ifdef PRINTING
    printf("Discharge Contactor Output set to Low, measure current and then press any key to continue... \n\r");
#endif //PRINTING
    char c;
    //while(1)
    //{
    device.read(&c, 1);
    discharge_contactor.write(1); // switch balancing on
#ifdef PRINTING
    printf("Discharge Contactor Output set to High, measure current. Did test pass (y/n)? \n\r");
#endif //PRINTING
    device.read(&c, 1);
    discharge_contactor.write(0); // switch balancing off
    if(c=='y'){
        return true;
    }

    return false;
}

bool test_charge_contactor(){
    charge_contactor.write(0); // switch balancing off
#ifdef PRINTING
    printf("Charge Contactor Output set to Low, measure current and then press any key to continue... \n\r");
#endif //PRINTING
    char c;
    //while(1)
    //{
    device.read(&c, 1);
    charge_contactor.write(1); // switch balancing on
#ifdef PRINTING
    printf("Ccharge Contactor Output set to High, measure current. Did test pass (y/n)? \n\r");
#endif //PRINTING
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
#ifdef PRINTING
    printf("Board not sleeping right now, press any key to go to sleep...  \n\r");
#endif //PRINTING
    device.read(&c, 1);
    sleep();

}
#endif //TESTING

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

#ifdef PRINTING
    printf("ADC pack voltage: ");
    printFloat(v);
    printf("\r\n");
#endif //PRINTING

    v = v*12/0.09;

#ifdef PRINTING
    printf("Pack Voltage: ");
    printFloat(v);
    printf(" V\r\n");
#endif //PRINTING
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
    
#ifdef PRINTING
    printf("ADC pack current: ");
    printFloat(i);
    printf("\r\n");
    printf("ADC pack current offset: ");
    printFloat(i-zero_current_ADC);
    printf("\r\n");
#endif //PRINTING

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
    
#ifdef PRINTING
    printf("Pack Current: ");
    printFloat(i);
    printf(" A\r\n");
#endif //PRINTING

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
#ifdef PRINTING
        printf("Message sent: %s\r\n", toSend.c_str()); // This should be removed except for testing CAN
#endif //PRINTING
    }
}

// WARNING: This method will be called in an ISR context
void canRxIrqHandler()
{
    CANMessage receivedCANMessage;
    while (intCan.read(receivedCANMessage))
    {
#ifdef PRINTING
        printf("Message received: %s\r\n", receivedCANMessage.data); // This should be changed to copying the CAN data to a global variable, except for testing CAN
#endif //PRINTING
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

#ifdef PRINTING
    printf("Calibrated Current sensor to ADC value of: ");
    printFloat(zero_current_ADC);
    printf("\r\n");
#endif //PRINTING
}

int main() {
    // This is only necessary if using software reset (not NRST pin)
    HAL_DBGMCU_EnableDBGSleepMode();
    HAL_DBGMCU_EnableDBGStandbyMode();
    HAL_DBGMCU_EnableDBGStopMode();

    // device.set_baud(38400);
#ifdef PRINTING
    printf("start main() \n\r");
#endif //PRINTING

    canInit();
    thread_sleep_for(2000);
    currentSensorInit();

    while(1){
#ifdef PRINTING
        printf("main thread loop\r\n");
#endif //PRINTING
#ifdef TESTING
        test_point_0 = test_point_0 ^ 1;
        test_pack_voltage(0, 1);
        test_pack_current(0, 1);
        // test_fan_output();
#endif //TESTING
        get_pack_voltage();
        get_pack_current();
        thread_sleep_for(1000);
#ifdef PRINTING
        printf("\r\n");
#endif //PRINTING
    }
}
