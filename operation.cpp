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
        else if (command == "clr" || command == "clear")
        {
            // TODO
            std::cout << "Breakpoint removed\n";
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
        else if (command == "bt" || command == "backtrace")
        {
            // TODO
        }
        else
        {
            std::cout <<    "Available commands:\n" << 
                            "b/brk/break/breakpoint *HEXADDR\n" <<
                            "b/brk/break/breakpint SYMBOL\n" <<
                            "clr/clear *HEXADDR\n" <<
                            "clr/clear SYMBOL\n" <<
                            "c/continue\n" <<
                            "si/stepin\n" << 
                            "bt/backtrace\n"
                            ;
        }
    }
    
}

int Operation::parse_and_run()
{
    return execute_command(get_tokenize_command());
}