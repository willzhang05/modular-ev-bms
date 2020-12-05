#include "Printing.h"
#include <mbed.h>

// Input: an integer representing a float with 2 digits past decimal multiplied by 100
// Output: print num as a float
void printIntegerAsFloat(int num) {
    int left = num;
    int right = num;
    left /= 100;
    right -= left * 100;
    right = abs(right);
    if(right < 10) {
        printf("%d.%d%d", left, 0, right);
    }
    else {
        printf("%d.%d", left, right);
    }
}

// Input: a float
// Output: print num as a float
void printFloat(float num) {
    int left = abs((int)(num));
    int right = abs((int)(num * 10000));
    right -= left * 10000;
    string toPrint = "";
    if(num < 0)
        toPrint += "-";
    toPrint += "%d.";
    if(right < 10) {
        toPrint += "0";
        // printf("%d.000%d", left, right);
    }
    if(right<100) {
        toPrint += "0";
        // printf("%d.00%d", left, right);
    }
    if(right<1000) {
        toPrint += "0";
        // printf("%d.0%d", left, right);
    }
    toPrint += "%d";
    printf(toPrint.c_str(), left, right);
}