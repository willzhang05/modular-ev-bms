#include <mbed.h>
#include <rtos.h>
#include <mbed_events.h>
#include <string>
#include <list>

BufferedSerial device(USBTX, USBRX);

CAN can1(PD_0, PD_1);
CAN can2(PB_5, PB_6);

DigitalIn button(USER_BUTTON);
DigitalOut rx_led(LED2);
DigitalOut tx_led(LED3);

Thread send_thread;
Thread recv_thread;

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

int main() {
    device.set_baud(38400);
    send_thread.start(read_from_serial);
    recv_thread.start(recv_func);
}

