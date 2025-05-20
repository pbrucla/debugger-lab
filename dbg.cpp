#include "dbg.hpp"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/personality.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
        int persona = personality(0xffffffffULL);
        personality(persona | ADDR_NO_RANDOMIZE);
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

unsigned long Tracee::syscall(const unsigned long syscall, const std::array<unsigned long, 6>& args) {
    // read registers
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, child_pid, nullptr, &regs);

    // inject syscall & store prev instr at addr
    unsigned long instruction_ptr_addr = regs.rip & ~0xfff;  // idk why im page aligning tbh
    int syscall_code = 0x050f;

    int instruction = 0;  // old value at memory
    read_memory(instruction_ptr_addr, &instruction, 2);
    write_memory(instruction_ptr_addr, &syscall_code, 2);  // overwrite to be syscall

    // set regs (INTERFACE UNAVAILABLE)
    struct user_regs_struct syscall_regs = regs;
    syscall_regs.rax = syscall;
    syscall_regs.rdi = args[0];
    syscall_regs.rsi = args[1];
    syscall_regs.rdx = args[2];
    syscall_regs.r10 = args[3];
    syscall_regs.r8 = args[4];
    syscall_regs.r9 = args[5];
    syscall_regs.rip = instruction_ptr_addr;
    util::throw_errno(ptrace(PTRACE_SETREGS, child_pid, nullptr, &syscall_regs));

    step_into();  // actually run the syscall

    // retrieve return value
    struct user_regs_struct after;
    ptrace(PTRACE_GETREGS, child_pid, nullptr, &after);
    unsigned long rv = after.rax;  // retvals at %rax

    write_memory(instruction_ptr_addr, &instruction, 2);  // restore instruction

    // set regs back
    util::throw_errno(ptrace(PTRACE_SETREGS, child_pid, nullptr, &regs));

    return rv;
}
unsigned long long& get_register_ref(user_regs_struct& regs, Register reg) {
    switch (reg) {
        case R15:
            return regs.r15;
        case R14:
            return regs.r14;
        case R13:
            return regs.r13;
        case R12:
            return regs.r12;
        case RBP:
            return regs.rbp;
        case RBX:
            return regs.rbx;
        case R11:
            return regs.r11;
        case R10:
            return regs.r10;
        case R9:
            return regs.r9;
        case R8:
            return regs.r8;
        case RAX:
            return regs.rax;
        case RCX:
            return regs.rcx;
        case RDX:
            return regs.rdx;
        case RSI:
            return regs.rsi;
        case RDI:
            return regs.rdi;
        case ORIG_RAX:
            return regs.orig_rax;
        case RIP:
            return regs.rip;
        case CS:
            return regs.cs;
        case EFLAGS:
            return regs.eflags;
        case RSP:
            return regs.rsp;
        case SS:
            return regs.ss;
        case FS_BASE:
            return regs.fs_base;
        case GS_BASE:
            return regs.gs_base;
        case DS:
            return regs.ds;
        case ES:
            return regs.es;
        case FS:
            return regs.fs;
        case GS:
            return regs.gs;
        default:
            throw std::runtime_error("Failed to get register ref");
    }
}

uint64_t Tracee::read_register(Register reg, int size) {
    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, child_pid, nullptr, &regs) == -1) {
        perror("ptrace(PTRACE_GETREGS)");
        throw std::runtime_error("Failed to get registers");
    }
    unsigned long long& value = get_register_ref(regs, reg);

    switch (size) {  // size is in bytes
        case 1:      // 1 byte
            return value & 0xFF;
        case 2:  // 2 bytes
            return value & 0xFFFF;
        case 4:  // 4 bytes
            return value & 0xFFFFFFFF;
        case 8:  // 8 bytes (full register)
            return value;
        default:
            throw std::runtime_error("Unsupported register size");
    }
}

void Tracee::write_register(Register reg, int size, uint64_t value) {
    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, child_pid, nullptr, &regs) == -1) {
        perror("ptrace(PTRACE_GETREGS)");
        throw std::runtime_error("Failed to get registers");
    }

    unsigned long long& full_register = get_register_ref(regs, reg);

    switch (size) {  // size is in bytes
        case 1:      // 1 byte
            full_register = (full_register & ~0xFF) | (value & 0xFF);
            break;
        case 2:  // 2 bytes
            full_register = (full_register & ~0xFFFF) | (value & 0xFFFF);
            break;
        case 4:  // 4 bytes
            full_register = (full_register & ~0xFFFFFFFF) | (value & 0xFFFFFFFF);
            break;
        case 8:  // 8 bytes (full register)
            full_register = value;
            break;
        default:
            throw std::runtime_error("Unsupported register size");
    }

    if (ptrace(PTRACE_SETREGS, child_pid, nullptr, &regs) == -1) {
        perror("ptrace(PTRACE_SETREGS)");
        throw std::runtime_error("Failed to set registers");
    }
}

std::pair<uint64_t, uint64_t> Tracee::get_stackframe(uint64_t bp) {  // will only go up one layer
    uint64_t next_bp = 0;
    uint64_t return_address = 0;
    read_memory(bp, &next_bp, 8);
    read_memory(bp + 8, &return_address, 8);

    return {return_address, next_bp};
}

std::vector<int64_t> Tracee::backtrace() {
    std::vector<int64_t> addresses;
    uint64_t bp = read_register(Register::RBP, 8);

    while (true) {
        auto [return_address, next_bp] = get_stackframe(bp);
        unsigned long long addr = ptrace(PTRACE_PEEKDATA, child_pid, next_bp, nullptr);
        if (addr == (unsigned long long)-1) {
            break;
        } else {
            addresses.push_back(return_address);
            bp = next_bp;
        }
    }
    return addresses;
}