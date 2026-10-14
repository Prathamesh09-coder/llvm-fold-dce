#include <stdio.h>
static int square(int v) { return v * v; }
int main(void) {
    int n = 4;
    int k = 2 * 8 + 1;              /* folds to 17 */
    int dead = k * n;               /* dead */
    int s = square(n) + k;          /* square(n) stays (call), + k uses folded const */
    for (int i = 0; i < 3; i++)
        s += i * (3 + 4);           /* 3 + 4 folds to 7 */
    (void)dead;
    printf("%d\n", s);
    return 0;
}
