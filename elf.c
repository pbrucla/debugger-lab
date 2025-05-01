#include <assert.h>
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>

// typedef struct
// {
//   unsigned char	e_ident[EI_NIDENT];	 Magic number and other info */
//   Elf64_Half	e_type;			/* Object file type */
//   Elf64_Half	e_machine;		/* Architecture */
//   Elf64_Word	e_version;		/* Object file version */
//   Elf64_Addr	e_entry;		/* Entry point virtual address */
//   Elf64_Off	e_phoff;		/* Program header table file offset */
//   Elf64_Off	e_shoff;		/* Section header table file offset */
//   Elf64_Word	e_flags;		/* Processor-specific flags */
//   Elf64_Half	e_ehsize;		/* ELF header size in bytes */
//   Elf64_Half	e_phentsize;		/* Program header table entry size */
//   Elf64_Half	e_phnum;		/* Program header table entry count */
//   Elf64_Half	e_shentsize;		/* Section header table entry size */
//   Elf64_Half	e_shnum;		/* Section header table entry count */
//   Elf64_Half	e_shstrndx;		/* Section header string table index */
// } Elf64_Ehdr;

// typedef struct
// {
//   Elf64_Word	sh_name;		/* Section name (string tbl index) */
//   Elf64_Word	sh_type;		/* Section type */
//   Elf64_Xword	sh_flags;		/* Section flags */
//   Elf64_Addr	sh_addr;		/* Section virtual addr at execution */
//   Elf64_Off	sh_offset;		/* Section file offset */
//   Elf64_Xword	sh_size;		/* Section size in bytes */
//   Elf64_Word	sh_link;		/* Link to another section */
//   Elf64_Word	sh_info;		/* Additional section information */
//   Elf64_Xword	sh_addralign;		/* Section alignment */
//   Elf64_Xword	sh_entsize;		/* Entry size if section holds table */
// } Elf64_Shdr;

int main(int argc, const char **argv)
{
  assert(argc == 2 && "missing file name");

  int fd = open(argv[1], O_RDONLY);
  assert(fd != -1 && "cannot open file");

  /* mmap the file into memory */
  struct stat sb;
  assert(fstat(fd, &sb) && "fstat failed");
  uint8_t *file = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(file != MAP_FAILED && "mmap failed");

  Elf64_Ehdr *ehdr = (Elf64_Ehdr*)file;

  /* Verify that the ELF magic bytes ar correct */
  assert(memcmp(ehdr->e_ident, ELFMAG, SELFMAG) == 0 && "not an ELF file");
  assert(ehdr->e_machine == EM_X86_64 && "unsupported machine");
  assert((ehdr->e_type == ET_EXEC || ehdr->e_type == ET_DYN) && "unsupported file type");
  assert(ehdr->e_ehsize == sizeof(Elf64_Ehdr) && "wrong ehdr size");
  assert(ehdr->e_phentsize == sizeof(Elf64_Phdr) && "wrong phdr size");
  assert(ehdr->e_shentsize == sizeof(Elf64_Shdr) && "wrong shdr size");

  /* Get the string table section header */
  Elf64_Shdr *shdrs = file + ehdr->e_shoff;
  Elf64_Shdr *shstrtab_hdr = shdrs + ehdr->e_shstrndx;
  assert(shstrtab_hdr->sh_type == SHT_STRTAB && "not a string table");
  const char *strtab = file + shstrtab_hdr->sh_offset;

  /* Read the section header and parse for the symbol table section */
  Elf64_Off symtab_offset = 0;
  Elf64_Xword	sh_size = -1;
  for (int i = 0; i < ehdr->e_shnum; ++i) {
    Elf64_Shdr *shdr = shdrs + i;
    printf("section %d: %s\n", i, &strtab[shdrs[i].sh_name]);
    if (shdr->sh_type == SHT_SYMTAB) {
      symtab_offset = shdr->sh_offset;
      sh_size = shdr->sh_size;
      break;
    }
  }

  assert(symtab_offset != 0 && "Symbol table not found (handle gracefully later)");
  
  return 0;
}
