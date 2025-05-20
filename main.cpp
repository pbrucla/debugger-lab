#include <sys/wait.h>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "dbg.hpp"
std::unordered_map<uint64_t, uint64_t> fake_memory;

unsigned long long int x = 0x1337133713371337ULL;

int main(int argc, char* argv[], char* envp[]) {
    const char* argv0 = argc == 0 ? "cydbg" : argv[0];
    if (argc < 2) {
        std::cerr << "Usage: " << argv0 << " [PROGRAM]\n";
        return 1;
    }

    char* const progname = argv[1];
    std::vector<char*> args;
    args.push_back(progname);
    args.insert(args.end(), argv + 2, argv + argc);
    args.push_back(nullptr);

    Tracee proc;

    try {
        proc.spawn_process(progname, args.data(), envp);
        proc.insert_breakpoint(0x40114c);
        proc.continue_process();
        uint64_t bp = proc.read_register(Register::RBP, 8);
        auto [addresstest, bptest] = proc.get_stackframe(bp);
        std::cout << std::hex;
        std::cout << "bp " << bp << ".\n";
        std::cout << "addresstest " << addresstest << ".\n";
        std::cout << "bptest " << bptest << ".\n";
        auto bt = proc.backtrace();
        for (const auto& addr: bt) {
            std::cout << "backtrace: 0x" << addr << '\n';
        }

        std::cout << "Hit breakpoint. Press ENTER to continue." << std::endl;
        std::cin.get();
        proc.continue_process();
        std::cout << "Got exit code " << exit << ".\n";
    } catch (const std::system_error& e) {
        std::cerr << "Got error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
