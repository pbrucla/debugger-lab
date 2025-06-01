#include <stdio.h>

#include <system_error>

#include "dbg.hpp"
#include "operation.hpp"

int main(int argc, char* argv[], char* envp[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program> [args...]\n", argv[0]);
        return 1;
    }

    Tracee proc(argv[1]);
    Operation op(proc);

    try {
        proc.spawn_process(argv + 1, envp);
        while (true) {
            op.parse_and_run();
        }
    } catch (const std::system_error& e) {
        fprintf(stderr, "Got error: %s\n", e.what());
        return 1;
    }

    return 0;
}
