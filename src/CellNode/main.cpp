#include <mbed.h>
#include <rtos.h>
#include <mbed_events.h>
#include <string>
#include <list>
#include <../CellNode/pindef.h>
#include <iostream>

AnalogIn cell_volt(CELL_VOLTAGE);
AnalogOut balance_out(BALANCING_CONTROL);
bool test_cell_voltage(double test_min, double test_max){
    float v = cell_volt.read();
    int v_int = (int)v;
    int v_dec = (int)(v/100);
    cout << v_int << "." << v_dec << endl;
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
    balance_out.write(0.0); // switch balancing off
    printf("Balance Output set to Low, measure current and then press any key to continue... \n\r");
    char c;
    //while(1)
    //{
    device.read(&c, 1);
    balance_out.write(1.0); // switch balancing on
    printf("Balance Output set to High, measure current. Did test pass (y/n)? \n\r");
    device.read(&c, 1);
    if(c=='y'){
        return true;
    }

    return false;
}
int main() {
    while(1)
    {
        // do nothing
    }
}
