#include "cmd_command.h"

int main(int argc, char *argv[]) {
    // Mantém a ordem entre stdout e os logs de erro (stderr) mesmo quando a saída é redirecionada.
    setvbuf(stdout, nullptr, _IOLBF, 0);
    return idl_execute_command(argc, argv);
}
