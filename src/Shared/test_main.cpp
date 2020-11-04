#include <mbed.h>
#include <rtos.h>
#include <mbed_events.h>
#include <string>
#include <list>
#include <../DevBoards/pindef.h>
#include <iostream>

using namespace std;

BufferedSerial device(USBTX, USBRX);

CAN can1(PD_0, PD_1);
CAN can2(PB_5, PB_6);

AnalogIn cell_volt(CELL_VOLTAGE);
AnalogOut balance_out(BALANCING_CONTROL);
DigitalIn button(USER_BUTTON);
DigitalOut rx_led(LED2);
DigitalOut tx_led(LED3);

Thread send_thread;
Thread recv_thread;
Thread test_thread;

Mutex printMutex;

void send_can_message(string message) {
    CANMessage msg(1, message.c_str(), message.length() + 1);
    tx_led = 1;
    if (can1.write(msg)) {
        printMutex.lock();
        printf("[CAN1] Sent CAN message '%s'\n\r", message.c_str());
        printMutex.unlock();
    } else {
        printMutex.lock();
        printf("[CAN1] Failed to send CAN message '%s'\n\r", message.c_str());
        printMutex.unlock();
    }
    tx_led = 0;
}

void recv_func() {
    CANMessage message;
    printMutex.lock();
    printf("[CAN2] Starting thread to receive CAN messages...\n\r");
    printMutex.unlock();
    while (1) {
        while (can2.read(message)) {
            rx_led = 1;
            printMutex.lock();
            printf("[CAN2] '%s'\n\r", message.data);
            printMutex.unlock();
        }
        rx_led = 0;
    }
}

void read_from_serial() {
    string buffer = "";
    list<string> messages;
    char c;
    printMutex.lock();
    printf("[CAN1] Starting thread to listening for input...\n\r");
    printMutex.unlock();
    while (1) {
        while (1) {
            device.read(&c, 1);
            printMutex.lock();
            printf("%c", c);
            printMutex.unlock();
            if (c == '\r') {
                messages.push_back(buffer);
                break;
            } else {
                buffer += c;
            }

            if (buffer.length() == 7) {
                messages.push_back(buffer);
                buffer = "";
            }
        }
        printMutex.lock();
        printf("\n\r");
        printMutex.unlock();
        while (!messages.empty()) {
            send_can_message(messages.front());
            messages.pop_front();
        }
        while (device.readable()) {
            device.read(&c, 1);
            printMutex.lock();
            printf("%c\n\r", c);
            printMutex.unlock();
        }
    }

}
void test_func(){
    printMutex.lock();
    printf("bb\n\r");
    printMutex.unlock();
}
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
    device.set_baud(38400);
    bool t = test_balance_output();
    cout << t << endl;
    //send_thread.start(read_from_serial);
    //recv_thread.start(recv_func);
    //test_cell_voltage(0.0,1.0);
    //test_thread.start(test_func);
}

