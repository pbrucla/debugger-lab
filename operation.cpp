#include <readline/readline.h> // if you have issues, consider installing readline-dev or readline-devel
#include <readline/history.h>
#include <string>
#include <vector>
#include <iostream>

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

int Operation::execute_command(std::vector<std::string> arguments, Tracee& tracee)
{
    std::string command = arguments.at(0);
    
    // yandere dev energy
    while (true)
    {
        if (command == "b" || command == "brk" || command == "break" || command == "breakpoint")
        {
            // determine if we are working with an address or symbol
            std::string arg1 = arguments.at(1);
            unsigned long arg1_ul;
            if (arg1[0] == '*') // address
            {
                arg1_ul = std::stoul(arg1.substr(1), NULL, 16); // drop the asterisk, convert to usigned long
            }
            else // symbol
            {
                arg1_ul = ELF::lookup_sym(arg1);
                if (!arg1_ul)
                {
                    std::cout << "This appears to be an invalid symbol. Try again.\n";
                    continue;
                }
            }

            tracee.insert_breakpoint(arg1_ul); // set breakpoint
            std::cout << "Breakpoint added at" << arg1_ul << "\n";
        }
        else if (command == "clr" || command == "clear")
        {
            unsigned long arg1 = std::stoul(arguments.at(1));
            std::cout << "Breakpoint removed\n";
        }
        else if (command == "c" || command == "continue")
        {
            std::cout << "Continuing\n";
            return tracee.continue_process();
        }
        else if (command == "si" || command == "stepin")
        {
            std::cout << "Stepping into child\n";
            tracee.step_into();
        }
        else if (command == "bt" || command == "backtrace")
        {

        }
        else
        {
            std::cout <<    "Available commands:\n" << 
                            "b/brk/break/breakpoint *HEXADDR\n" <<
                            "b/brk/break/breakpint SYMBOL\n" <<
                            "clr/clear *HEXADDR\n" <<
                            "clr/clear SYMBOL\n" <<
                            "c/continue\n" <<
                            "si\n" << 
                            "bt\n"
                            ;
        }
    }
    
}

int Operation::parse_and_run(Tracee tracee)
{
    return execute_command(get_tokenize_command(), tracee);
}