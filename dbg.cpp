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

using word = unsigned long;

Breakpoint::Breakpoint(size_t addr) : addr(addr), injected(false), orig_byte(0) {}

Tracee::Tracee() {}
Tracee::~Tracee() {
    try {
        Tracee::kill_process();
    } catch (const std::system_error&) {
    }
}

void Tracee::inject_breakpoint(Breakpoint& bp) {
    if (bp.injected) {
        return;
    }
    read_memory(bp.addr, &bp.orig_byte, 1);
    uint8_t new_byte = 0xcc;
    write_memory(bp.addr, &new_byte, 1);
    bp.injected = true;
}

void Tracee::uninject_breakpoint(Breakpoint& bp) {
    if (!bp.injected) {
        return;
    }
    write_memory(bp.addr, &bp.orig_byte, 1);
    bp.injected = false;
}

void Tracee::kill_process(int sgn) {
    if (child_pid == NOCHILD) {
        return;
    }
    if (kill(child_pid, sgn) < 0 && errno != ESRCH) {
        util::throw_errno();
    }
}

void Tracee::step_into() {
    if (child_pid == NOCHILD) {
        std::cerr << "Cannot single step in stopped process\n";
        return;
    }
    util::throw_errno(ptrace(PTRACE_SINGLESTEP, child_pid, nullptr, nullptr));
    int status;
    util::throw_errno(waitpid(child_pid, &status, 0));
    assert(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP);
}

void Tracee::read_memory(size_t addr, void* out, size_t sz) {
    if (child_pid == NOCHILD) {
        std::cerr << "Cannot read memory in stopped process\n";
        return;
    }
    const word* in_addr = reinterpret_cast<const word*>(addr);
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

void Tracee::write_memory(size_t addr, const void* data, size_t sz) {
    if (child_pid == NOCHILD) {
        std::cerr << "Cannot write memory in stopped process\n";
        return;
    }
    const word* in_addr = static_cast<const word*>(data);
    const word* out_addr = reinterpret_cast<const word*>(addr);
    for (size_t i = 0; i * sizeof(word) < sz; i++) {
        if (sz - i * sizeof(word) >= sizeof(word)) {
            util::throw_errno(ptrace(PTRACE_POKEDATA, child_pid, out_addr, *in_addr));
        } else {
            word val;
            read_memory(reinterpret_cast<size_t>(out_addr), &val, sizeof(val));
            memcpy(&val, in_addr, sz - i * sizeof(word));
            util::throw_errno(ptrace(PTRACE_POKEDATA, child_pid, out_addr, val));
        }
        out_addr++;
        in_addr++;
    }
}

void Tracee::spawn_process(const char* pathname, char* const argv[], char* const envp[]) {
    if (child_pid != NOCHILD) {
        kill_process();
        wait_process_exit();
    }
    pid_t pid = util::throw_errno(fork());
    if (pid == 0) {
        // child
        util::throw_errno(ptrace(PTRACE_TRACEME, 0, nullptr, nullptr));
        util::throw_errno(execve(pathname, argv, envp));
    } else {
        // parent
        child_pid = pid;
        int status;
        util::throw_errno(waitpid(child_pid, &status, 0));
        assert(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP);
        for (auto& p : breakpoints) {
            inject_breakpoint(p.second);
        }
    }
}

int Tracee::continue_process() {
    if (child_pid == NOCHILD) {
        std::cerr << "Cannot continue stopped process\n";
        return 0;
    }

    int status;
    if (breakpoint_hit) {
        struct user_regs_struct regs;
        util::throw_errno(ptrace(PTRACE_GETREGS, child_pid, nullptr, &regs));
        size_t pc = regs.rip;
        auto it = breakpoints.find(pc);
        breakpoint_hit = false;
        if (it != breakpoints.end()) {
            step_into();
            inject_breakpoint(it->second);
        }
    }

    util::throw_errno(ptrace(PTRACE_CONT, child_pid, NULL, NULL));
    util::throw_errno(waitpid(child_pid, &status, 0));
    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
        struct user_regs_struct regs;
        util::throw_errno(ptrace(PTRACE_GETREGS, child_pid, nullptr, &regs));
        regs.rip--;
        size_t pc = regs.rip;
        if (breakpoints.count(pc) > 0) {
            // we hit a breakpoint
            breakpoint_hit = true;
            uninject_breakpoint(breakpoints.at(pc));
            util::throw_errno(ptrace(PTRACE_SETREGS, child_pid, nullptr, &regs));
        }
    }
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        std::cerr << "process exited\n";
        child_pid = NOCHILD;
    }
    return status;
}

void Tracee::insert_breakpoint(size_t addr) {
    if (breakpoints.count(addr) > 0) {
        return;
    }
    auto [it, _] = breakpoints.emplace(addr, addr);
    Breakpoint& bp = it->second;
    if (child_pid != NOCHILD) {
        inject_breakpoint(bp);
    }
}

int Tracee::wait_process_exit() {
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
