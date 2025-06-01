#include "operation.hpp"

#include <ctype.h>
#include <readline/history.h>
#include <readline/readline.h>  // if you have issues, consider installing readline-dev or readline-devel
#include <stdint.h>
#include <stdio.h>

#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "dbg.hpp"
#include "elf.hpp"

std::optional<Register> Operation::get_register(std::string input) {
    if (input == "r15" || input == "R15")
        return R15;
    else if (input == "r14" || input == "R14")
        return R14;
    else if (input == "r13" || input == "R13")
        return R13;
    else if (input == "r12" || input == "R12")
        return R12;
    else if (input == "r11" || input == "R11")
        return R11;
    else if (input == "r10" || input == "R10")
        return R10;
    else if (input == "r9" || input == "R9")
        return R9;
    else if (input == "r8" || input == "R8")
        return R8;
    else if (input == "rbp" || input == "RBP")
        return RBP;
    else if (input == "rbx" || input == "RBX")
        return RBX;
    else if (input == "rax" || input == "RAX")
        return RAX;
    else if (input == "rcx" || input == "RCX")
        return RCX;
    else if (input == "rdx" || input == "RDX")
        return RDX;
    else if (input == "rsi" || input == "RSI")
        return RSI;
    else if (input == "rdi" || input == "RDI")
        return RDI;
    else if (input == "rip" || input == "RIP")
        return RIP;
    else if (input == "rsp" || input == "RSP")
        return RSP;
    else if (input == "orig_rax" || input == "ORIG_RAX")
        return ORIG_RAX;
    else if (input == "cs" || input == "CS")
        return CS;
    else if (input == "eflags" || input == "EFLAGS")
        return EFLAGS;
    else if (input == "ss" || input == "SS")
        return SS;
    else if (input == "ds" || input == "DS")
        return DS;
    else if (input == "es" || input == "ES")
        return ES;
    else if (input == "fs" || input == "FS")
        return FS;
    else if (input == "gs" || input == "GS")
        return GS;
    else if (input == "fs_base" || input == "FS_BASE")
        return FS_BASE;
    else if (input == "gs_base" || input == "GS_BASE")
        return GS_BASE;
    else {
        printf("Invalid register `%s`\n", input.c_str());
        return {};
    }
}

std::optional<uint64_t> Operation::get_addr(std::string arg) {
    if (isdigit(arg[0])) {
        return std::stoul(arg, nullptr, 16);
    }
    auto addr = m_tracee.elf().lookup_sym(arg);
    if (addr) {
        return addr.value();
    }
    printf("Undefined symbol `%s`\n", arg.c_str());
    return {};
}

std::vector<std::string> Operation::get_tokenize_command() {
    std::vector<std::string> command_arguments;
    std::string command = readline("cydbg> ");
    int command_len = command.size();
    int begin_substring = 0;
    int substring_len = 0;

    for (int i = 0; i < command_len; i++)  // tokenize into command_arguments
    {
        char ch = command.at(i);
        if (ch == ' ') {
            std::string arg = command.substr(begin_substring, substring_len);
            command_arguments.push_back(arg);
            begin_substring = i + 1;
            substring_len = 0;
        } else {
            substring_len++;
        }
    }
    command_arguments.push_back(command.substr(begin_substring));  // push the last arg

    return command_arguments;
}

void Operation::execute_command(const std::vector<std::string>& arguments) {
    std::string command = arguments.at(0);

    if (command == "b" || command == "brk" || command == "break" || command == "breakpoint") {
        auto addr = get_addr(arguments.at(1));
        if (addr) {
            m_tracee.insert_breakpoint(addr.value());
            printf("Breakpoint added at %#lx\n", addr.value());
        }
    } else if (command == "bt" || command == "backtrace") {
        std::vector<long> result = m_tracee.backtrace();
        std::cout << "Backtrace:\n";
        for (unsigned long i = 0; i < result.size(); i++) {
            auto addr = result.at(i);
            auto name = m_tracee.elf().lookup_addr(addr);

            std::cout << "#" << i << ": 0x" << std::hex << result.at(i) << std::dec;
            if (name.has_value()) {
                std::cout << " (" << *name << ")";
            }
            std::cout << '\n';
        }
    } else if (command == "si" || command == "stepin") {
        std::cout << "Stepping into child\n";
        m_tracee.step_into();
    } else if (command == "rr" || command == "readreg") {
        auto reg_name = arguments.at(1);
        auto reg = get_register(reg_name);
        if (reg) {
            printf("%s = %#lx\n", reg_name.c_str(), m_tracee.read_register(reg.value(), 8));
        }
    } else if (command == "wr" || command == "writereg") {
        auto reg_name = arguments.at(1);
        auto reg = get_register(reg_name);
        auto width = std::stoi(arguments.at(2));
        auto value = std::stoul(arguments.at(3));
        if (reg) {
            m_tracee.write_register(reg.value(), width, value);
            printf("Written to %s\n", reg_name.c_str());
        }
    } else if (command == "i" || command == "inj" || command == "inject") {
        // TODO
    } else if (command == "x" || command == "readmem") {
        auto addr = get_addr(arguments.at(1));
        auto size = std::stoul(arguments.at(2));
        if (addr) {
            unsigned long output;
            m_tracee.read_memory(addr.value(), &output, size);
            printf("%#lx: %#lx\n", addr.value(), output);
        }
    } else if (command == "set" || command == "writemem") {
        auto addr = get_addr(arguments.at(1));
        auto size = std::stoul(arguments.at(2));
        auto value = std::stoul(arguments.at(3));

        if (addr) {
            m_tracee.write_memory(addr.value(), &value, size);
            printf("Written to %#lx\n", addr.value());
        }
    } else if (command == "c" || command == "continue") {
        puts("Continuing");
        m_tracee.continue_process();
    } else {
        std::cout << "Available commands:\n"
                  << "bt/backtrace\n"
                  << "b/brk/break/breakpoint *0xHEXADDR\n"
                  << "b/brk/break/breakpoint SYMBOL\n"
                  << "c/continue\n"
                  << "si/stepin\n"
                  << "rr/readreg REG\n"
                  << "wr/writereg REG NBYTES VALUE\n"
                  << "i/inj/inject ___"
                  << "x/readmem *0xHEXADDR SIZE\n"
                  << "x/readmem SYMBOL SIZE\n"
                  << "set/writemem *0xHEXADDR SIZE VALUE\n"
                  << "set/writemem SYMBOL SIZE VALUE\n";
    }
}

void Operation::parse_and_run() {
    auto command = get_tokenize_command();
    execute_command(command);
}
