#include <sys/wait.h>

#include <cstdlib>
#include <iostream>
#include <vector>

#include "dbg.hpp"

unsigned long long int x = 0x1337133713371337ULL;

void exit_handler() { dbg::kill_process(); }

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

    try {
        dbg::spawn_process(progname, args.data(), envp);
        std::atexit(exit_handler);
        unsigned long long x;
        dbg::read_memory(reinterpret_cast<void*>(0x407008), &x, sizeof(x));
        std::cout << "Read a value of: " << std::hex << x << std::dec << '\n';
        dbg::insert_breakpoint(reinterpret_cast<void*>(0x40106f));
        dbg::continue_process();
        std::cout << "Hit breakpoint. Press ENTER to continue." << std::endl;
        std::cin.get();
        dbg::continue_process();
        int exit = dbg::wait_process_exit();
        std::cout << "Got exit code " << exit << ".\n";
    } catch (const std::system_error& e) {
        std::cerr << "Got error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
