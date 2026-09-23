/* Programa auxiliar dos testes: imprime e sai conforme os argumentos.
 *
 *   fake_tool [--stdout <texto>] [--stderr <texto>] [--exit <código>]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int code = 0;

    for (int i = 1; i + 1 < argc; i += 2) {
        if (strcmp(argv[i], "--stdout") == 0)
            fputs(argv[i + 1], stdout);
        else if (strcmp(argv[i], "--stderr") == 0)
            fputs(argv[i + 1], stderr);
        else if (strcmp(argv[i], "--exit") == 0)
            code = atoi(argv[i + 1]);
    }

    return code;
}
