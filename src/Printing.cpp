#include "Printing.h"
#include <mbed.h>

// Input: an integer representing a float with decimals digits past decimal multiplied by 10^decimals
// Output: print num as a float
void printIntegerAsFloat(int num, int decimals) {
    int left = num;
    int right = num;
    int d = decimals;

    if(left < 0)
        printf("-");
    
    int mult = 1;
    for(int i = 0; i < d; ++i)
        mult *= 10;
    
    left = abs(left/mult);
    right = abs(right) - left * mult;

    printf("%d.", left);

    for(int i = 10; i < mult; i*=10)
    {
        if(right < i)
            printf("0");
    }

    printf("%d", right);
}

// Input: a float
// Output: print num as a float
void printFloat(float num, int decimals) {
    float n = num;
    int d = decimals;

    int mult = 1;
    for(int i = 0; i < d; ++i)
        mult *= 10;
    
    printIntegerAsFloat((int)(n*mult), d);
}