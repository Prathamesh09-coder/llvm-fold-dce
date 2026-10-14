#include <stdio.h>
int main(void) {
    int a = 6, b = 7;
    int c = a * b + (10 - 4) / 2;   /* folds to 45 */
    int d = (c > 40) ? c << 1 : 0;  /* folds to 90 */
    printf("%d %d\n", c, d);
    return 0;
}
