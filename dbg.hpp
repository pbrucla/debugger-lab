#pragma once

#include <signal.h>
#include <stdint.h>

namespace dbg {
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
void read_memory(const void* addr, void* out, size_t sz);
// Writes `sz` bytes from address `data` in the current process to address `addr` in the child process.
void write_memory(const void* addr, const void* data, size_t sz);
// Inserts a breakpoint at address `addr` in the child process.
void insert_breakpoint(const void* addr);
}  // namespace dbg
