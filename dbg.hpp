#pragma once

#include <signal.h>
#include <stdint.h>

#include <unordered_map>
#include <unordered_set>

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


    /* Syscall insertion (clobber rcx, r11)
    args[0] = %rdi
    args[1] = %rsi
    args[2] = %rdx
    args[3] = %r10
    args[4] = %r8
    args[5] = %r9
    
    MUST PASS 6 UNSIGNED LONGS - FILL EXTRA REGISTERS WITH JUNK VALUES IF NECESSARY
    */
    void syscall(const int syscall, const std::array<unsigned long,6>& args);
};
