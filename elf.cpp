#include "elf.hpp"

#include <elf.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "dwarf.hpp"
#include "util.hpp"

ELF::ELF(const char* filename, uint64_t base) : m_base(base) { parse(filename); }

ELF& ELF::operator=(ELF&& other) {
    m_base = other.m_base;
    m_file = other.m_file;
    m_filesize = other.m_filesize;
    m_shnum = other.m_shnum;
    m_shdrs = other.m_shdrs;
    m_shstrtab = other.m_shstrtab;
    m_entry = other.m_entry;
    m_syms = std::move(other.m_syms);
    other.m_file = nullptr;
    return *this;
}

ELF::~ELF() {
    if (m_file) {
        munmap(m_file, m_filesize);
    }
}

std::optional<std::string_view> ELF::interp() const {
    auto* interp_shdr = find_section(".interp");
    if (!interp_shdr) {
        return {};
    }
    return reinterpret_cast<char*>(m_file + interp_shdr->sh_offset);
}

std::optional<uint64_t> ELF::lookup_sym(std::string_view name) const {
    auto sym = m_syms.find(name);
    if (sym == m_syms.end()) return {};
    return m_base + sym->second;
}

std::optional<std::string_view> ELF::lookup_addr(uint64_t addr) const {
    addr -= m_base;
    std::optional<std::string_view> ret;
    std::optional<uint64_t> ret_addr;
    for (const auto& [sym, sym_addr] : m_syms) {
        if (addr >= sym_addr && sym_addr >= ret_addr.value_or(0)) {
            ret = sym;
            ret_addr = sym_addr;
        }
    }
    return ret;
}

Elf64_Shdr* ELF::find_section(const char* name) const {
    for (size_t i = 0; i < m_shnum; ++i) {
        auto* shdr = m_shdrs + i;
        const char* sh_name = m_shstrtab + shdr->sh_name;
        if (strcmp(name, sh_name) == 0) {
            return shdr;
        }
    }
    return nullptr;
}

void ELF::parse(const char* filename) {
    int fd = util::throw_errno(open(filename, O_RDONLY));

    struct stat sb;
    util::throw_errno(fstat(fd, &sb));
    m_filesize = sb.st_size;
    m_file = static_cast<uint8_t*>(mmap(nullptr, m_filesize, PROT_READ, MAP_PRIVATE, fd, 0));
    util::throw_assert(m_file != MAP_FAILED, "mmap failed");
    auto* ehdr = reinterpret_cast<Elf64_Ehdr*>(m_file);
    m_shnum = ehdr->e_shnum;
    m_entry = ehdr->e_entry;

    util::throw_assert(memcmp(ehdr->e_ident, ELFMAG, SELFMAG) == 0, "not an ELF file");
    util::throw_assert(ehdr->e_machine == EM_X86_64, "unsupported machine");
    util::throw_assert(ehdr->e_type == ET_EXEC || ehdr->e_type == ET_DYN, "unsupported file type");
    util::throw_assert(ehdr->e_ehsize == sizeof(Elf64_Ehdr), "wrong ehdr size");
    util::throw_assert(ehdr->e_phentsize == sizeof(Elf64_Phdr), "wrong phdr size");
    util::throw_assert(ehdr->e_shentsize == sizeof(Elf64_Shdr), "wrong shdr size");

    m_shdrs = reinterpret_cast<Elf64_Shdr*>(m_file + ehdr->e_shoff);
    auto* shdr_shstrtab = m_shdrs + ehdr->e_shstrndx;
    util::throw_assert(shdr_shstrtab->sh_type == SHT_STRTAB, "shstrtab is not a string table");
    m_shstrtab = reinterpret_cast<char*>(m_file + shdr_shstrtab->sh_offset);

    auto collect_syms = [this](const char* symtab_name, const char* strtab_name) {
        auto* symtab_shdr = find_section(symtab_name);
        if (!symtab_shdr) {
            printf("No %s section found\n", symtab_name);
            return;
        }
        util::throw_assert(symtab_shdr->sh_entsize == sizeof(Elf64_Sym), "symbol table has unexpected entry size");
        util::throw_assert(symtab_shdr->sh_type == SHT_SYMTAB || symtab_shdr->sh_type == SHT_DYNSYM);
        auto* strtab_shdr = find_section(strtab_name);
        util::throw_assert(strtab_shdr, "symbol table has no corresponding string table");
        util::throw_assert(strtab_shdr->sh_type == SHT_STRTAB);

        auto* symtab = reinterpret_cast<Elf64_Sym*>(m_file + symtab_shdr->sh_offset);
        auto* strtab = reinterpret_cast<char*>(m_file + strtab_shdr->sh_offset);
        for (size_t i = 0; i < symtab_shdr->sh_size / sizeof(Elf64_Sym); ++i) {
            auto* sym = symtab + i;
            if (ELF64_ST_TYPE(sym->st_info) == STT_FUNC && sym->st_shndx != SHN_UNDEF) {
                m_syms.emplace(strtab + sym->st_name, sym->st_value);
            }
        }
    };

    collect_syms(".symtab", ".strtab");
    collect_syms(".dynsym", ".dynstr");
    printf("%zu symbols loaded from %s\n", m_syms.size(), filename);

    auto* eh_frame_shdr = find_section(".eh_frame");
    if (!eh_frame_shdr) {
        puts("No .eh_frame section found");
    } else {
#if 0
        auto* eh_frame = m_file + eh_frame_shdr->sh_offset;
        size_t eh_frame_size = eh_frame_shdr->sh_size;
        const auto* p = eh_frame;
        std::unordered_map<size_t, DWARF::CIE> cie_map;
        while (p < eh_frame + eh_frame_size) DWARF::parse_eh_frame_entry(p, eh_frame, eh_frame_shdr->sh_addr, cie_map);
#endif
    }

    close(fd);
}
