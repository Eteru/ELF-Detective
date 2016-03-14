#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <unordered_map>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cerrno>
#include <bfd.h>
#include <dis-asm.h>
#include <libiberty/safe-ctype.h>

#include "ELFFile.h"

#include "tools.h"
#include "elf-bfd.h"

/* The number of zeroes we want to see before we start skipping them.
   The number is arbitrarily chosen.  */

#define DEFAULT_SKIP_ZEROES 8

/* The number of zeroes to skip at the end of a section.  If the
   number of zeroes at the end is between SKIP_ZEROES_AT_END and
   SKIP_ZEROES, they will be disassembled.  If there are fewer than
   SKIP_ZEROES_AT_END, they will be skipped.  This is a heuristic
   attempt to avoid disassembling zeroes inserted by section
   alignment.  */

#define DEFAULT_SKIP_ZEROES_AT_END 3

bfd *abfs;

static int dump_section_contents;	/* -s */
static int dump_symtab;			/* -t */
static int dump_dynamic_symtab;		/* -T */
static int dump_reloc_info;		/* -r */
static int dump_dynamic_reloc_info;	/* -R */
static bfd_boolean disassemble;		/* -d */
static bfd_boolean disassemble_all;	/* -D */
static int disassemble_zeroes;		/* --disassemble-zeroes */
static int dump_special_syms = 0;	/* --special-syms */
static int prefix_addresses;		/* --prefix-addresses */
static int show_raw_insn;		/* --show-raw-insn */
static bfd_boolean display_file_offsets;/* -F */
static int wide_output;			/* -w */

static bfd_vma start_address = (bfd_vma) -1; /* --start-address */
static bfd_vma stop_address = (bfd_vma) -1;  /* --stop-address */

/* Architecture to disassemble for, or default if NULL.  */
static char *machine = NULL;

/* Target specific options to the disassembler.  */
static char *disassembler_options = NULL;

/* Endianness to disassemble for, or default if BFD_ENDIAN_UNKNOWN.  */
static enum bfd_endian endian = BFD_ENDIAN_UNKNOWN;

/* The symbol table.  */
static asymbol **syms;

/* Number of symbols in `syms'.  */
static long symcount = 0;

/* The sorted symbol table.  */
static asymbol **sorted_syms;

/* Number of symbols in `sorted_syms'.  */
static long sorted_symcount = 0;

/* The dynamic symbol table.  */
static asymbol **dynsyms;

/* The synthetic symbol table.  */
static asymbol *synthsyms;
static long synthcount = 0;

/* Number of symbols in `dynsyms'.  */
static long dynsymcount = 0;

static bfd_byte *stabs;
static bfd_size_type stab_size;

static char *strtab;
static bfd_size_type stabstr_size;

static bfd_boolean is_relocatable = FALSE;

static char *default_target = NULL;	/* Default at runtime.  */


/* Pointer to an array of 'only' structures.
   This pointer is NULL if the -j switch has not been used.  */

static struct only * only_list = NULL;

/* Hold the last function name and the last line number we displayed
   in a disassembly.  */

static char *prev_functionname;
static unsigned int prev_line;
static unsigned int prev_discriminator;

/* We keep a list of all files that we have seen when doing a
   disassembly with source, so that we know how much of the file to
   display.  This can be important for inlined functions.  */


static struct print_file_list *print_files;

/* sprintf to a "stream".  */

static int ATTRIBUTE_PRINTF_2 objdump_sprintf (SFILE *f, const char *format, ...)
{
    size_t n;
    va_list args;

    while (1)
    {
        size_t space = f->alloc - f->pos;

        va_start (args, format);
        n = vsnprintf (f->buffer + f->pos, space, format, args);
        va_end (args);

        if (space > n)
            break;

        f->alloc = (f->alloc + n) * 2;
        f->buffer = (char *) realloc (f->buffer, f->alloc);
    }
    f->pos += n;

    return n;
}

using namespace std;

static void nonfatal (const char *msg)
{
    bfd_nonfatal (msg);
}

/* Returns TRUE if the specified section should be dumped.  */

static bfd_boolean process_section_p (asection * section)
{
    struct only * only;

    if (only_list == NULL)
        return TRUE;

    for (only = only_list; only; only = only->next)
        if (strcmp (only->name, section->name) == 0)
        {
            only->seen = TRUE;
            return TRUE;
        }

    return FALSE;
}


/* Filter out (in place) symbols that are useless for disassembly.
   COUNT is the number of elements in SYMBOLS.
   Return the number of useful symbols.  */

static long remove_useless_symbols (asymbol **symbols, long count)
{
    asymbol **in_ptr = symbols, **out_ptr = symbols;

    while (--count >= 0)
    {
        asymbol *sym = *in_ptr++;

        if (sym->name == NULL || sym->name[0] == '\0')
            continue;
        if (sym->flags & (BSF_DEBUGGING | BSF_SECTION_SYM))
            continue;
        if (bfd_is_und_section (sym->section)
                || bfd_is_com_section (sym->section))
            continue;

        *out_ptr++ = sym;
    }
    return out_ptr - symbols;
}

static int compare_symbols (const void *ap, const void *bp)
{
    const asymbol *a = * (const asymbol **) ap;
    const asymbol *b = * (const asymbol **) bp;
    const char *an;
    const char *bn;
    size_t anl;
    size_t bnl;
    bfd_boolean af;
    bfd_boolean bf;
    flagword aflags;
    flagword bflags;

    if (bfd_asymbol_value (a) > bfd_asymbol_value (b))
        return 1;
    else if (bfd_asymbol_value (a) < bfd_asymbol_value (b))
        return -1;

    if (a->section > b->section)
        return 1;
    else if (a->section < b->section)
        return -1;

    an = bfd_asymbol_name (a);
    bn = bfd_asymbol_name (b);
    anl = strlen (an);
    bnl = strlen (bn);

    /* The symbols gnu_compiled and gcc2_compiled convey no real
       information, so put them after other symbols with the same value.  */
    af = (strstr (an, "gnu_compiled") != NULL
          || strstr (an, "gcc2_compiled") != NULL);
    bf = (strstr (bn, "gnu_compiled") != NULL
          || strstr (bn, "gcc2_compiled") != NULL);

    if (af && ! bf)
        return 1;
    if (! af && bf)
        return -1;

    /* We use a heuristic for the file name, to try to sort it after
       more useful symbols.  It may not work on non Unix systems, but it
       doesn't really matter; the only difference is precisely which
       symbol names get printed.  */

#define file_symbol(s, sn, snl)			\
  (((s)->flags & BSF_FILE) != 0			\
   || ((sn)[(snl) - 2] == '.'			\
       && ((sn)[(snl) - 1] == 'o'		\
	   || (sn)[(snl) - 1] == 'a')))

    af = file_symbol (a, an, anl);
    bf = file_symbol (b, bn, bnl);

    if (af && ! bf)
        return 1;
    if (! af && bf)
        return -1;

    /* Try to sort global symbols before local symbols before function
       symbols before debugging symbols.  */

    aflags = a->flags;
    bflags = b->flags;

    if ((aflags & BSF_DEBUGGING) != (bflags & BSF_DEBUGGING))
    {
        if ((aflags & BSF_DEBUGGING) != 0)
            return 1;
        else
            return -1;
    }
    if ((aflags & BSF_FUNCTION) != (bflags & BSF_FUNCTION))
    {
        if ((aflags & BSF_FUNCTION) != 0)
            return -1;
        else
            return 1;
    }
    if ((aflags & BSF_LOCAL) != (bflags & BSF_LOCAL))
    {
        if ((aflags & BSF_LOCAL) != 0)
            return 1;
        else
            return -1;
    }
    if ((aflags & BSF_GLOBAL) != (bflags & BSF_GLOBAL))
    {
        if ((aflags & BSF_GLOBAL) != 0)
            return -1;
        else
            return 1;
    }

    /* Symbols that start with '.' might be section names, so sort them
       after symbols that don't start with '.'.  */
    if (an[0] == '.' && bn[0] != '.')
        return 1;
    if (an[0] != '.' && bn[0] == '.')
        return -1;

    /* Finally, if we can't distinguish them in any other way, try to
       get consistent results by sorting the symbols by name.  */
    return strcmp (an, bn);
}

/* Sort relocs into address order.  */

static int compare_relocs (const void *ap, const void *bp)
{
    const arelent *a = * (const arelent **) ap;
    const arelent *b = * (const arelent **) bp;

    if (a->address > b->address)
        return 1;
    else if (a->address < b->address)
        return -1;

    /* So that associated relocations tied to the same address show up
       in the correct order, we don't do any further sorting.  */
    if (a > b)
        return 1;
    else if (a < b)
        return -1;
    else
        return 0;
}

/* Print the name of a symbol.  */

static void objdump_print_symname (bfd *abfd, struct disassemble_info *inf,
                                   asymbol *sym)
{
    char *alloc;
    const char *name, *version_string = NULL;
    bfd_boolean hidden = FALSE;

    alloc = NULL;
    name = bfd_asymbol_name (sym);

    //version_string = bfd_get_symbol_version_string (abfd, sym, &hidden);

    if (bfd_is_und_section (bfd_get_section (sym)))
        hidden = TRUE;

    if (inf != NULL)
    {
        (*inf->fprintf_func) (inf->stream, "%s", name);
        if (version_string && *version_string != '\0')
            (*inf->fprintf_func) (inf->stream, hidden ? "@%s" : "@@%s",
                                  version_string);
    }
    else
    {
        printf ("%s", name);
        if (version_string && *version_string != '\0')
            printf (hidden ? "@%s" : "@@%s", version_string);
    }

    if (alloc != NULL)
        free (alloc);
}

/* Print an address (VMA) to the output stream in INFO.
   If SKIP_ZEROES is TRUE, omit leading zeroes.  */

static void objdump_print_value (bfd_vma vma, struct disassemble_info *inf,
                                 bfd_boolean skip_zeroes)
{
    char buf[30];
    char *p;
    struct objdump_disasm_info *aux;

    aux = (struct objdump_disasm_info *) inf->application_data;
    bfd_sprintf_vma (aux->abfd, buf, vma);
    if (! skip_zeroes)
        p = buf;
    else
    {
        for (p = buf; *p == '0'; ++p)
            ;
        if (*p == '\0')
            --p;
    }
    (*inf->fprintf_func) (inf->stream, "%s", p);
}

/* Locate a symbol given a bfd and a section (from INFO->application_data),
   and a VMA.  If INFO->application_data->require_sec is TRUE, then always
   require the symbol to be in the section.  Returns NULL if there is no
   suitable symbol.  If PLACE is not NULL, then *PLACE is set to the index
   of the symbol in sorted_syms.  */

static asymbol *find_symbol_for_address (bfd_vma vma,
        struct disassemble_info *inf,
        long *place)
{
    /* @@ Would it speed things up to cache the last two symbols returned,
       and maybe their address ranges?  For many processors, only one memory
       operand can be present at a time, so the 2-entry cache wouldn't be
       constantly churned by code doing heavy memory accesses.  */

    /* Indices in `sorted_syms'.  */
    long min = 0;
    long max_count = sorted_symcount;
    long thisplace;
    struct objdump_disasm_info *aux;
    bfd *abfd;
    asection *sec;
    unsigned int opb;
    bfd_boolean want_section;

    if (sorted_symcount < 1)
        return NULL;

    aux = (struct objdump_disasm_info *) inf->application_data;
    abfd = aux->abfd;
    sec = aux->sec;
    opb = inf->octets_per_byte;

    /* Perform a binary search looking for the closest symbol to the
       required value.  We are searching the range (min, max_count].  */
    while (min + 1 < max_count)
    {
        asymbol *sym;

        thisplace = (max_count + min) / 2;
        sym = sorted_syms[thisplace];

        if (bfd_asymbol_value (sym) > vma)
            max_count = thisplace;
        else if (bfd_asymbol_value (sym) < vma)
            min = thisplace;
        else
        {
            min = thisplace;
            break;
        }
    }

    /* The symbol we want is now in min, the low end of the range we
       were searching.  If there are several symbols with the same
       value, we want the first one.  */
    thisplace = min;
    while (thisplace > 0
            && (bfd_asymbol_value (sorted_syms[thisplace])
                == bfd_asymbol_value (sorted_syms[thisplace - 1])))
        --thisplace;

    /* Prefer a symbol in the current section if we have multple symbols
       with the same value, as can occur with overlays or zero size
       sections.  */
    min = thisplace;
    while (min < max_count
            && (bfd_asymbol_value (sorted_syms[min])
                == bfd_asymbol_value (sorted_syms[thisplace])))
    {
        if (sorted_syms[min]->section == sec
                && inf->symbol_is_valid (sorted_syms[min], inf))
        {
            thisplace = min;

            if (place != NULL)
                *place = thisplace;

            return sorted_syms[thisplace];
        }
        ++min;
    }

    /* If the file is relocatable, and the symbol could be from this
       section, prefer a symbol from this section over symbols from
       others, even if the other symbol's value might be closer.

       Note that this may be wrong for some symbol references if the
       sections have overlapping memory ranges, but in that case there's
       no way to tell what's desired without looking at the relocation
       table.

       Also give the target a chance to reject symbols.  */
    want_section = (aux->require_sec
                    || ((abfd->flags & HAS_RELOC) != 0
                        && vma >= bfd_get_section_vma (abfd, sec)
                        && vma < (bfd_get_section_vma (abfd, sec)
                                  + bfd_section_size (abfd, sec) / opb)));
    if ((sorted_syms[thisplace]->section != sec && want_section)
            || ! inf->symbol_is_valid (sorted_syms[thisplace], inf))
    {
        long i;
        long newplace = sorted_symcount;

        for (i = min - 1; i >= 0; i--)
        {
            if ((sorted_syms[i]->section == sec || !want_section)
                    && inf->symbol_is_valid (sorted_syms[i], inf))
            {
                if (newplace == sorted_symcount)
                    newplace = i;

                if (bfd_asymbol_value (sorted_syms[i])
                        != bfd_asymbol_value (sorted_syms[newplace]))
                    break;

                /* Remember this symbol and keep searching until we reach
                an earlier address.  */
                newplace = i;
            }
        }

        if (newplace != sorted_symcount)
            thisplace = newplace;
        else
        {
            /* We didn't find a good symbol with a smaller value.
               Look for one with a larger value.  */
            for (i = thisplace + 1; i < sorted_symcount; i++)
            {
                if ((sorted_syms[i]->section == sec || !want_section)
                        && inf->symbol_is_valid (sorted_syms[i], inf))
                {
                    thisplace = i;
                    break;
                }
            }
        }

        if ((sorted_syms[thisplace]->section != sec && want_section)
                || ! inf->symbol_is_valid (sorted_syms[thisplace], inf))
            /* There is no suitable symbol.  */
            return NULL;
    }

    if (place != NULL)
        *place = thisplace;

    return sorted_syms[thisplace];
}

/* Print an address and the offset to the nearest symbol.  */

static void objdump_print_addr_with_sym (bfd *abfd, asection *sec, asymbol *sym,
        bfd_vma vma, struct disassemble_info *inf,
        bfd_boolean skip_zeroes)
{
    objdump_print_value (vma, inf, skip_zeroes);

    if (sym == NULL)
    {
        bfd_vma secaddr;

        (*inf->fprintf_func) (inf->stream, " <%s",
                              bfd_get_section_name (abfd, sec));
        secaddr = bfd_get_section_vma (abfd, sec);
        if (vma < secaddr)
        {
            (*inf->fprintf_func) (inf->stream, "-0x");
            objdump_print_value (secaddr - vma, inf, TRUE);
        }
        else if (vma > secaddr)
        {
            (*inf->fprintf_func) (inf->stream, "+0x");
            objdump_print_value (vma - secaddr, inf, TRUE);
        }
        (*inf->fprintf_func) (inf->stream, ">");
    }
    else
    {
        (*inf->fprintf_func) (inf->stream, " <");
        objdump_print_symname (abfd, inf, sym);
        if (bfd_asymbol_value (sym) > vma)
        {
            (*inf->fprintf_func) (inf->stream, "-0x");
            objdump_print_value (bfd_asymbol_value (sym) - vma, inf, TRUE);
        }
        else if (vma > bfd_asymbol_value (sym))
        {
            (*inf->fprintf_func) (inf->stream, "+0x");
            objdump_print_value (vma - bfd_asymbol_value (sym), inf, TRUE);
        }
        (*inf->fprintf_func) (inf->stream, ">");
    }
}

/* Print an address (VMA), symbolically if possible.
   If SKIP_ZEROES is TRUE, don't output leading zeroes.  */

static void objdump_print_addr (bfd_vma vma,
                                struct disassemble_info *inf,
                                bfd_boolean skip_zeroes)
{
    struct objdump_disasm_info *aux;
    asymbol *sym = NULL;
    bfd_boolean skip_find = FALSE;

    aux = (struct objdump_disasm_info *) inf->application_data;

    if (sorted_symcount < 1)
    {
        (*inf->fprintf_func) (inf->stream, "0x");
        objdump_print_value (vma, inf, skip_zeroes);

        return;
    }

    if (aux->reloc != NULL
            && aux->reloc->sym_ptr_ptr != NULL
            && * aux->reloc->sym_ptr_ptr != NULL)
    {
        sym = * aux->reloc->sym_ptr_ptr;

        /* Adjust the vma to the reloc.  */
        vma += bfd_asymbol_value (sym);

        if (bfd_is_und_section (bfd_get_section (sym)))
            skip_find = TRUE;
    }

    if (!skip_find)
        sym = find_symbol_for_address (vma, inf, NULL);

    objdump_print_addr_with_sym (aux->abfd, aux->sec, sym, vma, inf,
                                 skip_zeroes);
}

/* Print VMA to INFO.  This function is passed to the disassembler
   routine.  */

static void objdump_print_address (bfd_vma vma, struct disassemble_info *inf)
{
    objdump_print_addr (vma, inf, ! prefix_addresses);
}

/* Determine if the given address has a symbol associated with it.  */

static int objdump_symbol_at_address (bfd_vma vma, struct disassemble_info * inf)
{
    asymbol * sym;

    sym = find_symbol_for_address (vma, inf, NULL);

    return (sym != NULL && (bfd_asymbol_value (sym) == vma));
}

/* Disassemble some data in memory between given values.  */

static void disassemble_bytes (struct disassemble_info * inf,
                               disassembler_ftype        disassemble_fn,
                               bfd_boolean               insns,
                               bfd_byte *                data,
                               bfd_vma                   start_offset,
                               bfd_vma                   stop_offset,
                               bfd_vma		     rel_offset,
                               arelent ***               relppp,
                               arelent **                relppend)
{
    struct objdump_disasm_info *aux;
    asection *section;
    int octets_per_line;
    int skip_addr_chars;
    bfd_vma addr_offset;
    unsigned int opb = inf->octets_per_byte;
    unsigned int skip_zeroes = inf->skip_zeroes;
    unsigned int skip_zeroes_at_end = inf->skip_zeroes_at_end;
    int octets = opb;
    SFILE sfile;

    aux = (struct objdump_disasm_info *) inf->application_data;
    section = aux->sec;

    sfile.alloc = 120;
    sfile.buffer = (char *) malloc (sfile.alloc);
    sfile.pos = 0;

    octets_per_line = 16;

    /* Figure out how many characters to skip at the start of an
       address, to make the disassembly look nicer.  We discard leading
       zeroes in chunks of 4, ensuring that there is always a leading
       zero remaining.  */
    skip_addr_chars = 0;
    if (! prefix_addresses)
    {
        char buf[30];

        bfd_sprintf_vma (aux->abfd, buf, section->vma + section->size / opb);

        while (buf[skip_addr_chars] == '0')
            ++skip_addr_chars;

        /* Don't discard zeros on overflow.  */
        if (buf[skip_addr_chars] == '\0' && section->vma != 0)
            skip_addr_chars = 0;

        if (skip_addr_chars != 0)
            skip_addr_chars = (skip_addr_chars - 1) & -4;
    }

    inf->insn_info_valid = 0;

    addr_offset = start_offset;
    while (addr_offset < stop_offset)
    {
        bfd_vma z;
        bfd_boolean need_nl = FALSE;
        int previous_octets;

        /* Remember the length of the previous instruction.  */
        previous_octets = octets;
        octets = 0;

        /* Make sure we don't use relocs from previous instructions.  */
        aux->reloc = NULL;

        /* If we see more than SKIP_ZEROES octets of zeroes, we just
        print `...'.  */
        for (z = addr_offset * opb; z < stop_offset * opb; z++)
            if (data[z] != 0)
                break;
        if (! disassemble_zeroes
                && (inf->insn_info_valid == 0
                    || inf->branch_delay_insns == 0)
                && (z - addr_offset * opb >= skip_zeroes
                    || (z == stop_offset * opb &&
                        z - addr_offset * opb < skip_zeroes_at_end)))
        {
            /* If there are more nonzero octets to follow, we only skip
               zeroes in multiples of 4, to try to avoid running over
               the start of an instruction which happens to start with
               zero.  */
            if (z != stop_offset * opb)
                z = addr_offset * opb + ((z - addr_offset * opb) &~ 3);

            octets = z - addr_offset * opb;

            /* If we are going to display more data, and we are displaying
               file offsets, then tell the user how many zeroes we skip
               and the file offset from where we resume dumping.  */
            if (display_file_offsets && ((addr_offset + (octets / opb)) < stop_offset))
                printf ("\t... (skipping %d zeroes, resuming at file offset: 0x%lx)\n",
                        octets / opb,
                        (unsigned long) (section->filepos
                                         + (addr_offset + (octets / opb))));
            else
                printf ("\t...\n");
        }
        else
        {
            char buf[50];
            int bpc = 0;
            int pb = 0;

            if (! prefix_addresses)
            {
                char *s;

                bfd_sprintf_vma (aux->abfd, buf, section->vma + addr_offset);
                for (s = buf + skip_addr_chars; *s == '0'; s++)
                    *s = ' ';
                if (*s == '\0')
                    *--s = '0';
                printf ("%s:\t", buf + skip_addr_chars);
            }
            else
            {
                aux->require_sec = TRUE;
                objdump_print_address (section->vma + addr_offset, inf);
                aux->require_sec = FALSE;
                putchar (' ');
            }

            if (insns)
            {
                sfile.pos = 0;
                inf->fprintf_func = (fprintf_ftype) objdump_sprintf;
                inf->stream = &sfile;
                inf->bytes_per_line = 0;
                inf->bytes_per_chunk = 0;
                inf->flags = disassemble_all ? DISASSEMBLE_DATA : 0;
                if (machine)
                    inf->flags |= USER_SPECIFIED_MACHINE_TYPE;

                if (inf->disassembler_needs_relocs
                        && (bfd_get_file_flags (aux->abfd) & EXEC_P) == 0
                        && (bfd_get_file_flags (aux->abfd) & DYNAMIC) == 0
                        && *relppp < relppend)
                {
                    bfd_signed_vma distance_to_rel;

                    distance_to_rel = (**relppp)->address
                                      - (rel_offset + addr_offset);

                    /* Check to see if the current reloc is associated with
                       the instruction that we are about to disassemble.  */
                    if (distance_to_rel == 0
                            /* FIXME: This is wrong.  We are trying to catch
                            relocs that are addressed part way through the
                             current instruction, as might happen with a packed
                             VLIW instruction.  Unfortunately we do not know the
                             length of the current instruction since we have not
                             disassembled it yet.  Instead we take a guess based
                             upon the length of the previous instruction.  The
                             proper solution is to have a new target-specific
                             disassembler function which just returns the length
                             of an instruction at a given address without trying
                             to display its disassembly. */
                            || (distance_to_rel > 0
                                && distance_to_rel < (bfd_signed_vma) (previous_octets/ opb)))
                    {
                        inf->flags |= INSN_HAS_RELOC;
                        aux->reloc = **relppp;
                    }
                }

                if (! disassemble_all
                        && (section->flags & (SEC_CODE | SEC_HAS_CONTENTS))
                        == (SEC_CODE | SEC_HAS_CONTENTS))
                    /* Set a stop_vma so that the disassembler will not read
                       beyond the next symbol.  We assume that symbols appear on
                       the boundaries between instructions.  We only do this when
                       disassembling code of course, and when -D is in effect.  */
                    //inf->stop_vma = section->vma + stop_offset;

                octets = (*disassemble_fn) (section->vma + addr_offset, inf);

                //inf->stop_vma = 0;
                inf->fprintf_func = (fprintf_ftype) fprintf;
                inf->stream = stdout;
                if (inf->bytes_per_line != 0)
                    octets_per_line = inf->bytes_per_line;
                if (octets < (int) opb)
                {
                    if (sfile.pos)
                        printf ("%s\n", sfile.buffer);
                    if (octets >= 0)
                    {
                        non_fatal ("disassemble_fn returned length %d",
                                   octets);
                    }
                    break;
                }
            }
            else
            {
                bfd_vma j;

                octets = octets_per_line;
                if (addr_offset + octets / opb > stop_offset)
                    octets = (stop_offset - addr_offset) * opb;

                for (j = addr_offset * opb; j < addr_offset * opb + octets; ++j)
                {
                    if (ISPRINT (data[j]))
                        buf[j - addr_offset * opb] = data[j];
                    else
                        buf[j - addr_offset * opb] = '.';
                }
                buf[j - addr_offset * opb] = '\0';
            }

            if (prefix_addresses
                    ? show_raw_insn > 0
                    : show_raw_insn >= 0)
            {
                bfd_vma j;

                /* If ! prefix_addresses and ! wide_output, we print
                octets_per_line octets per line.  */
                pb = octets;
                if (pb > octets_per_line && ! prefix_addresses && ! wide_output)
                    pb = octets_per_line;

                if (inf->bytes_per_chunk)
                    bpc = inf->bytes_per_chunk;
                else
                    bpc = 1;

                for (j = addr_offset * opb; j < addr_offset * opb + pb; j += bpc)
                {
                    int k;

                    if (bpc > 1 && inf->display_endian == BFD_ENDIAN_LITTLE)
                    {
                        for (k = bpc - 1; k >= 0; k--)
                            printf ("%02x", (unsigned) data[j + k]);
                        putchar (' ');
                    }
                    else
                    {
                        for (k = 0; k < bpc; k++)
                            printf ("%02x", (unsigned) data[j + k]);
                        putchar (' ');
                    }
                }

                for (; pb < octets_per_line; pb += bpc)
                {
                    int k;

                    for (k = 0; k < bpc; k++)
                        printf ("  ");
                    putchar (' ');
                }

                /* Separate raw data from instruction by extra space.  */
                if (insns)
                    putchar ('\t');
                else
                    printf ("    ");
            }

            if (! insns)
                printf ("%s", buf);
            else if (sfile.pos)
                printf ("%s", sfile.buffer);

            if (prefix_addresses
                    ? show_raw_insn > 0
                    : show_raw_insn >= 0)
            {
                while (pb < octets)
                {
                    bfd_vma j;
                    char *s;

                    putchar ('\n');
                    j = addr_offset * opb + pb;

                    bfd_sprintf_vma (aux->abfd, buf, section->vma + j / opb);
                    for (s = buf + skip_addr_chars; *s == '0'; s++)
                        *s = ' ';
                    if (*s == '\0')
                        *--s = '0';
                    printf ("%s:\t", buf + skip_addr_chars);

                    pb += octets_per_line;
                    if (pb > octets)
                        pb = octets;
                    for (; j < addr_offset * opb + pb; j += bpc)
                    {
                        int k;

                        if (bpc > 1 && inf->display_endian == BFD_ENDIAN_LITTLE)
                        {
                            for (k = bpc - 1; k >= 0; k--)
                                printf ("%02x", (unsigned) data[j + k]);
                            putchar (' ');
                        }
                        else
                        {
                            for (k = 0; k < bpc; k++)
                                printf ("%02x", (unsigned) data[j + k]);
                            putchar (' ');
                        }
                    }
                }
            }

            if (!wide_output)
                putchar ('\n');
            else
                need_nl = TRUE;
        }

        while ((*relppp) < relppend
                && (**relppp)->address < rel_offset + addr_offset + octets / opb)
        {
            if (dump_reloc_info || dump_dynamic_reloc_info)
            {
                arelent *q;

                q = **relppp;

                if (wide_output)
                    putchar ('\t');
                else
                    printf ("\t\t\t");

                objdump_print_value (section->vma - rel_offset + q->address,
                                     inf, TRUE);

                if (q->howto == NULL)
                    printf (": *unknown*\t");
                else if (q->howto->name)
                    printf (": %s\t", q->howto->name);
                else
                    printf (": %d\t", q->howto->type);

                if (q->sym_ptr_ptr == NULL || *q->sym_ptr_ptr == NULL)
                    printf ("*unknown*");
                else
                {
                    const char *sym_name;

                    sym_name = bfd_asymbol_name (*q->sym_ptr_ptr);
                    if (sym_name != NULL && *sym_name != '\0')
                        objdump_print_symname (aux->abfd, inf, *q->sym_ptr_ptr);
                    else
                    {
                        asection *sym_sec;

                        sym_sec = bfd_get_section (*q->sym_ptr_ptr);
                        sym_name = bfd_get_section_name (aux->abfd, sym_sec);
                        if (sym_name == NULL || *sym_name == '\0')
                            sym_name = "*unknown*";
                        printf ("%s", sym_name);
                    }
                }

                if (q->addend)
                {
                    bfd_signed_vma addend = q->addend;
                    if (addend < 0)
                    {
                        printf ("-0x");
                        addend = -addend;
                    }
                    else
                        printf ("+0x");
                    objdump_print_value (addend, inf, TRUE);
                }

                printf ("\n");
                need_nl = FALSE;
            }
            ++(*relppp);
        }

        if (need_nl)
            printf ("\n");

        addr_offset += octets / opb;
    }

    free (sfile.buffer);
}

static void disassemble_section (bfd *abfd, asection *section, void *inf)
{
    const struct elf_backend_data * bed;
    bfd_vma                      sign_adjust = 0;
    struct disassemble_info *    pinfo = (struct disassemble_info *) inf;
    struct objdump_disasm_info * paux;
    unsigned int                 opb = pinfo->octets_per_byte;
    bfd_byte *                   data = NULL;
    bfd_size_type                datasize = 0;
    arelent **                   rel_pp = NULL;
    arelent **                   rel_ppstart = NULL;
    arelent **                   rel_ppend;
    bfd_vma                      stop_offset;
    asymbol *                    sym = NULL;
    long                         place = 0;
    long                         rel_count;
    bfd_vma                      rel_offset;
    unsigned long                addr_offset;

    /* Sections that do not contain machine
       code are not normally disassembled.  */
    if (! disassemble_all
            && only_list == NULL
            && ((section->flags & (SEC_CODE | SEC_HAS_CONTENTS))
                != (SEC_CODE | SEC_HAS_CONTENTS)))
        return;

    if (! process_section_p (section))
        return;

    datasize = bfd_get_section_size (section);
    if (datasize == 0)
        return;

    if (start_address == (bfd_vma) -1
            || start_address < section->vma)
        addr_offset = 0;
    else
        addr_offset = start_address - section->vma;

    if (stop_address == (bfd_vma) -1)
        stop_offset = datasize / opb;
    else
    {
        if (stop_address < section->vma)
            stop_offset = 0;
        else
            stop_offset = stop_address - section->vma;
        if (stop_offset > datasize / opb)
            stop_offset = datasize / opb;
    }

    if (addr_offset >= stop_offset)
        return;

    /* Decide which set of relocs to use.  Load them if necessary.  */
    paux = (struct objdump_disasm_info *) pinfo->application_data;
    if (paux->dynrelbuf)
    {
        rel_pp = paux->dynrelbuf;
        rel_count = paux->dynrelcount;
        /* Dynamic reloc addresses are absolute, non-dynamic are section
        relative.  REL_OFFSET specifies the reloc address corresponding
         to the start of this section.  */
        rel_offset = section->vma;
    }
    else
    {
        rel_count = 0;
        rel_pp = NULL;
        rel_offset = 0;

        if ((section->flags & SEC_RELOC) != 0
                && (dump_reloc_info || pinfo->disassembler_needs_relocs))
        {
            long relsize;

            relsize = bfd_get_reloc_upper_bound (abfd, section);
            if (relsize < 0)
                bfd_fatal (bfd_get_filename (abfd));

            if (relsize > 0)
            {
                rel_ppstart = rel_pp = (arelent **) malloc (relsize);
                rel_count = bfd_canonicalize_reloc (abfd, section, rel_pp, syms);
                if (rel_count < 0)
                    bfd_fatal (bfd_get_filename (abfd));

                /* Sort the relocs by address.  */
                qsort (rel_pp, rel_count, sizeof (arelent *), compare_relocs);
            }
        }
    }
    rel_ppend = rel_pp + rel_count;

    data = (bfd_byte *) malloc (datasize);

    bfd_get_section_contents (abfd, section, data, 0, datasize);

    paux->sec = section;
    pinfo->buffer = data;
    pinfo->buffer_vma = section->vma;
    pinfo->buffer_length = datasize;
    pinfo->section = section;

    /* Skip over the relocs belonging to addresses below the
       start address.  */
    while (rel_pp < rel_ppend
            && (*rel_pp)->address < rel_offset + addr_offset)
        ++rel_pp;

    printf ("\nDisassembly of section %s:\n", section->name);

    /* Find the nearest symbol forwards from our current position.  */
    paux->require_sec = TRUE;
    sym = (asymbol *) find_symbol_for_address (section->vma + addr_offset,
            (struct disassemble_info *) inf,
            &place);
    paux->require_sec = FALSE;

    /* PR 9774: If the target used signed addresses then we must make
       sure that we sign extend the value that we calculate for 'addr'
       in the loop below.  */
    if (bfd_get_flavour (abfd) == bfd_target_elf_flavour
            && (bed = get_elf_backend_data (abfd)) != NULL
            && bed->sign_extend_vma)
        sign_adjust = (bfd_vma) 1 << (bed->s->arch_size - 1);

    /* Disassemble a block of instructions up to the address associated with
       the symbol we have just found.  Then print the symbol and find the
       next symbol on.  Repeat until we have disassembled the entire section
       or we have reached the end of the address range we are interested in.  */
    while (addr_offset < stop_offset)
    {
        bfd_vma addr;
        asymbol *nextsym;
        bfd_vma nextstop_offset;
        bfd_boolean insns;

        addr = section->vma + addr_offset;
        addr = ((addr & ((sign_adjust << 1) - 1)) ^ sign_adjust) - sign_adjust;

        if (sym != NULL && bfd_asymbol_value (sym) <= addr)
        {
            int x;

            for (x = place;
                    (x < sorted_symcount
                     && (bfd_asymbol_value (sorted_syms[x]) <= addr));
                    ++x)
                continue;

            pinfo->symbols = sorted_syms + place;
            pinfo->num_symbols = x - place;
            pinfo->symtab_pos = place;
        }
        else
        {
            pinfo->symbols = NULL;
            pinfo->num_symbols = 0;
            pinfo->symtab_pos = -1;
        }

        if (! prefix_addresses)
        {
            pinfo->fprintf_func (pinfo->stream, "\n");
            objdump_print_addr_with_sym (abfd, section, sym, addr,
                                         pinfo, FALSE);
            pinfo->fprintf_func (pinfo->stream, ":\n");
        }

        if (sym != NULL && bfd_asymbol_value (sym) > addr)
            nextsym = sym;
        else if (sym == NULL)
            nextsym = NULL;
        else
        {
#define is_valid_next_sym(SYM) \
  ((SYM)->section == section \
   && (bfd_asymbol_value (SYM) > bfd_asymbol_value (sym)) \
   && pinfo->symbol_is_valid (SYM, pinfo))

            /* Search forward for the next appropriate symbol in
               SECTION.  Note that all the symbols are sorted
               together into one big array, and that some sections
               may have overlapping addresses.  */
            while (place < sorted_symcount
                    && ! is_valid_next_sym (sorted_syms [place]))
                ++place;

            if (place >= sorted_symcount)
                nextsym = NULL;
            else
                nextsym = sorted_syms[place];
        }

        if (sym != NULL && bfd_asymbol_value (sym) > addr)
            nextstop_offset = bfd_asymbol_value (sym) - section->vma;
        else if (nextsym == NULL)
            nextstop_offset = stop_offset;
        else
            nextstop_offset = bfd_asymbol_value (nextsym) - section->vma;

        if (nextstop_offset > stop_offset
                || nextstop_offset <= addr_offset)
            nextstop_offset = stop_offset;

        /* If a symbol is explicitly marked as being an object
        rather than a function, just dump the bytes without
         disassembling them.  */
        if (disassemble_all
                || sym == NULL
                || sym->section != section
                || bfd_asymbol_value (sym) > addr
                || ((sym->flags & BSF_OBJECT) == 0
                    && (strstr (bfd_asymbol_name (sym), "gnu_compiled")
                        == NULL)
                    && (strstr (bfd_asymbol_name (sym), "gcc2_compiled")
                        == NULL))
                || (sym->flags & BSF_FUNCTION) != 0)
            insns = TRUE;
        else
            insns = FALSE;

        disassemble_bytes (pinfo, paux->disassemble_fn, insns, data,
                           addr_offset, nextstop_offset,
                           rel_offset, &rel_pp, rel_ppend);

        addr_offset = nextstop_offset;
        sym = nextsym;
    }

    free (data);

    if (rel_ppstart != NULL)
        free (rel_ppstart);
}

/* Disassemble the contents of an object file.  */

static void disassemble_data (bfd *abfd)
{
    struct disassemble_info disasm_info;
    struct objdump_disasm_info aux;
    long i;

    print_files = NULL;
    prev_functionname = NULL;
    prev_line = -1;
    prev_discriminator = 0;

    /* We make a copy of syms to sort.  We don't want to sort syms
       because that will screw up the relocs.  */
    sorted_symcount = symcount ? symcount : dynsymcount;
    sorted_syms = (asymbol **) malloc ((sorted_symcount + synthcount)
                                       * sizeof (asymbol *));
    memcpy (sorted_syms, symcount ? syms : dynsyms,
            sorted_symcount * sizeof (asymbol *));

    sorted_symcount = remove_useless_symbols (sorted_syms, sorted_symcount);

    for (i = 0; i < synthcount; ++i)
    {
        sorted_syms[sorted_symcount] = synthsyms + i;
        ++sorted_symcount;
    }

    /* Sort the symbols into section and symbol order.  */
    qsort (sorted_syms, sorted_symcount, sizeof (asymbol *), compare_symbols);

    init_disassemble_info (&disasm_info, stdout, (fprintf_ftype) fprintf);

    disasm_info.application_data = (void *) &aux;
    aux.abfd = abfd;
    aux.require_sec = FALSE;
    aux.dynrelbuf = NULL;
    aux.dynrelcount = 0;
    aux.reloc = NULL;

    disasm_info.print_address_func = objdump_print_address;
    disasm_info.symbol_at_address_func = objdump_symbol_at_address;

    if (machine != NULL)
    {
        const bfd_arch_info_type *inf = bfd_scan_arch (machine);

        if (inf == NULL)
            fatal ("can't use supplied machine %s", machine);

        abfd->arch_info = inf;
    }

    if (endian != BFD_ENDIAN_UNKNOWN)
    {
        struct bfd_target *xvec;

        xvec = (struct bfd_target *) malloc (sizeof (struct bfd_target));
        memcpy (xvec, abfd->xvec, sizeof (struct bfd_target));
        xvec->byteorder = endian;
        abfd->xvec = xvec;
    }

    /* Use libopcodes to locate a suitable disassembler.  */
    aux.disassemble_fn = disassembler (abfd);
    if (!aux.disassemble_fn)
    {
        non_fatal ("can't disassemble for architecture %s\n",
                   bfd_printable_arch_mach (bfd_get_arch (abfd), 0));
        return;
    }

    disasm_info.flavour = bfd_get_flavour (abfd);
    disasm_info.arch = bfd_get_arch (abfd);
    disasm_info.mach = bfd_get_mach (abfd);
    disasm_info.disassembler_options = disassembler_options;
    disasm_info.octets_per_byte = bfd_octets_per_byte (abfd);
    disasm_info.skip_zeroes = DEFAULT_SKIP_ZEROES;
    disasm_info.skip_zeroes_at_end = DEFAULT_SKIP_ZEROES_AT_END;
    disasm_info.disassembler_needs_relocs = FALSE;

    if (bfd_big_endian (abfd))
        disasm_info.display_endian = disasm_info.endian = BFD_ENDIAN_BIG;
    else if (bfd_little_endian (abfd))
        disasm_info.display_endian = disasm_info.endian = BFD_ENDIAN_LITTLE;
    else
        /* ??? Aborting here seems too drastic.  We could default to big or little
           instead.  */
        disasm_info.endian = BFD_ENDIAN_UNKNOWN;

    /* Allow the target to customize the info structure.  */
    disassemble_init_for_target (& disasm_info);

    /* Pre-load the dynamic relocs if we are going
       to be dumping them along with the disassembly.  */
    if (dump_dynamic_reloc_info)
    {
        long relsize = bfd_get_dynamic_reloc_upper_bound (abfd);

        if (relsize < 0)
            bfd_fatal (bfd_get_filename (abfd));

        if (relsize > 0)
        {
            aux.dynrelbuf = (arelent **) malloc (relsize);
            aux.dynrelcount = bfd_canonicalize_dynamic_reloc (abfd,
                              aux.dynrelbuf,
                              dynsyms);
            if (aux.dynrelcount < 0)
                bfd_fatal (bfd_get_filename (abfd));

            /* Sort the relocs by address.  */
            qsort (aux.dynrelbuf, aux.dynrelcount, sizeof (arelent *),
                   compare_relocs);
        }
    }
    disasm_info.symtab = sorted_syms;
    disasm_info.symtab_size = sorted_symcount;

    bfd_map_over_sections (abfd, disassemble_section, & disasm_info);

    if (aux.dynrelbuf != NULL)
        free (aux.dynrelbuf);
    free (sorted_syms);
}

/* Display a section in hexadecimal format with associated characters.
   Each line prefixed by the zero padded address.  */

static void dump_section (bfd *abfd, asection *section, void *dummy ATTRIBUTE_UNUSED)
{
    bfd_byte *data = 0;
    bfd_size_type datasize;
    bfd_vma addr_offset;
    bfd_vma start_offset;
    bfd_vma stop_offset;
    unsigned int opb = bfd_octets_per_byte (abfd);
    /* Bytes per line.  */
    const int onaline = 16;
    char buf[64];
    int count;
    int width;

    if ((section->flags & SEC_HAS_CONTENTS) == 0)
        return;

    if (! process_section_p (section))
        return;

    if ((datasize = bfd_section_size (abfd, section)) == 0)
        return;

    /* Compute the address range to display.  */
    if (start_address == (bfd_vma) -1
            || start_address < section->vma)
        start_offset = 0;
    else
        start_offset = start_address - section->vma;

    if (stop_address == (bfd_vma) -1)
        stop_offset = datasize / opb;
    else
    {
        if (stop_address < section->vma)
            stop_offset = 0;
        else
            stop_offset = stop_address - section->vma;

        if (stop_offset > datasize / opb)
            stop_offset = datasize / opb;
    }

    if (start_offset >= stop_offset)
        return;

    printf ("Contents of section %s:", section->name);

    if (!bfd_get_full_section_contents (abfd, section, &data))
    {
        non_fatal ("Reading section %s failed because: %s",
                   section->name, bfd_errmsg (bfd_get_error ()));
        return;
    }

    width = 4;

    bfd_sprintf_vma (abfd, buf, start_offset + section->vma);
    if (strlen (buf) >= sizeof (buf))
        abort ();

    count = 0;
    while (buf[count] == '0' && buf[count+1] != '\0')
        count++;
    count = strlen (buf) - count;
    if (count > width)
        width = count;

    bfd_sprintf_vma (abfd, buf, stop_offset + section->vma - 1);
    if (strlen (buf) >= sizeof (buf))
        abort ();

    count = 0;
    while (buf[count] == '0' && buf[count+1] != '\0')
        count++;
    count = strlen (buf) - count;
    if (count > width)
        width = count;

    for (addr_offset = start_offset;
            addr_offset < stop_offset; addr_offset += onaline / opb)
    {
        bfd_size_type j;

        bfd_sprintf_vma (abfd, buf, (addr_offset + section->vma));
        count = strlen (buf);
        if ((size_t) count >= sizeof (buf))
            abort ();

        putchar (' ');
        while (count < width)
        {
            putchar ('0');
            count++;
        }
        fputs (buf + count - width, stdout);
        putchar (' ');

        for (j = addr_offset * opb;
                j < addr_offset * opb + onaline; j++)
        {
            if (j < stop_offset * opb)
                printf ("%02x", (unsigned) (data[j]));
            else
                printf ("  ");
            if ((j & 3) == 3)
                printf (" ");
        }

        printf (" ");
        for (j = addr_offset * opb;
                j < addr_offset * opb + onaline; j++)
        {
            if (j >= stop_offset * opb)
                printf (" ");
            else
                printf ("%c", ISPRINT (data[j]) ? data[j] : '.');
        }
        putchar ('\n');
    }
    free (data);
}

/* Actually display the various requested regions.  */

static void dump_data (bfd *abfd)
{
    bfd_map_over_sections (abfd, dump_section, NULL);
}


static void dump_symbols (bfd *abfd ATTRIBUTE_UNUSED, bfd_boolean dynamic)
{
    asymbol **current;
    long max_count;
    long count;

    if (dynamic)
    {
        current = dynsyms;
        max_count = dynsymcount;
        printf ("DYNAMIC UNDEFINED SYMBOLS TABLE:\n");
    }
    else
    {
        current = syms;
        max_count = symcount;
        printf ("UNDEFINED SYMBOLS TABLE:\n");
    }

    if (max_count == 0)
        printf ("no symbols\n");

    for (count = 0; count < max_count; count++)
    {
        bfd *cur_bfd;

        if (*current == NULL)
            printf ("no information for symbol number %ld\n", count);

        else if ((cur_bfd = bfd_asymbol_bfd (*current)) == NULL)
            printf ("could not determine the type of symbol number %ld\n",
                    count);

        else if (process_section_p ((* current)->section)
                 && (dump_special_syms
                     || !bfd_is_target_special_symbol (cur_bfd, *current)))
        {
            const char *name = (*current)->name;
            symvalue val = (*current)->value;

            if (bfd_is_und_section((*current)->section))
            {
                printf("SYMBOL: %s    VALUE: %ld\n", name, val);
                bfd_print_symbol (cur_bfd, stdout, *current,
                              bfd_print_symbol_all);
                printf ("\n");
            }
        }

        current++;
    }
    printf ("\n\n");
}

/* Read in the dynamic symbols.  */

static asymbol **slurp_dynamic_symtab (bfd *abfd)
{
    asymbol **sy = NULL;
    long storage;

    storage = bfd_get_dynamic_symtab_upper_bound (abfd);
    if (storage < 0)
    {
        if (!(bfd_get_file_flags (abfd) & DYNAMIC))
        {
            non_fatal ("%s: not a dynamic object", bfd_get_filename (abfd));
            dynsymcount = 0;
            return NULL;
        }

        bfd_fatal (bfd_get_filename (abfd));
    }
    if (storage)
        sy = (asymbol **) malloc (storage);

    dynsymcount = bfd_canonicalize_dynamic_symtab (abfd, sy);
    if (dynsymcount < 0)
        bfd_fatal (bfd_get_filename (abfd));
    return sy;
}

static void dump_reloc_set (bfd *abfd, asection *sec, arelent **relpp, long relcount)
{
    arelent **p;
    char *last_filename, *last_functionname;
    /*unsigned int last_line;
    unsigned int last_discriminator;*/

    /* Get column headers lined up reasonably.  */
    {
        static int width;

        if (width == 0)
        {
            char buf[30];

            bfd_sprintf_vma (abfd, buf, (bfd_vma) -1);
            width = strlen (buf) - 7;
        }
        printf ("OFFSET %*s TYPE %*s VALUE \n", width, "", 12, "");
    }

    last_filename = NULL;
    last_functionname = NULL;
    /*last_line = 0;
    last_discriminator = 0;*/

    for (p = relpp; relcount && *p != NULL; p++, relcount--)
    {
        arelent *q = *p;
        /*const char *filename, *functionname;
        unsigned int linenumber;
        unsigned int discriminator;*/
        const char *sym_name;
        const char *section_name;
        bfd_vma addend2 = 0;

        if (start_address != (bfd_vma) -1
                && q->address < start_address)
            continue;
        if (stop_address != (bfd_vma) -1
                && q->address > stop_address)
            continue;

        if (q->sym_ptr_ptr && *q->sym_ptr_ptr)
        {
            sym_name = (*(q->sym_ptr_ptr))->name;
            section_name = (*(q->sym_ptr_ptr))->section->name;
        }
        else
        {
            sym_name = NULL;
            section_name = NULL;
        }

        bfd_printf_vma (abfd, q->address);
        if (q->howto == NULL)
            printf (" *unknown*         ");
        else if (q->howto->name)
        {
            const char *name = q->howto->name;

            /* R_SPARC_OLO10 relocations contain two addends.
               But because 'arelent' lacks enough storage to
               store them both, the 64-bit ELF Sparc backend
               records this as two relocations.  One R_SPARC_LO10
               and one R_SPARC_13, both pointing to the same
               address.  This is merely so that we have some
               place to store both addend fields.

               Undo this transformation, otherwise the output
               will be confusing.  */
            if (abfd->xvec->flavour == bfd_target_elf_flavour
                    && elf_tdata(abfd)->elf_header->e_machine == EM_SPARCV9
                    && relcount > 1
                    && !strcmp (q->howto->name, "R_SPARC_LO10"))
            {
                arelent *q2 = *(p + 1);
                if (q2 != NULL
                        && q2->howto
                        && q->address == q2->address
                        && !strcmp (q2->howto->name, "R_SPARC_13"))
                {
                    name = "R_SPARC_OLO10";
                    addend2 = q2->addend;
                    p++;
                }
            }
            printf (" %-16s  ", name);
        }
        else
            printf (" %-16d  ", q->howto->type);

        if (sym_name)
        {
            objdump_print_symname (abfd, NULL, *q->sym_ptr_ptr);
        }
        else
        {
            if (section_name == NULL)
                section_name = "*unknown*";
            printf ("[%s]", section_name);
        }

        if (q->addend)
        {
            bfd_signed_vma addend = q->addend;
            if (addend < 0)
            {
                printf ("-0x");
                addend = -addend;
            }
            else
                printf ("+0x");
            bfd_printf_vma (abfd, addend);
        }
        if (addend2)
        {
            printf ("+0x");
            bfd_printf_vma (abfd, addend2);
        }

        printf ("\n");
    }

    if (last_filename != NULL)
        free (last_filename);
    if (last_functionname != NULL)
        free (last_functionname);
}

static void dump_relocs_in_section (bfd *abfd,
                                    asection *section,
                                    void *dummy ATTRIBUTE_UNUSED)
{
    arelent **relpp;
    long relcount;
    long relsize;

    if (   bfd_is_abs_section (section)
            || bfd_is_und_section (section)
            || bfd_is_com_section (section)
            || (! process_section_p (section))
            || ((section->flags & SEC_RELOC) == 0))
        return;

    relsize = bfd_get_reloc_upper_bound (abfd, section);
    if (relsize < 0)
        bfd_fatal (bfd_get_filename (abfd));

    printf ("RELOCATION RECORDS FOR [%s]:", section->name);

    if (relsize == 0)
    {
        printf (" (none)\n\n");
        return;
    }

    relpp = (arelent **) malloc (relsize);
    relcount = bfd_canonicalize_reloc (abfd, section, relpp, syms);

    if (relcount < 0)
    {
        printf ("\n");
        non_fatal ("failed to read relocs in: %s", bfd_get_filename (abfd));
        bfd_fatal ("error message was");
    }
    else if (relcount == 0)
        printf (" (none)\n\n");
    else
    {
        printf ("\n");
        dump_reloc_set (abfd, section, relpp, relcount);
        printf ("\n\n");
    }
    free (relpp);
}

static void dump_relocs (bfd *abfd)
{
    bfd_map_over_sections (abfd, dump_relocs_in_section, NULL);
}

static void dump_dynamic_relocs (bfd *abfd)
{
    long relsize;
    arelent **relpp;
    long relcount;

    relsize = bfd_get_dynamic_reloc_upper_bound (abfd);
    if (relsize < 0)
        bfd_fatal (bfd_get_filename (abfd));

    printf ("DYNAMIC RELOCATION RECORDS");

    if (relsize == 0)
        printf (" (none)\n\n");
    else
    {
        relpp = (arelent **) malloc (relsize);
        relcount = bfd_canonicalize_dynamic_reloc (abfd, relpp, dynsyms);

        if (relcount < 0)
            bfd_fatal (bfd_get_filename (abfd));
        else if (relcount == 0)
            printf (" (none)\n\n");
        else
        {
            printf ("\n");
            dump_reloc_set (abfd, NULL, relpp, relcount);
            printf ("\n\n");
        }
        free (relpp);
    }
}

/**
 * ACTUAL BEGINING
 */

static const string filename = "ELFDetective";
static ELFFile *exefile = nullptr;
static vector<ELFFile *> objfiles;
static unordered_map<string, addrbind> addrbinds;

static void check_symbols (ELFFile *file, bfd_boolean dynamic)
{
    asymbol **current;
    long max_count;
    long count;

    if (dynamic) {
        current = dynsyms;
        max_count = dynsymcount;
        cout << "DYNAMIC UNDEFINED SYMBOLS TABLE:" << endl;
    } else {
        current = file->getSyms();
        max_count = file->getSymcount();
        cout << "UNDEFINED SYMBOLS TABLE:" << endl;
    }

    if (max_count == 0)
        cout << "no symbols" << endl;

    for (count = 0; count < max_count; ++count) {
        bfd *cur_bfd;

        if (*current == NULL)
            cout << "no information for symbol number " << count << endl;

        else if ((cur_bfd = bfd_asymbol_bfd (*current)) == NULL)
            cout << "could not determine the type of symbol number " << count
                 << endl;

        else if (process_section_p ((* current)->section)
                 && (dump_special_syms
                     || !bfd_is_target_special_symbol (cur_bfd, *current))) {

            if (bfd_is_und_section((*current)->section)) {
                bfd_print_symbol (cur_bfd, stdout, *current,
                              bfd_print_symbol_all);
                cout << endl;
            }
        }

        current++;
    }

    cout << endl << endl;
}

static void slurp_symtab (ELFFile *file)
{
    asymbol **sy = NULL;
    bfd *abfd = file->getBfd();
    long storage;

    if (!(bfd_get_file_flags (abfd) & HAS_SYMS)) {
        file->setSymcount(0);
        file->setSyms(NULL);
        return;
    }

    storage = bfd_get_symtab_upper_bound (abfd);
    if (storage < 0) {
        cerr << "failed to read symbol table from: "
             << bfd_get_filename (abfd)
             << endl;
        bfd_fatal ("error message was");
    }

    if (storage)
        sy = (asymbol **) malloc (storage);

    file->setSymcount(bfd_canonicalize_symtab (abfd, sy));
    if (symcount < 0)
        bfd_fatal (bfd_get_filename (abfd));

    file->setSyms(sy);
}

static void gather_symbols (ELFFile *file)
{
    bfd *abfd = file->getBfd();

    cout << endl << bfd_get_filename (abfd) << ":     file format "
         << abfd->xvec->name << endl;

    slurp_symtab (file);
}


static void prepare_file(ELFFile *file)
{
    char **matching;
    bfd *abfd = file->getBfd();

    if (!bfd_check_format_matches (abfd, bfd_object, &matching) ||
        !bfd_check_format_matches (abfd, bfd_core, &matching)) {
        gather_symbols (file);
        return;
    }

    cerr << "ERROR: Invallid file!" << endl;
}

static void print_usage()
{
    cerr << "Usage: elfdetective -obj obj1 ... objn -exe exec"
         << endl
         << "At least one file is required."
         << endl;
    exit(0);
}

static void parse_args(int argc, char **args)
{
    string arg;
    bool is_obj = false;
    bool is_exe = false;

    if (argc < 2)
        print_usage();

    for (int i = 1; i < argc; ++i) {
        arg = args[i];

        if (arg == "-obj") {
            is_obj = true;
            is_exe = false;
            continue;
        }
        if (arg == "-exe") {
            is_obj = false;
            is_exe = true;
            continue;
        }

        if (is_obj)
            objfiles.push_back(new ELFFile(arg));

        if (is_exe) {
            if (exefile != nullptr) {
                print_usage();
                exit(0);
            }

            exefile = new ELFFile(arg);
        }
    }
}

/*
 * Might need think of a better way to parse the files.
 */

static void find_bindings()
{
    char buf[30];
    symvalue dec_value;
    asymbol **current;
    addrbind ab;

    // Parse each object file looking for undefined values
    for (ELFFile *E : objfiles) {
         current = E->getSyms();

        for (int i = 0; i < E->getSymcount(); ++i) {
            bfd *cur_bfd;

            if (*current == NULL)
                continue;

            else if ((cur_bfd = bfd_asymbol_bfd (*current)) == NULL)
                continue;

            else if (process_section_p ((* current)->section)
                     && (dump_special_syms
                         || !bfd_is_target_special_symbol (cur_bfd, *current))) {

                if (bfd_is_und_section((*current)->section)) {
                    ab.name = bfd_asymbol_name(*current);

                    ab.undefined_in = E->getName();
                    dec_value = bfd_asymbol_value(*current);

                    bfd_sprintf_vma(cur_bfd, buf, dec_value);
                    ab.undef_value = buf;

                    ab.defined_in = "UNK";
                    ab.exe_name = exefile->getName();

                    addrbinds[ab.name] = ab;
                }
            }

            current++;
        }

    }

    // Parse each object file looking for definitions of what we found
    for (ELFFile *E : objfiles) {
        current = E->getSyms();

        for (int i = 0; i < E->getSymcount(); ++i) {
            bfd *cur_bfd;
            if (*current == NULL)
                continue;

            else if ((cur_bfd = bfd_asymbol_bfd (*current)) == NULL)
                continue;

            else if (process_section_p ((* current)->section)
                     && (dump_special_syms
                         || !bfd_is_target_special_symbol (cur_bfd, *current))) {

                if (!bfd_is_und_section((*current)->section)) {
                    string name = (*current)->name;
                    auto it = addrbinds.find(name);

                    if (it == addrbinds.end()) {
                        current++;
                        continue;
                    }

                    ab = it->second;
                    ab.defined_in = E->getName();
                    dec_value = bfd_asymbol_value(*current);

                    bfd_sprintf_vma(cur_bfd, buf, dec_value);
                    ab.def_value = buf;

                    addrbinds[name] = ab;
                }
            }

            current++;
        }
    }
    // Parse symbols from exefile
    current = exefile->getSyms();
    for (int i = 0; i < exefile ->getSymcount(); ++i) {
        bfd *cur_bfd;
        if (*current == NULL)
            continue;

        else if ((cur_bfd = bfd_asymbol_bfd (*current)) == NULL)
            continue;

        else if (process_section_p ((* current)->section)
                 && (dump_special_syms
                     || !bfd_is_target_special_symbol (cur_bfd, *current))) {

            if (!bfd_is_und_section((*current)->section)) {
                string name = (*current)->name;
                auto it = addrbinds.find(name);

                if (it == addrbinds.end()) {
                    current++;
                    continue;
                }

                ab = it->second;
                dec_value = bfd_asymbol_value(*current);

                bfd_sprintf_vma(cur_bfd, buf, dec_value);
                ab.exe_value = buf;

                addrbinds[name] = ab;
            }
        }

        current++;
    }

    cout << endl
         << endl;

    cout << setw(20) << left << "NAME";
    cout << setw(10) << left << "UNDEF_IN";
    cout << setw(20) << left << "VALUE";
    cout << setw(10) << left << "DEF_IN";
    cout << setw(20) << left << "VALUE";
    cout << setw(10) << left << "EXEC_FILE";
    cout << setw(20) << left << "VALUE";
    cout << endl;

    for (auto symbol : addrbinds) {
        if (symbol.second.defined_in == "UNK")
            continue;

        cout << setw(20) << left << symbol.first;
        cout << setw(10) << left << symbol.second.undefined_in;
        cout << setw(20) << left << symbol.second.undef_value;
        cout << setw(10) << left << symbol.second.defined_in;
        cout << setw(20) << left << symbol.second.def_value;
        cout << setw(10) << left << symbol.second.exe_name;
        cout << setw(20) << left << symbol.second.exe_value;
        cout << endl;
    }

}

static void cleanup()
{
    delete exefile;
    for (ELFFile *E : objfiles)
        delete E;

    if (syms)
    {
        free (syms);
        syms = NULL;
    }

    if (dynsyms)
    {
        free (dynsyms);
        dynsyms = NULL;
    }

    if (synthsyms)
    {
        free (synthsyms);
        synthsyms = NULL;
    }

    symcount = 0;
    dynsymcount = 0;
    synthcount = 0;
}

int main (int argc, char **argv)
{
    parse_args(argc, argv);

    bfd_init ();
    dump_symtab = TRUE;

    exefile->initBfd();

    for (ELFFile *E : objfiles)
        E->initBfd();

    prepare_file(exefile);

    for (ELFFile *E : objfiles)
        prepare_file(E);

    find_bindings();

    cleanup();

    return 0;
}
