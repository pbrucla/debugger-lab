#ifndef OPERATION_GRD
#define OPERATION_GRD
#include <readline/readline.h> // if you have issues, consider installing readline-dev or readline-devel
#include <readline/history.h>
#include <string>
#include <vector>
#include <iostream>
#include <optional>

#include "elf.hpp"
#include "dbg.hpp"

class Operation {
    ELF* elf;
    Tracee* tracee;
    long get_addr(std::string arg);
    std::vector<std::string> get_tokenize_command();
    int execute_command(std::vector<std::string> arguments);

    public:
        Operation(Tracee& tracee, ELF& elf);
        int parse_and_run();
};
#endif