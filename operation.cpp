#include <cstdio>
#include <readline/readline.h> // if you have issues, consider installing readline-dev or readline-devel
#include <readline/history.h>
#include <string>
#include <vector>
#include <iostream>
#include <optional>
#include "operation.hpp"

#include "elf.hpp"
#include "dbg.hpp"

Operation::Operation(Tracee& tracee_arg, ELF& elf_arg)
{
    tracee = &tracee_arg;
    elf = &elf_arg;
}

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
        throw -1;
    }
}

long Operation::get_addr(std::string arg)
{
    if (isdigit(arg[0])) // address
    {
        
        return std::stoul(arg, NULL, 16); // drop the asterisk, convert to unsigned long
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


std::vector<std::string> Operation::get_tokenize_command()
{
    std::vector<std::string> command_arguments;
    std::string command = readline("cydbg> ");
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
    
    if (command == "b" || command == "brk" || command == "break" || command == "breakpoint")
    {
        std::string arg1 = arguments.at(1);
        long arg1_l = Operation::get_addr(arg1);
        // determine if we are working with an address or symbol
        
        
        if (arg1_l == -1)
        {
            std::cout << "This appears to be an invalid symbol. Try again.\n";
            return -1;
        }
        else if (arg1_l == -2)
        {
            std::cout << "Symbols are not supported at this time. Use addresses for the time being.\n";
            return -1;
        }

        tracee->insert_breakpoint(arg1_l); // set breakpoint
        std::cout << "Breakpoint added at" << arg1_l << "\n";
        return 0;

    }
    else if (command == "bt" || command == "backtrace")
    {
        std::vector<long> result = tracee->backtrace();
        std::cout << "Backtrace:\n";
        for (int i = 0; i < result.size(); i++)
        {
            std::cout << result.at(i) << "\n";
        }
        std::cout << "End backtrace\n";
        return 0;
    }
    else if (command == "si" || command == "stepin")
    {
        std::cout << "Stepping into child\n";
        tracee->step_into();
        return 0;
    }
    else if (command == "rr" || command == "readreg")
    {
        Register arg1 = get_register(arguments.at(1));
        printf("%#lx", tracee->read_register(arg1, 8));
        return 0;
    }
    else if (command == "wr" || command == "writereg")
    {
        Register arg1 = get_register(arguments.at(1));
        unsigned long arg2 = std::stoul(arguments.at(2));
        tracee->write_register(arg1, 8, arg2);
        printf("Written\n");
        return 0;
    }
    else if (command == "i" || command == "inj" || command == "inject")
    {
        // i/inj/inject SYSCALL_NUM VALUE
        // the user must provide all 6 syscall arguments
        unsigned long syscall_num = std::stoul(arguments.at(1));

        std::array<unsigned long, 6> syscall_args;

        if (arguments.size() < 8) // not enough args
        {
            std::cout << "You don't have enough arguments. Provide a syscall value and 6 arguments.\n For more, hit ENTER.\n";
            return -1; 
        }

        for (int i = 0; i < 6; i++)
        {
            syscall_args[i] = std::stoul(arguments[i + 2]);
        }

        unsigned long retval = tracee->syscall(syscall_num, syscall_args);
        printf("%#lx", retval);
        return 0;
    }
    else if (command == "x" || command == "readmem")
    {
        std::string arg1 = arguments.at(1);
        long arg1_l = Operation::get_addr(arg1);
        long arg2_l = std::stoul(arguments.at(2));
        unsigned long output;
        tracee->read_memory(arg1_l, &output, arg2_l);
        printf("%#lx", output);
        return 0;
    }
    else if (command == "set" || command == "writemem")
    {
        std::string arg1 = arguments.at(1);
        long arg1_l = Operation::get_addr(arg1);
        long arg2_l = std::stoul(arguments.at(2));
        long arg3_l = std::stoul(arguments.at(3));

        tracee->write_memory(arg1_l, &arg3_l, arg2_l);

        std::cout << "Written\n";
        return 0;
    }
    else
    {
        std::cout <<    "Available commands:\n" << 
                        "bt/backtrace\n" <<
                        "b/brk/break/breakpoint 0xHEXADDR\n" <<
                        "b/brk/break/breakpoint SYMBOL\n" <<
                        "c/continue\n" <<
                        "si/stepin\n" << 
                        "rr/readreg REG\n" <<
                        "wr/writereg REG DATA\n" <<
                        "i/inj/inject SYSCALL_NUM RDI RSI RDX R10 R8 R9\n" <<
                        "x/readmem 0xHEXADDR SIZE\n" <<
                        "x/readmem SYMBOL SIZE\n" <<
                        "set/writemem 0xHEXADDR SIZE DATA\n" <<
                        "set/writemem SYMBOL SIZE DATA\n";
        return 0;
    }
}

int Operation::parse_and_run()
{
    while (true)
    {
        std::vector<std::string> command = get_tokenize_command();

        if (command.at(0) == "c" || command.at(0) == "continue") // break things off if we want to continue
        {
            break;
        }
        else // the user wants to do something
        {
            try
            {
                execute_command(command);
                std::cout << "\n";
            }
            catch(...)
            {
                std::cout << "Something went wrong. Reevaluate your commands, and try again.\nFor help, hit ENTER.\n";
            }
        }
    }
    std::cout << "Continuing\n";
    return 0;
}