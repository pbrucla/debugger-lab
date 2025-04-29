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
        //    unsigned long long x;
        // proc.read_memory(0x407008, &x, sizeof(x));
        //     std::cout << "Read a value of: " << std::hex << x << std::dec << '\n';
        proc.insert_breakpoint(0x4011c0);
        proc.continue_process();
        std::cout << "Hit breakpoint. Press ENTER to continue." << std::endl;
        std::cin.get();
        proc.continue_process();
        std::cout << "Hit breakpoint. Press ENTER to continue." << std::endl;
        std::cin.get();
        proc.continue_process();
        std::cout << "Going to try to syscall" << std::endl;
        char* a = "ab\n";
        unsigned long ab = (unsigned long) a;
        std::array<unsigned long, 6> b= {{1, ab, 3,0,0,0}};
        proc.syscall(1, b);
        std::cout << "Hit breakpoint. Press ENTER to continue." << std::endl;
        std::cin.get();
        proc.continue_process();
        int exit = proc.wait_process_exit();
        std::cout << "Got exit code " << exit << ".\n";
    } catch (const std::system_error& e) {
        std::cerr << "Got error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
