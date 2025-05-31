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
    std::optional<std::string_view> lookup_addr(uint64_t addr) const;

   private:
    Elf64_Shdr* find_section(const char* name) const;
    void parse(const char* filename);

    uint64_t m_base = 0;
    uint8_t* m_file;
    size_t m_filesize;
    size_t m_shnum;
    Elf64_Shdr* m_shdrs;
    const char* m_shstrtab;
    std::unordered_map<std::string_view, uint64_t> m_syms;
};
