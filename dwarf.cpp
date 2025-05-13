#include "dwarf.hpp"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <unordered_map>

#include "dwarf2.h"
#include "util.hpp"

namespace {
int64_t read_leb128(const uint8_t*& p) {
    uint64_t result = 0;
    size_t shift = 0;
    uint8_t byte;
    do {
        byte = *p++;
        result |= static_cast<uint64_t>(byte & 0x7f) << shift;
        shift += 7;
    } while (byte & 0x80);
    if ((shift < sizeof(result) * 8) && (byte & 0x40)) result |= -(1UL << shift);
    return result;
}

uint64_t read_uleb128(const uint8_t*& p) {
    uint64_t result = 0;
    size_t shift = 0;
    uint8_t byte;
    do {
        byte = *p++;
        result |= static_cast<uint64_t>(byte & 0x7f) << shift;
        shift += 7;
    } while (byte & 0x80);
    return result;
}

template <typename T>
T read(const uint8_t*& p) {
    T result;
    memcpy(&result, p, sizeof(T));
    p += sizeof(T);
    return result;
}

uint64_t read_length(const uint8_t*& p, bool& is_dwarf64) {
    uint64_t len = read<uint32_t>(p);
    if (len == 0xffffffff) {
        len = read<uint64_t>(p);
        is_dwarf64 = true;
    } else {
        is_dwarf64 = false;
    }
    return len;
}

uint64_t read_encoded_value(const uint8_t*& p, const uint8_t* section_start, uint64_t section_vaddr, uint8_t encoding) {
    uint64_t base;
    switch (encoding & 0x70) {
        case DW_EH_PE_absptr:
            base = 0;
            break;
        case DW_EH_PE_pcrel:
            base = section_vaddr + (p - section_start);
            break;
        default:
            util::throw_assert(false, "unsupported encoding");
    }
    util::throw_assert((encoding & 0x7) != 0);
    switch (encoding & 0xf) {
        case DW_EH_PE_udata4:
            return base + read<uint32_t>(p);
        case DW_EH_PE_sdata4:
            return base + read<int32_t>(p);
        default:
            util::throw_assert(false, "unsupported encoding");
            __builtin_unreachable();
    }
}
}  // namespace

namespace DWARF {
void parse_eh_frame_entry(const uint8_t*& p, const uint8_t* section_start, uint64_t section_vaddr,
                          std::unordered_map<uint64_t, CIE>& cie_map, bool only_cie) {
    const auto* start = p;
    bool is_dwarf64;
    auto len = read_length(p, is_dwarf64);
    if (len == 0) return;
    const auto* end = p + len;
    uint64_t cie_ptr = is_dwarf64 ? read<uint64_t>(p) : read<uint32_t>(p);
    if (cie_ptr == 0) {
        auto cie_offset = start - section_start;
        if (cie_map.contains(cie_offset)) {
            p = end;
            return;
        }
        CIE cie{};
        auto version = *p++;
        util::throw_assert(version == 1, "unsupported CIE version");
        const auto* augmentation = reinterpret_cast<const char*>(p);
        p += strlen(augmentation) + 1;
        cie.code_alignment_factor = read_uleb128(p);
        cie.data_alignment_factor = read_leb128(p);
        cie.return_address_register = *p++;
        cie.has_z_augmentation = *augmentation == 'z';
        if (cie.has_z_augmentation) {
            auto n = read_uleb128(p);
            cie.initial_insns = p + n;
            ++augmentation;
        }
        for (; *augmentation != 0; ++augmentation) {
            if (*augmentation == 'L')
                ++p;
            else if (*augmentation == 'R')
                cie.encoding = *p++;
            else if (*augmentation == 'P' || *augmentation == 'S')
                util::throw_assert(false, "not implemented");
            else
                break;
        }
        if (!cie.initial_insns) cie.initial_insns = p;
        printf("CIE: code align %lu, data align %ld, ret addr reg %lu, encoding %hhu\n", cie.code_alignment_factor,
               cie.data_alignment_factor, cie.return_address_register, cie.encoding);
        cie_map.emplace(cie_offset, cie);
    } else {
        util::throw_assert(!only_cie, "unexpected FDE");
        auto cie_offset = p - (is_dwarf64 ? sizeof(uint64_t) : sizeof(uint32_t)) - section_start - cie_ptr;
        auto it = cie_map.find(cie_offset);
        if (it == cie_map.end()) {
            const auto* new_cie = section_start + cie_offset;
            parse_eh_frame_entry(new_cie, section_start, section_vaddr, cie_map, true);
            it = cie_map.find(cie_offset);
        }
        const auto& cie = it->second;
        auto initial_addr = read_encoded_value(p, section_start, section_vaddr, cie.encoding);
        auto range = read_encoded_value(p, section_start, section_vaddr, cie.encoding & 0xf);
        if (cie.has_z_augmentation) {
            auto n = read_uleb128(p);
            p += n;
        }
        printf("FDE: initial addr %#lx, range %#lx\n", initial_addr, range);
    }
    p = end;
}
}  // namespace DWARF
