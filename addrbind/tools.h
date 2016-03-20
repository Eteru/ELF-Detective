#ifndef TOOLS_H_INCLUDED
#define TOOLS_H_INCLUDED

#include <string>
#include <cstdarg>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <bfd.h>
#include <dis-asm.h>

/* address binding information */

typedef struct
{
    std::string name;
    std::string type;

    std::string undefined_in;
    std::string defined_in;
    std::string exe_name;

    std::string undef_value;
    std::string def_value;
    std::string exe_value;

    std::string undefined_section;
    std::string section_name;
    std::string section_value;
    std::string offset;

    bfd_vma sz;
} addrbind;

/* Pseudo FILE object for strings.  */
typedef struct
{
    char *buffer;
    size_t pos;
    size_t alloc;
} SFILE;

struct print_file_list
{
    struct print_file_list *next;
    const char *filename;
    const char *modname;
    const char *map;
    size_t mapsize;
    const char **linemap;
    unsigned maxline;
    unsigned last_line;
    int first;
};

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

/* A structure to record the sections mentioned in -j switches.  */
struct only
{
    const char * name; /* The name of the section.  */
    bfd_boolean  seen; /* A flag to indicate that the section has been found in one or more input files.  */
    struct only * next; /* Pointer to the next structure in the list.  */
};

/* Return the filename in a static buffer.  */
const char *bfd_get_archive_filename (const bfd *);

void bfd_nonfatal (const char *);

void bfd_nonfatal_message (const char *, const bfd *, const asection *,
                           const char *, ...);

void bfd_fatal (const char *) ATTRIBUTE_NORETURN;

void report (const char *, va_list) ATTRIBUTE_PRINTF(1,0);

void fatal (const char *, ...) ATTRIBUTE_PRINTF_1 ATTRIBUTE_NORETURN;

void non_fatal (const char *, ...) ATTRIBUTE_PRINTF_1;

void list_matching_formats (char **);

off_t get_file_size (const char *);

extern void *bfd_malloc (bfd_size_type);

#endif // TOOLS_H_INCLUDED
