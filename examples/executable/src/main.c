#include <stdio.h>
#include <string.h>

#include "os/os_name.h"
#include "text/counter.h"

static int count_stream(FILE *f, const char *label) {
    counter_t counter;
    counter_init(&counter);

    char buffer[4096];
    size_t size;
    while ((size = fread(buffer, 1, sizeof(buffer), f)) > 0)
        counter_feed(&counter, buffer, size);

    printf("%8zu %8zu %8zu %s\n", counter.lines, counter.words, counter.chars, label);
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--version") == 0) {
        printf("wordcount 1.0 (%s)\n", os_name());
        return 0;
    }

    if (argc == 1)
        return count_stream(stdin, "-");

    int status = 0;
    for (int i = 1; i < argc; i++) {
        FILE *f = fopen(argv[i], "rb");
        if (!f) {
            fprintf(stderr, "wordcount: cannot open %s\n", argv[i]);
            status = 1;
            continue;
        }
        count_stream(f, argv[i]);
        fclose(f);
    }
    return status;
}
