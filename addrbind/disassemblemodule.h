#ifndef DISASSEMBLEMODULE_H_INCLUDED
#define DISASSEMBLEMODULE_H_INCLUDED

#include <bfd.h>
#include <dis-asm.h>
#include <libiberty/safe-ctype.h>

#include <iostream>
#include <vector>
#include <string>
#include <cstring>

#include "ELFFile.h"
#include "tools.h"
#include "elf-bfd.h"

/* Extra info to pass to the section disassembler and address printing
   function.  */
struct objdump_disasm_info
{
    bfd *              abfd;
    asection *         sec;
    bfd_boolean        require_sec;
    arelent **         dynrelbuf;
    long               dynrelcount;
    disassembler_ftype disassemble_fn;
    arelent *          reloc;
};

namespace Disassembly
{
static std::vector<std::string> sections;
static std::string output;
static std::string disassemble_line;

static void print_to_string(std::string, bool);
static int ATTRIBUTE_PRINTF_2 disassemble_print (SFILE *, const char *, ...);

static const int DEFAULT_SKIP_ZEROES = 8;
static const int DEFAULT_SKIP_ZEROES_AT_END = 3;

static bfd_vma start_address = (bfd_vma) -1;
static bfd_vma stop_address = (bfd_vma) -1;

/* The symbol table.  */
static asymbol **syms;

/* Number of symbols in `syms'.  */
static long symcount = 0, dynsymcount = 0;

/* The dynamic symbol table.  */
static asymbol **dynsyms;

static long sorted_symcount;
static asymbol **sorted_syms;

/* The synthetic symbol table.  */
static asymbol *synthsyms;
static long synthcount = 0;

/* Hold the last function name and the last line number we displayed
   in a disassembly.  */

static char *prev_functionname;
static unsigned int prev_line;
static unsigned int prev_discriminator;

/* We keep a list of all files that we have seen when doing a
   disassembly with source, so that we know how much of the file to
   display.  This can be important for inlined functions.  */

static struct print_file_list *print_files;

/* Endianness to disassemble for, or default if BFD_ENDIAN_UNKNOWN.  */
static enum bfd_endian endian = BFD_ENDIAN_UNKNOWN;



static int objdump_symbol_at_address(bfd_vma, struct disassemble_info *);
static void objdump_print_symname(bfd *, struct disassemble_info *, asymbol *, bool);
static void objdump_print_addr_with_sym(bfd *, asection *, asymbol *, bfd_vma, struct disassemble_info *, bool);
static asymbol *find_symbol_for_address(bfd_vma, struct disassemble_info *, long *);
static void objdump_print_value(bfd_vma, struct disassemble_info *, bool);
static void objdump_print_addr(bfd_vma, struct disassemble_info *);
static void objdump_print_address(bfd_vma, struct disassemble_info *);

static long remove_useless_symbols (asymbol **, long);
static int compare_relocs(const void *, const void *);
static bfd_boolean process_section_p(asection *);
static void disassemble_bytes(struct disassemble_info *, disassembler_ftype, bfd_boolean, bfd_byte *,
                              bfd_vma, bfd_vma, bfd_vma, arelent ***, arelent **);
static void disassemble_section(bfd *, asection *, void *);


void add_section (std::string);
std::string disassemble_data(ELFFile *);


}
#endif // DISASSEMBLEMODULE_H_INCLUDED
