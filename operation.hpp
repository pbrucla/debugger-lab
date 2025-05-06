#ifndef OPERATION_GRD
#define OPERATION_GRD
#include <cstdio>
#include <readline/readline.h> // if you have issues, consider installing readline-dev or readline-devel
#include <readline/history.h>
#include <string>
#include <vector>
#include <iostream>
#include <optional>

#include "elf.hpp"
#include "dbg.hpp"

class Operation {
    public:
        Operation(Tracee& tracee_arg, ELF& elf_arg);
        Operation(Tracee& tracee_arg);
        int parse_and_run();

    private:
        ELF* elf;
        Tracee* tracee = NULL;
        long get_addr(std::string arg);
        std::vector<std::string> get_tokenize_command();
        int execute_command(std::vector<std::string> arguments);
};

#endif