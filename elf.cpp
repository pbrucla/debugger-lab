#include "elf.hpp"

#include <elf.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util.hpp"

void parse_elf(const char* filename) {
    int fd = util::throw_errno(open(filename, O_RDONLY));

    struct stat sb;
    util::throw_errno(fstat(fd, &sb));
    auto* file = static_cast<uint8_t*>(mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
    util::throw_assert(file != MAP_FAILED, "mmap failed");
    auto* ehdr = reinterpret_cast<Elf64_Ehdr*>(file);

    util::throw_assert(memcmp(ehdr->e_ident, ELFMAG, SELFMAG) == 0, "not an ELF file");
    util::throw_assert(ehdr->e_machine == EM_X86_64, "unsupported machine");
    util::throw_assert(ehdr->e_type == ET_EXEC || ehdr->e_type == ET_DYN, "unsupported file type");
    util::throw_assert(ehdr->e_ehsize == sizeof(Elf64_Ehdr), "wrong ehdr size");
    util::throw_assert(ehdr->e_phentsize == sizeof(Elf64_Phdr), "wrong phdr size");
    util::throw_assert(ehdr->e_shentsize == sizeof(Elf64_Shdr), "wrong shdr size");

    auto* shdrs = reinterpret_cast<Elf64_Shdr*>(file + ehdr->e_shoff);
    auto* shdr_shstrtab = shdrs + ehdr->e_shstrndx;
    util::throw_assert(shdr_shstrtab->sh_type == SHT_STRTAB, "shstrtab is not a string table");
    auto* shstrtab = reinterpret_cast<char*>(file + shdr_shstrtab->sh_offset);

    Elf64_Off symtab_offset = 0;
    Elf64_Off strtab_offset = 0;
    size_t symtab_num = 0;
    for (Elf64_Half i = 0; i < ehdr->e_shnum; ++i) {
        auto* shdr = shdrs + i;
        const char* name = shstrtab + shdr->sh_name;
        printf("section %d: %s\n", i, name);
        if (shdr->sh_type == SHT_SYMTAB) {
            util::throw_assert(strcmp(name, ".symtab") == 0, "symbol table has unexpected section name");
            util::throw_assert(shdr->sh_entsize == sizeof(Elf64_Sym), "symbol table has unexpected entry size");
            symtab_offset = shdr->sh_offset;
            symtab_num = shdr->sh_size / sizeof(Elf64_Sym);
        } else if (shdr->sh_type == SHT_STRTAB && strcmp(name, ".strtab") == 0) {
            strtab_offset = shdr->sh_offset;
        }
    }

    util::throw_assert(symtab_offset != 0, "symbol table not found (handle gracefully later)");
    util::throw_assert(strtab_offset != 0, "string table not found (handle gracefully later)");
    auto* symtab = reinterpret_cast<Elf64_Sym*>(file + symtab_offset);
    auto* strtab = reinterpret_cast<char*>(file + strtab_offset);
    for (size_t i = 0; i < symtab_num; ++i) {
        auto* sym = symtab + i;
        if (ELF64_ST_TYPE(sym->st_info) == STT_FUNC && sym->st_value != 0) {
            printf("symbol %s: %#lx\n", strtab + sym->st_name, sym->st_value);
        }
    }

    munmap(file, sb.st_size);
    close(fd);
}
