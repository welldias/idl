#include <stdio.h>

int case_value(void);
int case_version(void);

int main(void) {
    int passed = case_value() + case_version();
    printf("suite: %d of 2 cases passed\n", passed);
    return passed == 2 ? 0 : 1;
}
