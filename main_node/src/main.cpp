#include <mbed.h>
#include <pindef.h>

BufferedSerial device(USBTX, USBRX);

AnalogIn pack_volt(PACK_VOLTAGE);
AnalogIn pack_current(PACK_CURRENT);

DigitalOut fan_ctrl(FAN_CTRL);
PwmOut fan_pwm(FAN_PWM);

DigitalOut charge_contactor(CHARGE_CONTACTOR_CTRL);
DigitalOut discharge_contactor(DISCHARGE_CONTACTOR_CTRL);
DigitalOut test_point_0(UNUSED_PIN_0);

bool test_pack_voltage(float test_min, float test_max){
    float v = pack_volt.read();
    int v_int = (int)v;
    int v_dec = (int)(v*100);
    printf("%d.%d\n\r", v_int, v_dec);
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
    printf("%d.%d\n\r", i_int, i_dec);
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
    fan_ctrl.write(0.0);
    fan_pwm.write(0.1);
    printf("Fan ctrl set to High, PWM set to Low, press any key to continue...  \n\r");
    device.read(&c, 1);
    fan_ctrl.write(0.0);
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

int main() {
    // device.set_baud(38400);
    printf("test start \n\r");

    while(1){
        test_point_0 = test_point_0 ^ 1;
        printf("writing \n\r");
        test_pack_voltage(0, 1);
        thread_sleep_for(1000);
        printf("\r\n");
    }
}
