#pragma once

#include <elf.h>
#include <stdint.h>

#include <optional>
#include <string_view>
#include <unordered_map>

class ELF {
   public:
    explicit ELF(const char* filename);
    ~ELF();
    void set_base(uint64_t base) { m_base = base; }
    std::optional<uint64_t> lookup_sym(std::string_view name) const;

   private:
    Elf64_Shdr* find_section(const char* name) const;
    void parse(const char* filename);
    void parse_eh_frame();

    uint64_t m_base = 0;
    uint8_t* m_file;
    size_t m_filesize;
    size_t m_shnum;
    Elf64_Shdr* m_shdrs;
    const char* m_shstrtab;
    std::unordered_map<std::string_view, uint64_t> m_syms;
};

struct EhFrameHdr {
    uint8_t version;
    uint8_t eh_frame_ptr_enc;
    uint8_t fde_count_enc;
    uint8_t table_enc;
    uint32_t eh_frame_ptr;
    uint32_t fde_count;
    EhFrameHdrTblEnt* bin_search_tbl;
};

struct EhFrameHdrTblEnt {
    uint32_t loc;
    uint32_t addr;
};
