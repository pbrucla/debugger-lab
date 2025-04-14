#include "dbg.hpp"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "util.hpp"

static constexpr int NOCHILD = -1;
using word = unsigned long;

static pid_t child_pid = NOCHILD;
static std::unordered_set<const void*> breakpoints;
static std::unordered_map<const void*, uint8_t> breakpoint_bytes;

namespace dbg {
int wait_process_exit() {
    if (child_pid == NOCHILD) {
        return 0;
    }
    while (1) {
        int status;
        util::throw_errno(waitpid(child_pid, &status, 0));
        if (WIFEXITED(status)) {
            child_pid = NOCHILD;
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            child_pid = NOCHILD;
            return WTERMSIG(status);
        }
    }
}

void kill_process(int sgn) {
    if (child_pid == NOCHILD) {
        return;
    }
    if (kill(child_pid, sgn) < 0 && errno != ESRCH) {
        util::throw_errno();
    }
}

void step_into() {
    if (child_pid == NOCHILD) {
        std::cerr << "Cannot single step in stopped process\n";
        return;
    }
    util::throw_errno(ptrace(PTRACE_SINGLESTEP, child_pid, nullptr, nullptr));
}

void read_memory(const void* addr, void* out, size_t sz) {
    if (child_pid == NOCHILD) {
        std::cerr << "Cannot read memory in stopped process\n";
        return;
    }
    const word* in_addr = static_cast<const word*>(addr);
    word* out_addr = static_cast<word*>(out);
    for (size_t i = 0; i * sizeof(word) < sz; i++) {
        errno = 0;
        if (sz - i * sizeof(word) >= sizeof(word)) {
            *out_addr = util::throw_errno(ptrace(PTRACE_PEEKDATA, child_pid, in_addr, nullptr));
        } else {
            word out = util::throw_errno(ptrace(PTRACE_PEEKDATA, child_pid, in_addr, nullptr));
            memcpy(out_addr, static_cast<void*>(&out), sz - i * sizeof(word));
        }
        out_addr++;
        in_addr++;
    }
}

void write_memory(const void* addr, const void* data, size_t sz) {
    if (child_pid == NOCHILD) {
        std::cerr << "Cannot write memory in stopped process\n";
        return;
    }
    const word* in_addr = static_cast<const word*>(data);
    const word* out_addr = static_cast<const word*>(addr);
    for (size_t i = 0; i * sizeof(word) < sz; i++) {
        if (sz - i * sizeof(word) >= sizeof(word)) {
            util::throw_errno(ptrace(PTRACE_POKEDATA, child_pid, out_addr, *in_addr));
        } else {
            word val;
            read_memory(out_addr, &val, sizeof(val));
            memcpy(&val, in_addr, sz - i * sizeof(word));
            util::throw_errno(ptrace(PTRACE_POKEDATA, child_pid, out_addr, val));
        }
        out_addr++;
        in_addr++;
    }
}

// Injects a breakpoint into a running child process.
static void inject_breakpoint(const void* addr) {
    // don't inject breakpoint if it's already injected
    if (breakpoint_bytes.count(addr) > 0) {
        return;
    }
    // TODO: inject the breakpoint into address addr, and store the original memory value in breakpoint_bytes
}

void spawn_process(const char* pathname, char* const argv[], char* const envp[]) {
    if (child_pid != NOCHILD) {
        kill_process();
        wait_process_exit();
    }
    pid_t pid = util::throw_errno(fork());
    if (pid == 0) {
        // child
        // TODO: start tracing and then execve the new process
    } else {
        // parent
        child_pid = pid;
        // TODO: wait for the child's SIGTRAP to signify the execve having run
        for (const void* addr : breakpoints) {
            inject_breakpoint(addr);
        }
    }
}

int continue_process() {
    if (child_pid == NOCHILD) {
        std::cerr << "Cannot continue stopped process\n";
        return 0;
    }
    int status;
    // TODO: continue the child process, and wait for it to change state, storing the status in status
    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
        struct user_regs_struct regs;
        util::throw_errno(ptrace(PTRACE_GETREGS, child_pid, nullptr, &regs));
        regs.rip--;
        void* pc = reinterpret_cast<void*>(regs.rip);
        if (breakpoints.count(pc) > 0) {
            // we hit a breakpoint
            // TODO: restore the original memory at address pc
            breakpoint_bytes.erase(pc);
            util::throw_errno(ptrace(PTRACE_SETREGS, child_pid, nullptr, &regs));
        }
    }
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        child_pid = NOCHILD;
    }
    return status;
}

void insert_breakpoint(const void* addr) {
    breakpoints.insert(addr);
    if (child_pid != NOCHILD) {
        inject_breakpoint(addr);
    }
}
}  // namespace dbg
