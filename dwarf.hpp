#pragma once

#include <stdint.h>

#include <unordered_map>

namespace DWARF {
struct CIE {
    uint64_t code_alignment_factor;
    int64_t data_alignment_factor;
    uint64_t return_address_register;
    const uint8_t* initial_insns;
    uint8_t encoding;
    bool has_z_augmentation;
};

void parse_eh_frame_entry(const uint8_t*& p, const uint8_t* section_start, uint64_t section_vaddr,
                          std::unordered_map<uint64_t, CIE>& cie_map, bool only_cie = false);
}  // namespace DWARF
