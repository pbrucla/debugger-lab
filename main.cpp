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
        std::cout << "Hit breakpoint. Press ENTER to continue." << std::endl;
        std::cin.get();
        std::cout << "Going to try to syscall" << std::endl;
        std::string str = "Hi my name is Ebony Dark’ness Dementia Raven Way and I have long ebony black hair (that’s how I got my name) with purple streaks and red tips that reaches my mid-back and icy blue eyes like limpid tears and a lot of people tell me I look like Amy Lee (AN: if u don’t know who she is get da hell out of here!).\n";
        unsigned long len = str.size();
        const char* cstr = str.c_str();
        //mmap(null, 0x1000,3,0, -1,0)
        unsigned long f = proc.syscall(0x09, {0,0x1000,3,0x22,(unsigned long)0,0});
        proc.write_memory(f, cstr, len);
        unsigned long k = proc.syscall(0x01, {1, f, len, 0,0,0});
        // std::cout << f << std::endl;
        // std::cout << k << std::endl;
        proc.continue_process();
        int exit = proc.wait_process_exit();
        std::cout << "Got exit code " << exit << ".\n";
    } catch (const std::system_error& e) {
        std::cerr << "Got error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
