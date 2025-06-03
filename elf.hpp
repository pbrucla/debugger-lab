#pragma once

#include <elf.h>
#include <stdint.h>

#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>

class ELF {
   public:
    explicit ELF(const char* filename, uint64_t base = 0);
    ELF(const ELF& other) = delete;
    ELF& operator=(const ELF& other) = delete;
    ELF(ELF&& other) { *this = std::move(other); }
    ELF& operator=(ELF&& other);
    ~ELF();
    uint64_t base() const { return m_base; }
    void set_base_from_entry(uint64_t entry) { m_base = entry - m_entry; }
    std::optional<std::string_view> interp() const;
    std::optional<uint64_t> lookup_sym(std::string_view name) const;
    std::optional<std::string_view> lookup_addr(uint64_t addr) const;

   private:
    Elf64_Shdr* find_section(const char* name) const;
    void parse(const char* filename);

    uint64_t m_base;
    uint8_t* m_file;
    size_t m_filesize;
    size_t m_shnum;
    Elf64_Shdr* m_shdrs;
    const char* m_shstrtab;
    uint64_t m_entry;
    std::unordered_map<std::string_view, uint64_t> m_syms;
};
