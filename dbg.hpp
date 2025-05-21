#pragma once

#include <capstone/capstone.h>
#include <signal.h>
#include <stdint.h>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

enum Register {
    R15,
    R14,
    R13,
    R12,
    RBP,
    RBX,
    R11,
    R10,
    R9,
    R8,
    RAX,
    RCX,
    RDX,
    RSI,
    RDI,
    ORIG_RAX,
    RIP,
    CS,
    EFLAGS,
    RSP,
    SS,
    FS_BASE,
    GS_BASE,
    DS,
    ES,
    FS,
    GS,
};
#include <array>

class Breakpoint {
   public:
    size_t addr;
    bool injected;
    uint8_t orig_byte;

    Breakpoint(size_t addr);
};

class Tracee {
    static constexpr int NOCHILD = -1;
    bool breakpoint_hit = false;

    pid_t child_pid = NOCHILD;
    std::unordered_map<size_t, Breakpoint> breakpoints;

    // Injects a breakpoint into a running child process.
    void inject_breakpoint(Breakpoint& bp);
    // Uninjects a breakpoint from a running child process.
    void uninject_breakpoint(Breakpoint& bp);

   public:
    Tracee();
    Tracee(const Tracee& other) = delete;
    Tracee& operator=(const Tracee& other) = delete;
    ~Tracee();
    // Waits for the child process to exit.
    int wait_process_exit();
    // Continues executing the child process.
    int continue_process();
    // Sends a signal to the child process.
    void kill_process(int sgn = SIGKILL);
    // Spawns a new child process given the same arguments as execve().
    void spawn_process(const char* pathname, char* const argv[], char* const envp[]);
    // Single steps the child process.
    void step_into();
    // Reads `sz` bytes at address `addr` in the child process to address `out` in the current process.
    void read_memory(size_t addr, void* out, size_t sz);
    // Writes `sz` bytes from address `data` in the current process to address `addr` in the child process.
    void write_memory(size_t addr, const void* data, size_t sz);
    // Inserts a breakpoint at address `addr` in the child process.
    void insert_breakpoint(size_t addr);
<<<<<<< HEAD
    // Prints out a number of disassembled instructions starting from address
    int disassemble(int lineNumber, size_t address);
=======

    uint64_t read_register(Register reg, int size);

    void write_register(Register reg, int size, uint64_t value);

    // Gets the current stack frame, returning a (return address, parent frame pointer) base.
    std::pair<uint64_t, uint64_t> get_stackframe(uint64_t bp);

    std::vector<int64_t> backtrace();

    unsigned long syscall(const unsigned long syscall, const std::array<unsigned long, 6>& args);
>>>>>>> main
};
