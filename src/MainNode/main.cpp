#include <mbed.h>
#include <rtos.h>
#include <mbed_events.h>
#include <string>
#include <list>
#include <../MainNode/pindef.h>
#include <iostream>

AnalogIn cell_volt(PACK_VOLTAGE);

bool test_cell_voltage(double test_min, double test_max){
    float v = cell_volt.read();
    int v_int = (int)v;
    int v_dec = (int)(v/100);
    cout << v_int << "." << v_dec << endl;
    cout << (int)(v/100) << endl;
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

int main() {
    while(1)
    {
        // do nothing
    }
}
