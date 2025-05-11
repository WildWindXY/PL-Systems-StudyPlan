#include <stdio.h>

int isTmax(int x) {
    return !(x + x + 2) & !!(x + 1);
}

int isTmaxVolatile(int x) {
    volatile int a = x + x + 2;
    volatile int b = x + 1;
    return !a & !!b;
}

int main() {
    printf("isTmax(0x7fffffff)         = %d\n", isTmax(0x7fffffff));
    printf("isTmaxVolatile(0x7fffffff) = %d\n", isTmaxVolatile(0x7fffffff));
    return 0;
}