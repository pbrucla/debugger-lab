#pragma once

#include <signal.h>
#include <stdint.h>

#include <unordered_map>
#include <unordered_set>

#include <string>

class Breakpoint {
   public:
    size_t addr;
    bool injected;
    uint8_t orig_byte;

    Breakpoint(size_t addr);
};

class Tracee {
    static constexpr int NOCHILD = -1;

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
    
    uint64_t read_register(Register reg, int size);

    void write_register(Register reg, int size, uint64_t value);
};

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


Register string_to_register(const std::string& name) {
    static const std::unordered_map<std::string, Register> reg_map = {
        {"r15", R15},
        {"r14", R14},
        {"r13", R13},
        {"r12", R12},
        {"rbp", RBP},
        {"rbx", RBX},
        {"r11", R11},
        {"r10", R10},
        {"r9", R9},
        {"r8", R8},
        {"rax", RAX},
        {"rcx", RCX},
        {"rdx", RDX},
        {"rsi", RSI},
        {"rdi", RDI},
        {"orig_rax", ORIG_RAX},
        {"rip", RIP},
        {"cs", CS},
        {"eflags", EFLAGS},
        {"rsp", RSP},
        {"ss", SS},
        {"fs_base", FS_BASE},
        {"gs_base", GS_BASE},
        {"ds", DS},
        {"es", ES},
        {"fs", FS},
        {"gs", GS}
    };

    auto it = reg_map.find(name);
    if (it == reg_map.end()) {
        throw std::invalid_argument("Unknown register name: " + name);
    }
    return it->second;
}
