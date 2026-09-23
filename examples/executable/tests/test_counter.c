#include <stdio.h>
#include <string.h>

#include "text/counter.h"

static int failures = 0;

static void check(const char *text, size_t lines, size_t words, size_t chars) {
    counter_t counter;
    counter_init(&counter);
    counter_feed(&counter, text, strlen(text));

    if (counter.lines != lines || counter.words != words || counter.chars != chars) {
        printf("FAIL \"%s\": got %zu/%zu/%zu, expected %zu/%zu/%zu\n", text,
               counter.lines, counter.words, counter.chars, lines, words, chars);
        failures++;
    }
}

int main(void) {
    check("", 0, 0, 0);
    check("one", 0, 1, 3);
    check("one two\n", 1, 2, 8);
    check("  spaced   out  \n\n", 2, 2, 18);

    // A word split between two chunks is counted once.
    counter_t counter;
    counter_init(&counter);
    counter_feed(&counter, "hel", 3);
    counter_feed(&counter, "lo world", 8);
    if (counter.words != 2) {
        printf("FAIL chunks: got %zu words, expected 2\n", counter.words);
        failures++;
    }

    printf("%s\n", failures ? "counter: FAILED" : "counter: ok");
    return failures ? 1 : 0;
}
