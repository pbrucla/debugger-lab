#pragma once

#include <stdint.h>

#include <optional>
#include <string>
#include <vector>

#include "dbg.hpp"

class Operation {
   public:
    Operation(Tracee& tracee) : m_tracee(tracee) {}
    void parse_and_run();

   private:
    std::optional<uint64_t> get_addr(std::string arg);
    std::optional<Register> get_register(std::string input);
    std::vector<std::string> get_tokenize_command();
    void execute_command(const std::vector<std::string>& arguments);

    Tracee& m_tracee;
};
