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
    int left = (int)(num);
    int right = (int)(num * 100);
    right -= left * 100;
    right = abs(right);
    if(right < 10) {
        printf("%d.%d%d", left, 0, right);
    }
    else {
        printf("%d.%d", left, right);
    }
}