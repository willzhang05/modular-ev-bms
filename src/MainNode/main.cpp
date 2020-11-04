#include <mbed.h>
#include <rtos.h>
#include <mbed_events.h>
#include <string>
#include <list>
#include <../MainNode/pindef.h>
#include <iostream>

AnalogIn pack_volt(PACK_VOLTAGE);
AnalogIn pack_current(PACK_CURRENT);

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

int main() {
    while(1)
    {
        // do nothing
    }
}
