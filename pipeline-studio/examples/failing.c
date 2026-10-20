#include <stdio.h>

/* Deliberate logic bug: builds successfully, then fails its test. */
static int add(int a, int b) { return a - b; }

int main(void) {
    if (add(2, 3) != 5) { puts("FAIL: add(2, 3) should equal 5"); return 1; }
    puts("PASS: addition");
    return 0;
}
