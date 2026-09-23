#include "cmd_command.h"

int main(int argc, char *argv[]) {
    // Keep stdout and the error logs (stderr) in order even when the output is redirected.
    setvbuf(stdout, nullptr, _IOLBF, 0);

    // Debug messages only in debug builds (DEBUG=1) or with IDL_DEBUG set in the environment.
    const char *debug_env = getenv("IDL_DEBUG");
    bool debug = DEBUG || (debug_env && debug_env[0] && strcmp(debug_env, "0") != 0);
    log_set_level(debug ? LOG_TRACE : LOG_INFO);

    return idl_execute_command(argc, argv);
}
