#include <sys/wait.h>

#include <cstdlib>
#include <iostream>
#include <vector>

#include "dbg.hpp"

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
        proc.insert_breakpoint(0x4011c0);
        proc.continue_process();
        std::cout << "Hit breakpoint. Press ENTER to continue." << std::endl;
        std::cin.get();
        proc.continue_process();
        std::cout << "Hit breakpoint. Press ENTER to continue." << std::endl;
        std::cin.get();
        proc.continue_process();
        std::cout << "Hit breakpoint. Press ENTER to continue." << std::endl;
        std::cin.get();
        std::cout << "Read " << std::hex << proc.read_register(Register::RSI, 8) << std::dec << '\n';
        proc.write_register(Register::RSI, 2, 0x1234ULL);
        proc.continue_process();
        int exit = proc.wait_process_exit();
        std::cout << "Got exit code " << exit << ".\n";
    } catch (const std::system_error& e) {
        std::cerr << "Got error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
