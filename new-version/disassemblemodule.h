#ifndef DISASSEMBLEMODULE_H
#define DISASSEMBLEMODULE_H

// this needs to be defined before any bfd.h include
// due to a 'won't fix' bug
#define PACKAGE "elfdetective"

#include <bfd.h>
#include <dis-asm.h>
#include <libiberty/safe-ctype.h>

#include <iostream>
#include <vector>
#include <string>
#include <cstring>

#include "elffile.h"
#include "tools.h"
#include "elf-bfd.h"

/* Extra info to pass to the section disassembler and address printing
   function.  */
struct disasm_info
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
  void print_to_string(std::string);
  int ATTRIBUTE_PRINTF_2 disassemble_print (SFILE *, const char *, ...);

  int symbol_at_address(bfd_vma, struct disassemble_info *);
  void print_addr_with_sym(bfd *, asection *, asymbol *, bfd_vma, struct disassemble_info *);
  asymbol *find_symbol_for_address(bfd_vma, struct disassemble_info *, long *);
  void print_value(bfd_vma, struct disassemble_info *);
  void print_addr(bfd_vma, struct disassemble_info *);
  void print_address(bfd_vma, struct disassemble_info *);

  long remove_useless_symbols (asymbol **, long);
  int compare_relocs(const void *, const void *);
  bfd_boolean process_section_p(asection *);
  void disassemble_bytes(struct disassemble_info *, disassembler_ftype, bfd_boolean, bfd_byte *,
                         bfd_vma, bfd_vma, bfd_vma, arelent ***, arelent **, Function *);
  void disassemble_section(bfd *, asection *, void *);


  void add_section (std::string);
  void disassemble_data(ELFFile *);
}

#endif // DISASSEMBLEMODULE_H
