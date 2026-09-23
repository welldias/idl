#include "counter.h"

#include <ctype.h>

void counter_init(counter_t *counter) {
    counter->lines = 0;
    counter->words = 0;
    counter->chars = 0;
    counter->in_word = false;
}

void counter_feed(counter_t *counter, const char *data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        unsigned char c = (unsigned char)data[i];
        counter->chars++;
        if (c == '\n')
            counter->lines++;

        if (isspace(c)) {
            counter->in_word = false;
        } else if (!counter->in_word) {
            counter->in_word = true;
            counter->words++;
        }
    }
}
