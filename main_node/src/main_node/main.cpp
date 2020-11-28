#include <mbed.h>
#include <rtos.h>
#include <mbed_events.h>
#include <string>
#include <list>
#include "pindef.h"
#include <iostream>

BufferedSerial device(USBTX, USBRX);

AnalogIn pack_volt(PACK_VOLTAGE);
AnalogIn pack_current(PACK_CURRENT);

DigitalOut fan_ctrl(FAN_CTRL);
PwmOut fan_pwm(FAN_PWM);

DigitalOut charge_contactor(CHARGE_CONTACTOR_CTRL);
DigitalOut discharge_contactor(DISCHARGE_CONTACTOR_CTRL);

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

bool test_pack_voltage(float test_min, float test_max){
    float v = pack_volt.read();
    int v_int = (int)v;
    int v_dec = (int)(v/100);
    cout << v_int << "." << v_dec << endl;
    if(v>=test_min && v<=test_max){
        printf("Pack Voltage Test PASSED \n\r");
        return true;
    }
    else
    {
        printf("Pack Voltage Test FAILED \n\r");
        return false; 
    }
}

bool test_pack_current(float test_min, float test_max){
    float i = pack_current.read();
    int i_int = (int)i;
    int i_dec = (int)(i/100);
    cout << i_int << "." << i_dec << endl;
    if(i>=test_min && i<=test_max){
        printf("Pack Current Test PASSED \n\r");
        return true;
    }
    else
    {
        printf("Pack Current Test FAILED \n\r");
        return false; 
    }
}

bool test_fan_output(){
    char c;
    fan_ctrl.write(0);
    fan_pwm.write(0.0); 
    printf("Fan ctrl set to Low, press any key to continue...  \n\r");
    device.read(&c, 1);
    fan_ctrl.write(1);
    fan_pwm.write(0.1);
    printf("Fan ctrl set to High, PWM set to Low, press any key to continue...  \n\r");
    device.read(&c, 1);
    fan_ctrl.write(1);
    fan_pwm.write(0.9);
    printf("Fan ctrl set to High, PWM set to High, Did test pass (y/n)? \n\r");
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
    printf("Discharge Contactor Output set to Low, measure current and then press any key to continue... \n\r");
    char c;
    //while(1)
    //{
    device.read(&c, 1);
    discharge_contactor.write(1); // switch balancing on
    printf("Discharge Contactor Output set to High, measure current. Did test pass (y/n)? \n\r");
    device.read(&c, 1);
    discharge_contactor.write(0); // switch balancing off
    if(c=='y'){
        return true;
    }

    return false;
}

bool test_charge_contactor(){
    charge_contactor.write(0); // switch balancing off
    printf("Charge Contactor Output set to Low, measure current and then press any key to continue... \n\r");
    char c;
    //while(1)
    //{
    device.read(&c, 1);
    charge_contactor.write(1); // switch balancing on
    printf("Ccharge Contactor Output set to High, measure current. Did test pass (y/n)? \n\r");
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
    printf("Board not sleeping right now, press any key to go to sleep...  \n\r");
    device.read(&c, 1);
    sleep();

}
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
int main() {
    while(1)
    {
        // do nothing
    }
}
