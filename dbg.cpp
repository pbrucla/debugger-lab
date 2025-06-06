#include "dbg.hpp"

#include <capstone/capstone.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <link.h>
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
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "elf.hpp"
#include "util.hpp"

using word = unsigned long;

Tracee::~Tracee() {
    try {
        kill_process();
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
    if (m_child_pid == NOCHILD) {
        return;
    }
    if (kill(m_child_pid, sgn) < 0 && errno != ESRCH) {
        util::throw_errno();
    }
}

void Tracee::step_into() {
    if (m_child_pid == NOCHILD) {
        std::cerr << "Cannot single step in stopped process\n";
        return;
    }
    util::throw_errno(ptrace(PTRACE_SINGLESTEP, m_child_pid, nullptr, nullptr));
    int status;
    util::throw_errno(waitpid(m_child_pid, &status, 0));
    util::throw_assert(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP);
}

void Tracee::read_memory(size_t addr, void* out, size_t sz) {
    if (m_child_pid == NOCHILD) {
        std::cerr << "Cannot read memory in stopped process\n";
        return;
    }
    const word* in_addr = reinterpret_cast<const word*>(addr);
    word* out_addr = static_cast<word*>(out);
    for (size_t i = 0; i * sizeof(word) < sz; i++) {
        errno = 0;
        if (sz - i * sizeof(word) >= sizeof(word)) {
            *out_addr = util::throw_errno(ptrace(PTRACE_PEEKDATA, m_child_pid, in_addr, nullptr));
        } else {
            word out = util::throw_errno(ptrace(PTRACE_PEEKDATA, m_child_pid, in_addr, nullptr));
            memcpy(out_addr, static_cast<void*>(&out), sz - i * sizeof(word));
        }
        out_addr++;
        in_addr++;
    }
}

void Tracee::write_memory(size_t addr, const void* data, size_t sz) {
    if (m_child_pid == NOCHILD) {
        std::cerr << "Cannot write memory in stopped process\n";
        return;
    }
    const word* in_addr = static_cast<const word*>(data);
    const word* out_addr = reinterpret_cast<const word*>(addr);
    for (size_t i = 0; i * sizeof(word) < sz; i++) {
        if (sz - i * sizeof(word) >= sizeof(word)) {
            util::throw_errno(ptrace(PTRACE_POKEDATA, m_child_pid, out_addr, *in_addr));
        } else {
            word val;
            read_memory(reinterpret_cast<size_t>(out_addr), &val, sizeof(val));
            memcpy(&val, in_addr, sz - i * sizeof(word));
            util::throw_errno(ptrace(PTRACE_POKEDATA, m_child_pid, out_addr, val));
        }
        out_addr++;
        in_addr++;
    }
}

std::string Tracee::read_string(uint64_t addr) {
    std::string ret;
    char buf[8];
    for (;; addr += sizeof(buf)) {
        read_memory(addr, buf, sizeof(buf));
        auto* end = static_cast<char*>(memchr(buf, 0, sizeof(buf)));
        ret.append(buf, end ? end : (buf + sizeof(buf)));
        if (end) {
            break;
        }
    }
    return ret;
}

void Tracee::spawn_process(char* const argv[], char* const envp[]) {
    if (m_child_pid != NOCHILD) {
        kill_process();
        wait_process_exit();
    }
    pid_t pid = util::throw_errno(fork());
    if (pid == 0) {
        // child
        int persona = personality(0xffffffffULL);
        personality(persona | ADDR_NO_RANDOMIZE);
        util::throw_errno(ptrace(PTRACE_TRACEME, 0, nullptr, nullptr));
        util::throw_errno(execve(m_pathname, argv, envp));
    } else {
        // parent
        m_child_pid = pid;
        int status;
        util::throw_errno(waitpid(m_child_pid, &status, 0));
        util::throw_assert(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP);
        post_spawn();
    }
}

int Tracee::continue_process() {
    if (m_child_pid == NOCHILD) {
        std::cerr << "Cannot continue stopped process\n";
        return 0;
    }

    int status;
    if (m_breakpoint_hit) {
        struct user_regs_struct regs;
        util::throw_errno(ptrace(PTRACE_GETREGS, m_child_pid, nullptr, &regs));
        size_t pc = regs.rip;
        auto it = m_breakpoints.find(pc);
        m_breakpoint_hit = false;
        if (it != m_breakpoints.end()) {
            step_into();
            inject_breakpoint(it->second);
        }
    }

    util::throw_errno(ptrace(PTRACE_CONT, m_child_pid, nullptr, nullptr));
    util::throw_errno(waitpid(m_child_pid, &status, 0));
    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
        struct user_regs_struct regs;
        util::throw_errno(ptrace(PTRACE_GETREGS, m_child_pid, nullptr, &regs));
        regs.rip--;
        size_t pc = regs.rip;
        if (m_breakpoints.contains(pc)) {
            // we hit a breakpoint
            printf("Hit breakpoint at %#zx\n", pc);
            m_breakpoint_hit = true;
            uninject_breakpoint(m_breakpoints.at(pc));
            util::throw_errno(ptrace(PTRACE_SETREGS, m_child_pid, nullptr, &regs));
        }
    } else if (WIFEXITED(status)) {
        printf("Process exited with code %d\n", WEXITSTATUS(status));
        m_child_pid = NOCHILD;
    } else if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        printf("Process received signal %d (%s)\n", sig, strsignal(sig));
        m_child_pid = NOCHILD;
    }
    return status;
}

void Tracee::insert_breakpoint(size_t addr) {
    if (m_breakpoints.contains(addr)) {
        return;
    }
    auto [it, _] = m_breakpoints.emplace(addr, Breakpoint{.addr = addr});
    Breakpoint& bp = it->second;
    if (m_child_pid != NOCHILD) {
        inject_breakpoint(bp);
    }
}

int Tracee::wait_process_exit() {
    if (m_child_pid == NOCHILD) {
        return 0;
    }
    while (1) {
        int status;
        util::throw_errno(waitpid(m_child_pid, &status, 0));
        if (WIFEXITED(status)) {
            m_child_pid = NOCHILD;
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            m_child_pid = NOCHILD;
            return WTERMSIG(status);
        }
    }
}

int Tracee::disassemble(int lineNumber, size_t address) {
    csh handle;
    cs_open(CS_ARCH_X86, CS_MODE_64, &handle);
    cs_insn* insn = cs_malloc(handle);
    uint8_t code[16];
    std::vector<cs_insn*> disassembledInstructions;
    // Read only a few instructions at a time, take the first one, move to the address at the beginning of the next
    // instruction

    int i = 0;
    while (i < lineNumber) {
        read_memory(reinterpret_cast<size_t>(address), &code, 16);

        size_t count = cs_disasm(handle, code, 16, address, 1, &insn);
        if (count > 0) {
            // Move to the next instruction

            cs_insn* copy = (cs_insn*)malloc(sizeof(cs_insn));
            memcpy(copy, &insn[0], sizeof(cs_insn));
            disassembledInstructions.push_back(copy);

            address += insn[0].size;
            cs_free(insn, count);

        } else {
            std::cerr << "disassemble: Could not disassemble instruction " << i + 1 << std::endl;
            cs_close(&handle);
            return -1;
        }
        i++;
    }
    cs_close(&handle);

    if (disassembledInstructions.empty()) {
        std::cerr << "disassemble: No instructions to print" << std::endl;
        return -1;
    }
    ulong base_address = disassembledInstructions[0]->address;
    for (auto instr : disassembledInstructions) {
        std::cout << std::hex << instr->address << " <+" << instr->address - base_address << ">:\t" << instr->mnemonic
                  << "\t" << instr->op_str << std::endl;
    }

    for (cs_insn* i : disassembledInstructions) {
        free(i);
    }
    return 0;
}

unsigned long Tracee::syscall(const unsigned long syscall, const std::array<unsigned long, 6>& args) {
    // read registers
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, m_child_pid, nullptr, &regs);

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
    util::throw_errno(ptrace(PTRACE_SETREGS, m_child_pid, nullptr, &syscall_regs));

    step_into();  // actually run the syscall

    // retrieve return value
    struct user_regs_struct after;
    ptrace(PTRACE_GETREGS, m_child_pid, nullptr, &after);
    unsigned long rv = after.rax;  // retvals at %rax

    write_memory(instruction_ptr_addr, &instruction, 2);  // restore instruction

    // set regs back
    util::throw_errno(ptrace(PTRACE_SETREGS, m_child_pid, nullptr, &regs));

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
    util::throw_errno(ptrace(PTRACE_GETREGS, m_child_pid, nullptr, &regs));
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
    util::throw_errno(ptrace(PTRACE_GETREGS, m_child_pid, nullptr, &regs));

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

    util::throw_errno(ptrace(PTRACE_SETREGS, m_child_pid, nullptr, &regs));
}

std::pair<uint64_t, uint64_t> Tracee::get_stackframe(uint64_t bp) {  // will only go up one layer
    uint64_t next_bp = 0;
    uint64_t return_address = 0;
    read_memory(bp, &next_bp, 8);
    read_memory(bp + 8, &return_address, 8);

    return {return_address, next_bp};
}

std::vector<int64_t> Tracee::backtrace() {
    std::vector<int64_t> addresses{read_register(Register::RIP, 8)};
    uint64_t bp = read_register(Register::RBP, 8);

    while (true) {
        auto [return_address, next_bp] = get_stackframe(bp);
        unsigned long long addr = ptrace(PTRACE_PEEKDATA, m_child_pid, next_bp, nullptr);
        if (addr == (unsigned long long)-1) {
            break;
        } else {
            addresses.push_back(return_address);
            bp = next_bp;
        }
    }
    return addresses;
}

std::optional<uint64_t> Tracee::lookup_sym(std::string_view name) const {
    std::optional<uint64_t> addr;
    if ((addr = m_elf.lookup_sym(name))) {
        return addr;
    }
    if (m_dl && (addr = m_dl->lookup_sym(name))) {
        return addr;
    }
    for (const auto& shlib : m_shlibs) {
        if ((addr = shlib.lookup_sym(name))) {
            return addr;
        }
    }
    return {};
}

std::optional<std::pair<uint64_t, uint64_t>> Tracee::find_segment(uint32_t type) {
    auto phdr_addr = m_auxv.at(AT_PHDR);
    auto phnum = m_auxv.at(AT_PHNUM);
    auto phent = m_auxv.at(AT_PHENT);
    util::throw_assert(phent == sizeof(Elf64_Phdr));
    Elf64_Phdr phdr;
    for (size_t i = 0; i < phnum; ++i) {
        read_memory(phdr_addr + i * phent, &phdr, phent);
        if (phdr.p_type == type) {
            return {{m_elf.base() + phdr.p_vaddr, phdr.p_memsz}};
        }
    }
    return {};
}

std::optional<uint64_t> Tracee::find_dynamic_entry(int64_t tag) {
    auto [dyn_addr, dyn_size] = m_dyn;
    Elf64_Dyn dyn;
    for (size_t i = 0; i < dyn_size / sizeof(dyn); ++i) {
        read_memory(dyn_addr + i * sizeof(dyn), &dyn, sizeof(dyn));
        if (dyn.d_tag == tag) {
            return dyn.d_un.d_val;
        }
    }
    return {};
}

void Tracee::post_spawn() {
    char auxv_path[256];
    snprintf(auxv_path, sizeof(auxv_path), "/proc/%d/auxv", m_child_pid);
    int auxv_fd = util::throw_errno(open(auxv_path, O_RDONLY));
    Elf64_auxv_t auxv;
    for (;;) {
        auto n = read(auxv_fd, &auxv, sizeof(auxv));
        util::throw_assert(n == sizeof(auxv));
        if (auxv.a_type == AT_NULL) {
            break;
        }
        m_auxv.emplace(auxv.a_type, auxv.a_un.a_val);
    }
    close(auxv_fd);
    auto entry = m_auxv.at(AT_ENTRY);
    m_elf.set_base_from_entry(entry);
    if (m_elf.base() != 0) {
        printf("PIE executable (base %#lx)\n", m_elf.base());

        std::unordered_map<size_t, Breakpoint> new_breakpoints;
        for (auto& [addr, bp] : m_breakpoints) {
            bp.addr += m_elf.base();
            inject_breakpoint(bp);
            new_breakpoints.emplace(m_elf.base() + addr, bp);
        }
        m_breakpoints = std::move(new_breakpoints);
    }

    if (auto interp = m_elf.interp()) {
        m_dl.emplace(interp->data(), m_auxv.at(AT_BASE));
        printf("Setting temporary breakpoint at entry point (%#lx)\n", entry);
        insert_breakpoint(entry);
        continue_process();
        m_breakpoints.erase(entry);

        m_dyn = *find_segment(PT_DYNAMIC);
        auto debug_addr = *find_dynamic_entry(DT_DEBUG);
        r_debug debug;
        read_memory(debug_addr, &debug, sizeof(debug));
        link_map lm;
        for (auto lm_addr = reinterpret_cast<uint64_t>(debug.r_map); lm_addr != 0;
             lm_addr = reinterpret_cast<uint64_t>(lm.l_next)) {
            read_memory(lm_addr, &lm, sizeof(lm));
            if (!lm.l_prev) {
                continue;
            }
            auto name = read_string(reinterpret_cast<uint64_t>(lm.l_name));
            if (name == *interp || name == "linux-vdso.so.1") {
                continue;
            }
            printf("Adding shared library %s (%#lx)\n", name.c_str(), lm.l_addr);
            m_shlibs.emplace_back(name.c_str(), lm.l_addr);
        }
    }
}
