#ifndef COUNTER_H
#define COUNTER_H

#include <stdbool.h>
#include <stddef.h>

/* Counts lines, words and characters of a text fed in chunks. */
typedef struct {
    size_t lines;
    size_t words;
    size_t chars;
    bool in_word;
} counter_t;

void counter_init(counter_t *counter);
void counter_feed(counter_t *counter, const char *data, size_t size);

#endif
