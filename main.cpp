#include <sys/wait.h>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "dbg.hpp"
#include "elf.hpp"
#include "operation.hpp"

unsigned long long int x = 0x1337133713371337ULL;

int main(int argc, char* argv[], char* envp[]) {
    const char* argv0 = argc == 0 ? "cydbg" : argv[0];
    if (argc < 2) {
        std::cerr << "Usage: " << argv0 << " [PROGRAM]\n";
        return 1;
    }

    char* const progname = argv[1];
    ELF elf(progname);
    auto main_sym = elf.lookup_sym("main");
    printf("main: %#lx\n", main_sym.value());
    std::vector<char*> args;
    args.push_back(progname);
    args.insert(args.end(), argv + 2, argv + argc);
    args.push_back(nullptr);

    Tracee proc;
    Operation op(proc, elf);

    try {
        proc.spawn_process(progname, args.data(), envp);
        while (true)
        {
            op.parse_and_run();
        }        
    } catch (const std::system_error& e) {
        std::cerr << "Got error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
