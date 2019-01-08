#include <iostream>

extern "C" {
double f(double);
}

int main() {
    int i = 0;
    while (11 - i) {
        printf("f( %d ) = %f\n", i, f(i));
        i = i + 1;
    }
}