#include <mbed.h>
#include "pindef.h"

BufferedSerial device(USBTX, USBRX);

AnalogIn cell_volt(CELL_VOLTAGE);
AnalogIn cell_temp(TEMPERATURE_DATA);
DigitalOut balance_out(BALANCING_CONTROL);
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

int callibrate_length = 7;
float voltage_callibrate [7] = {0, 2.5, 2.8, 3.3, 3.6, 4.2, 10};
float SOC_callibrate [7] = {0, 0, 20, 80, 90, 100, 100};

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

bool test_cell_voltage(float test_min, float test_max){
    float v = cell_volt.read();
    int v_int = (int)v;
    int v_dec = (int)(v/100);
    printf("%d.%d\n\r", v_int, v_dec);
    if(v>=test_min && v<=test_max){
        printf("Cell Voltage Test PASSED \n\r");
        return true;
    }
    else
    {
        printf("Cell Voltage Test FAILED \n\r");
        return false; 
    }
}
bool test_balance_output(){
    balance_out.write(0); // switch balancing off
    printf("Balance Output set to Low, measure current and then press any key to continue... \n\r");
    char c;
    //while(1)
    //{
    device.read(&c, 1);
    balance_out.write(1); // switch balancing on
    printf("Balance Output set to High, measure current. Did test pass (y/n)? \n\r");
    device.read(&c, 1);
    balance_out.write(0); // switch balancing off
    if(c=='y'){
        return true;
    }

    return false;
}


bool test_cell_temperature(float test_min, float test_max){
    float t = cell_temp.read();
    int t_int = (int)t;
    int t_dec = (int)(t/100);
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
    char c;
    printf("Board not sleeping right now, press any key to go to sleep...  \n\r");
    device.read(&c, 1);
    sleep();

}
int main() {
    while(1)
    {
        // do nothing
    }
}
