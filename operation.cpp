#include <cstdio>
#include <readline/readline.h> // if you have issues, consider installing readline-dev or readline-devel
#include <readline/history.h>
#include <string>
#include <vector>
#include <iostream>
#include <optional>
#include "operation.hpp"

// #include "elf.hpp" TODO uncomment upon ELF merging into main
#include "dbg.hpp"

/* TODO uncomment upon ELF merging into main
Operation::Operation(Tracee& tracee_arg, ELF& elf_arg)
{
    tracee = &tracee_arg;
    elf = &elf_arg;
}
    */

Operation::Operation(Tracee& tracee_arg)
{
    tracee = &tracee_arg;
    std::cout << "No ELF parsing will occur in this session. Refrain from using symbols in arguments.\n";
}

Register Operation::get_register(std::string input)
{
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
    else
    {
        perror("No valid register provided. Try again.\n");
    }
}

/* TODO uncomment upon ELF merging into main 
long Operation::get_addr(std::string arg)
{
    if (arg[0] == '*') // address
    {
        return std::stoul(arg.substr(1), NULL, 16); // drop the asterisk, convert to unsigned long
    }
    else // symbol
    {
        if (elf == NULL)
        {
            return -2;
        }

        std::optional<long> arg_ul = elf->lookup_sym(arg);
        if (arg_ul.has_value())
        {
            return arg_ul.value();
        }
        else return -1;
    }
}
    */


std::vector<std::string> Operation::get_tokenize_command()
{
    std::vector<std::string> command_arguments;
    std::string command = readline(NULL);
    int command_len = command.size();
    int begin_substring = 0;
    int substring_len = 0;
    
    for (int i = 0; i < command_len; i++) // tokenize into command_arguments
    {
        char ch = command.at(i);
        if (ch == ' ')
        {
            std::string arg = command.substr(begin_substring, substring_len);
            command_arguments.push_back(arg);
            begin_substring = i + 1;
            substring_len = 0;
        }
        else
        {
            substring_len++;
        }   
    }
    command_arguments.push_back(command.substr(begin_substring)); // push the last arg

    return command_arguments;
}

int Operation::execute_command(std::vector<std::string> arguments)
{
    std::string command = arguments.at(0);
    
    // yandere dev energy
    while (true)
    {
        if (command == "b" || command == "brk" || command == "break" || command == "breakpoint")
        {
            /* TODO uncomment upon ELF merging into main
            // determine if we are working with an address or symbol
            std::string arg1 = arguments.at(1);
            long arg1_l = Operation::get_addr(arg1);
            
            if (arg1_l == -1)
            {
                std::cout << "This appears to be an invalid symbol. Try again.\n";
                continue;
            }
            else if (arg1_l == -2)
            {
                std::cout << "Symbols are not supported at this time. Use addresses for the time being.\n";
                continue;
            }

            tracee->insert_breakpoint(arg1_l); // set breakpoint
            std::cout << "Breakpoint added at" << arg1_l << "\n";

            */
        }
        else if (command == "c" || command == "continue")
        {
            std::cout << "Continuing\n";
            return tracee->continue_process();
        }
        else if (command == "si" || command == "stepin")
        {
            std::cout << "Stepping into child\n";
            tracee->step_into();
        }
        else if (command == "rr" || command == "readreg")
        {
            Register arg1 = get_register(arguments.at(1));
            int arg2 = std::stoi(arguments.at(2));
            printf("%lu", tracee->read_register(arg1, arg2));
        }
        else if (command == "wr" || command == "writereg")
        {
            Register arg1 = get_register(arguments.at(1));
            int arg2 = stoi(arguments.at(2));
            unsigned long arg3 = std::stoul(arguments.at(3));
            tracee->write_register(arg1, arg2, arg3);
            printf("Written\n");
        }
        else if (command == "i" || command == "inj" || command == "inject")
        {
            // TODO
        }
        else if (command == "x" || command == "readmem")
        {
            // TODO
        }
        else if (command == "set" || command == "writemem")
        {
            // TODO
        }
        else
        {
            std::cout <<    "Available commands:\n" << 
                            "b/brk/break/breakpoint *HEXADDR\n" <<
                            "b/brk/break/breakpoint SYMBOL\n" <<
                            "c/continue\n" <<
                            "si/stepin\n" << 
                            "rr/readreg REG NBYTES\n" <<
                            "wr/writereg REG NBYTES VALUE\n" <<
                            "i/inj/inject ___" <<
                            "x/readmem ___" <<
                            "set/writemem ___"
                            ;
        }
    }
    
}

int Operation::parse_and_run()
{
    return execute_command(get_tokenize_command());
}