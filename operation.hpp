#ifndef OPERATION_GRD
#define OPERATION_GRD
#include <readline/readline.h> // if you have issues, consider installing readline-dev or readline-devel
#include <readline/history.h>
#include <string>
#include <vector>
#include <iostream>

#include "elf.hpp"
#include "dbg.hpp"

class Operation {
    unsigned long get_addr(std::string arg);
    std::vector<std::string> get_tokenize_command();
    int execute_command(std::vector<std::string> arguments, Tracee& tracee);

    public:
    int parse_and_run(Tracee tracee);
};
#endif