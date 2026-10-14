#include <stdio.h>
int main(void) {
    int x = 5;
    int unused1 = x * 100;          /* dead */
    int unused2 = unused1 + 3;      /* dead (only feeds a dead value) */
    int y = x + 1;
    (void)unused2;
    printf("%d\n", y);
    return 0;
}
