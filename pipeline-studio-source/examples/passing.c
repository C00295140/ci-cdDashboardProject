#include <stdio.h>

static int add(int a, int b) { return a + b; }

int main(void) {
    if (add(2, 3) != 5) { puts("FAIL: positive addition"); return 1; }
    puts("PASS: positive addition");
    if (add(-2, 2) != 0) { puts("FAIL: mixed signs"); return 1; }
    puts("PASS: mixed signs");
    if (add(0, 0) != 0) { puts("FAIL: zero addition"); return 1; }
    puts("PASS: zero addition");
    puts("3 tests passed.");
    return 0;
}
