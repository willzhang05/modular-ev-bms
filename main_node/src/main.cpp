#include <mbed.h>
#include <pindef.h>

// #define TESTING     // only defined if using test functions
#define PRINTING    // only defined if using printf functions

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

int cell_num = 2;
int max_cell_node = 50;
float cell_voltages [50];
float cell_balancing_thresh = 0.3;
float cell_temperatures [50];
float temperature_thresh = 30.0;
float VDD = 3.3;

int current_length = 7;
float c_voltage_output [7] = {0, 0.5, 1, 2.5, 4, 4.5, 5};
float current [7] = {-2700, -1300, -700, 30, 1300, 1500, 3300};

#ifdef TESTING
DigitalOut test_point_0(UNUSED_PIN_0);
#endif //TESTING

#ifdef TESTING
bool test_pack_voltage(float test_min, float test_max){
    float v = pack_volt.read();
#ifdef PRINTING
    int v_int = (int)v;
    int v_dec = (int)(v*100);
    printf("%d.%d\n\r", v_int, v_dec);
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
    int i_int = (int)i;
    int i_dec = (int)(i*100);
    printf("%d.%d\n\r", i_int, i_dec);
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
    if(cell_num>0){
        float min_cell_voltage = cell_voltages[0];
        for(int i = 0;i<cell_num;i++ )
        {
            if(cell_voltages[i]<min_cell_voltage){
                min_cell_voltage = cell_voltages[i];
            }
        }
        for(int i = 0;i<cell_num;i++ )
        {
            if(cell_voltages[i]>(min_cell_voltage+cell_balancing_thresh)){
                //Turn cell balancing on
            }
            else{
                //turn cell balancing off
            }
        }
    }
    
}

void fan_logic(){
    if(cell_num>0){
        bool fanOn = false;
        float max_cell_temp = cell_temperatures[0];
        for(int i = 0;i<cell_num;i++ )
        {
            if(cell_temperatures[i]>temperature_thresh){
                fanOn = true;
            }
            if(cell_temperatures[i]>max_cell_temp){
                max_cell_temp = cell_temperatures[i];
            }
        }
        if(fanOn)
        {
            fan_ctrl.write(1);
            float fan_power = (max_cell_temp-max_cell_temp)/20; 
            if(fan_power>1.0){
                fan_power = 1.0;
            }
            else if(fan_power<0.0){
                fan_power = 0.0;
            }
            fan_pwm.write(fan_power);
        }
        else{
            fan_ctrl.write(0);
            fan_pwm.write(0.0); 
        }
    }
}

float get_pack_voltage(){
    float v = pack_volt.read()*VDD*(180+90000)/180;
    return v;
}

float get_cell_voltage(){
    float c_voltage = pack_current.read()*VDD;
    for(int i =0;i<current_length-1;i++){ //Voltage calibration
        if(c_voltage>=c_voltage_output[i] && c_voltage<=c_voltage_output[i+1]){
            return ((c_voltage-c_voltage_output[i])*(current[i+1]-current[i])/(c_voltage_output[i+1]-c_voltage_output[i]))+current[i]; //SOC based on voltage calibration
        }
    }
    return -2800.0;
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
    intCanTxTicker.attach(&canTxIrqHandler, 1); // float, in seconds
    intCan.attach(&canRxIrqHandler, CAN::RxIrq);
    intCanStby = 0;
}

int main() {
    // device.set_baud(38400);
#ifdef PRINTING
    printf("start main() \n\r");
#endif //PRINTING

    canInit();

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
        thread_sleep_for(1000);
#ifdef PRINTING
        printf("\r\n");
#endif //PRINTING
    }
}
