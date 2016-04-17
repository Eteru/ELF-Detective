#include "disassemblemodule.h"

#include <algorithm>
#include <memory>

/* print to string */
static void Disassembly::print_to_string(std::string str, bool dis = false)
{
    if (dis)
        disassemble_line += str;
    else
        output += str;
}

/* sprintf to a "stream".  */
static int ATTRIBUTE_PRINTF_2 Disassembly::disassemble_print (SFILE *f, const char *format, ...)
{
    int final_n, n;
    std::string str;
    std::unique_ptr<char[]> formatted;
    va_list ap;

    n = strlen(format) * 2;

    while(1)
    {
        formatted.reset(new char[n]);
        strcpy(&formatted[0], format);

        va_start(ap, format);
        final_n = vsnprintf(&formatted[0], n, format, ap);
        va_end(ap);

        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }

    disassemble_line += std::string(formatted.get());

    return final_n;
}

/* Returns TRUE if the specified section should be dumped.  */

static bfd_boolean Disassembly::process_section_p (asection * section)
{
    std::string section_name = section->name;

    return std::find(sections.begin(), sections.end(), section_name) != sections.end();
}

/* Sort relocs into address order.  */

static int Disassembly::compare_relocs (const void *ap, const void *bp)
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

static void Disassembly::objdump_print_symname (bfd *abfd, struct disassemble_info *inf,
        asymbol *sym, bool dis = false)
{
    char *alloc;
    const char *name, *version_string = NULL;
    bfd_boolean hidden = FALSE;

    alloc = NULL;
    name = bfd_asymbol_name(sym);

    if (bfd_is_und_section(bfd_get_section(sym)))
        hidden = TRUE;

    if (inf != NULL)
    {
        print_to_string(name, dis);
        if (version_string && *version_string != '\0')
        {
            print_to_string(hidden ? "@%" : "@@", dis);
            print_to_string(version_string, dis);
        }

    }

    if (alloc != NULL)
        free(alloc);
}

/* Print an address and the offset to the nearest symbol.  */

static void Disassembly::objdump_print_addr_with_sym (bfd *abfd, asection *sec, asymbol *sym,
        bfd_vma vma, struct disassemble_info *inf, bool dis = false)
{
    char buf[100];

    objdump_print_value(vma, inf, dis);

    if (sym == NULL)
    {
        bfd_vma secaddr;

        print_to_string("<", dis);
        print_to_string(bfd_get_section_name(abfd, sec), dis);

        secaddr = bfd_get_section_vma(abfd, sec);
        if (vma < secaddr)
        {
            print_to_string("-0x", dis);
            objdump_print_value(secaddr - vma, inf, dis);
        }
        else if (vma > secaddr)
        {
            print_to_string("+0x", dis);
            objdump_print_value(vma - secaddr, inf, dis);
        }
        print_to_string(">", dis);
    }
    else
    {
        print_to_string(" <", dis);
        objdump_print_symname(abfd, inf, sym, dis);
        if (bfd_asymbol_value(sym) > vma)
        {
            print_to_string("-0x", dis);
            objdump_print_value(bfd_asymbol_value(sym) - vma, inf, dis);
        }
        else if (vma > bfd_asymbol_value(sym))
        {
            print_to_string("+0x", dis);
            objdump_print_value(vma - bfd_asymbol_value(sym), inf, dis);
        }
        print_to_string(">", dis);
    }

    /*sprintf(buf, " (File Offset: 0x%lx)", (long int)(sec->filepos + (vma - sec->vma)));
    print_to_string(buf, dis);*/
}

static asymbol *Disassembly::find_symbol_for_address (bfd_vma vma,
        struct disassemble_info *inf,
        long *place)
{
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

    aux = (struct objdump_disasm_info *)inf->application_data;
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

        if (bfd_asymbol_value(sym) > vma)
            max_count = thisplace;
        else if (bfd_asymbol_value(sym) < vma)
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
    while (thisplace > 0 && (bfd_asymbol_value(sorted_syms[thisplace])
                == bfd_asymbol_value(sorted_syms[thisplace - 1])))
        --thisplace;

    /* Prefer a symbol in the current section if we have multple symbols
       with the same value, as can occur with overlays or zero size
       sections.  */
    min = thisplace;
    while (min < max_count && (bfd_asymbol_value(sorted_syms[min])
                == bfd_asymbol_value(sorted_syms[thisplace])))
    {
        if (sorted_syms[min]->section == sec
                && inf->symbol_is_valid(sorted_syms[min], inf))
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
                        && vma >= bfd_get_section_vma(abfd, sec)
                        && vma < (bfd_get_section_vma(abfd, sec)
                                  + bfd_section_size(abfd, sec) / opb)));
    if ((sorted_syms[thisplace]->section != sec && want_section)
            || ! inf->symbol_is_valid(sorted_syms[thisplace], inf))
    {
        long i;
        long newplace = sorted_symcount;

        for (i = min - 1; i >= 0; i--)
        {
            if ((sorted_syms[i]->section == sec || !want_section)
                    && inf->symbol_is_valid(sorted_syms[i], inf))
            {
                if (newplace == sorted_symcount)
                    newplace = i;

                if (bfd_asymbol_value(sorted_syms[i])
                        != bfd_asymbol_value(sorted_syms[newplace]))
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
                        && inf->symbol_is_valid(sorted_syms[i], inf))
                {
                    thisplace = i;
                    break;
                }
            }
        }

        if ((sorted_syms[thisplace]->section != sec && want_section)
                || ! inf->symbol_is_valid(sorted_syms[thisplace], inf))
            /* There is no suitable symbol.  */
            return NULL;
    }

    if (place != NULL)
        *place = thisplace;

    return sorted_syms[thisplace];
}

/* Print an address (VMA) to the output stream in INFO.
   If SKIP_ZEROES is TRUE, omit leading zeroes.  */

static void Disassembly::objdump_print_value (bfd_vma vma, struct disassemble_info *inf, bool dis = false)
{
    char buf[30];
    struct objdump_disasm_info *aux;

    aux = (struct objdump_disasm_info *) inf->application_data;
    bfd_sprintf_vma(aux->abfd, buf, vma);

    print_to_string(buf, dis);
}


/* Print an address (VMA), symbolically if possible.
   If SKIP_ZEROES is TRUE, don't output leading zeroes.  */

void Disassembly::objdump_print_addr (bfd_vma vma,
                                     struct disassemble_info *inf)
{
    char buf[100];
    struct objdump_disasm_info *aux;
    asymbol *sym = NULL;
    bfd_boolean skip_find = FALSE;

    aux = (struct objdump_disasm_info *)inf->application_data;

    if (sorted_symcount < 1)
    {
        print_to_string("0x", true);
        objdump_print_value(vma, inf, true);

        /*sprintf(buf, " (File Offset: 0x%lx)", (long int)(aux->sec->filepos + (vma - aux->sec->vma)));
        print_to_string(buf, true);*/
        return;
    }

    if (aux->reloc != NULL
            && aux->reloc->sym_ptr_ptr != NULL
            && * aux->reloc->sym_ptr_ptr != NULL)
    {
        sym = * aux->reloc->sym_ptr_ptr;

        /* Adjust the vma to the reloc.  */
        vma += bfd_asymbol_value(sym);

        if (bfd_is_und_section(bfd_get_section(sym)))
            skip_find = TRUE;
    }

    if (!skip_find)
        sym = find_symbol_for_address(vma, inf, NULL);

    objdump_print_addr_with_sym(aux->abfd, aux->sec, sym, vma, inf, true);
}

/* Print VMA to INFO.  This function is passed to the disassembler
   routine.  */

void Disassembly::objdump_print_address (bfd_vma vma, struct disassemble_info *inf)
{
    objdump_print_addr(vma, inf);
}

/* Filter out (in place) symbols that are useless for disassembly.
   COUNT is the number of elements in SYMBOLS.
   Return the number of useful symbols.  */

static long Disassembly::remove_useless_symbols (asymbol **symbols, long count)
{
    asymbol **in_ptr = symbols, **out_ptr = symbols;

    while (--count >= 0)
    {
        asymbol *sym = *in_ptr++;

        if (sym->name == NULL || sym->name[0] == '\0')
            continue;
        if (sym->flags & (BSF_DEBUGGING | BSF_SECTION_SYM))
            continue;
        if (bfd_is_und_section(sym->section)
                || bfd_is_com_section(sym->section))
            continue;

        *out_ptr++ = sym;
    }
    return out_ptr - symbols;
}

/* Determine if the given address has a symbol associated with it.  */

static int Disassembly::objdump_symbol_at_address (bfd_vma vma, struct disassemble_info * inf)
{
    asymbol * sym;

    sym = find_symbol_for_address(vma, inf, NULL);

    return (sym != NULL && (bfd_asymbol_value(sym) == vma));
}

static void Disassembly::disassemble_bytes (struct disassemble_info * inf,
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
    int octets = opb;
    SFILE sfile;

    char hexdump[2];

    aux = (struct objdump_disasm_info *) inf->application_data;
    section = aux->sec;

    sfile.alloc = 120;
    sfile.buffer = (char *) malloc(sfile.alloc);
    sfile.pos = 0;

    if (insns)
        octets_per_line = 4;
    else
        octets_per_line = 16;

    /* Figure out how many characters to skip at the start of an
       address, to make the disassembly look nicer.  We discard leading
       zeroes in chunks of 4, ensuring that there is always a leading
       zero remaining.  */
    skip_addr_chars = 0;

    char buf[30];

    bfd_sprintf_vma(aux->abfd, buf, section->vma + section->size / opb);

    while (buf[skip_addr_chars] == '0')
        ++skip_addr_chars;

    /* Don't discard zeros on overflow.  */
    if (buf[skip_addr_chars] == '\0' && section->vma != 0)
        skip_addr_chars = 0;

    if (skip_addr_chars != 0)
        skip_addr_chars = (skip_addr_chars - 1) & -4;

    inf->insn_info_valid = 0;

    addr_offset = start_offset;
    while (addr_offset < stop_offset)
    {
        bfd_vma z;
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

        char buf[50];
        int bpc = 0;
        int pb = 0;

        char *s;

        bfd_sprintf_vma(aux->abfd, buf, section->vma + addr_offset);
        for (s = buf + skip_addr_chars; *s == '0'; s++)
            *s = ' ';
        if (*s == '\0')
            *--s = '0';

        print_to_string(buf + skip_addr_chars);
        print_to_string(":\t");

        if (insns)
        {
            sfile.pos = 0;

            inf->stream = &sfile;
            inf->bytes_per_line = 0;
            inf->bytes_per_chunk = 0;
            inf->flags = DISASSEMBLE_DATA;

            if (inf->disassembler_needs_relocs
                    && (bfd_get_file_flags(aux->abfd) & EXEC_P) == 0
                    && (bfd_get_file_flags(aux->abfd) & DYNAMIC) == 0
                    && *relppp < relppend)
            {
                bfd_signed_vma distance_to_rel;

                distance_to_rel = (**relppp)->address
                                  - (rel_offset + addr_offset);

                /* Check to see if the current reloc is associated with
                   the instruction that we are about to disassemble.  */
                if (distance_to_rel == 0 || (distance_to_rel > 0
                            && distance_to_rel < (bfd_signed_vma) (previous_octets/ opb)))
                {
                    inf->flags |= INSN_HAS_RELOC;
                    aux->reloc = **relppp;
                }
            }

            octets = (*disassemble_fn) (section->vma + addr_offset, inf);

            if (inf->bytes_per_line != 0)
                octets_per_line = inf->bytes_per_line;
            if (octets < (int) opb)
                break;
        }
        else
        {
            bfd_vma j;

            octets = octets_per_line;
            if (addr_offset + octets / opb > stop_offset)
                octets = (stop_offset - addr_offset) * opb;

            for (j = addr_offset * opb; j < addr_offset * opb + octets; ++j)
            {
                if (ISPRINT(data[j]))
                    buf[j - addr_offset * opb] = data[j];
                else
                    buf[j - addr_offset * opb] = '.';
            }
            buf[j - addr_offset * opb] = '\0';
        }
        bfd_vma j;

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
                {
                    sprintf(hexdump, "%02x", (unsigned) data[j + k]);
                    print_to_string(hexdump);
                }
                print_to_string(" ");
            }
            else
            {
                for (k = 0; k < bpc; k++)
                {
                    sprintf(hexdump, "%02x", (unsigned) data[j + k]);
                    print_to_string(hexdump);
                }
                print_to_string(" ");
            }
        }

        for (; pb < octets_per_line; pb += bpc)
        {
            int k;

            for (k = 0; k < bpc; k++)
                print_to_string(" ");
            print_to_string(" ");
        }

        /* Separate raw data from instruction by extra space.  */
        if (insns)
            print_to_string("\t");
        else
            print_to_string("    ");

        while (pb < octets)
        {
            bfd_vma j;
            char *s;

            print_to_string("\n");
            j = addr_offset * opb + pb;

            bfd_sprintf_vma(aux->abfd, buf, section->vma + j / opb);
            for (s = buf + skip_addr_chars; *s == '0'; s++)
                *s = ' ';
            if (*s == '\0')
                *--s = '0';

            print_to_string(buf + skip_addr_chars);
            print_to_string(":\t");

            pb += octets_per_line;
            if (pb > octets)
                pb = octets;
            for (; j < addr_offset * opb + pb; j += bpc)
            {
                int k;

                if (bpc > 1 && inf->display_endian == BFD_ENDIAN_LITTLE)
                {
                    for (k = bpc - 1; k >= 0; k--)
                    {
                        sprintf(hexdump, "%02x", (unsigned) data[j + k]);
                        print_to_string(hexdump);
                    }
                    print_to_string(" ");
                }
                else
                {
                    for (k = 0; k < bpc; k++)
                    {
                        sprintf(hexdump, "%02x", (unsigned) data[j + k]);
                        print_to_string(hexdump);
                    }
                    print_to_string(" ");
                }
            }
        }
        print_to_string(disassemble_line);
        disassemble_line = "";

        print_to_string("\n");

        while ((*relppp) < relppend
                && (**relppp)->address < rel_offset + addr_offset + octets / opb)
        {
            arelent *q;

            q = **relppp;

            print_to_string("\t\t\t");

            objdump_print_value(section->vma - rel_offset + q->address,
                                 inf);

            if (q->howto == NULL)
                print_to_string(": unkown*\t");
            else if (q->howto->name)
            {
                print_to_string(": ");
                print_to_string(q->howto->name);
                print_to_string("\t");
            }
            else
            {
                print_to_string(": ");
                print_to_string("" + q->howto->type);
                print_to_string("\t");
            }

            if (q->sym_ptr_ptr == NULL || *q->sym_ptr_ptr == NULL)
                print_to_string("*unkown*");
            else
            {
                const char *sym_name;

                sym_name = bfd_asymbol_name(*q->sym_ptr_ptr);
                if (sym_name != NULL && *sym_name != '\0')
                    objdump_print_symname(aux->abfd, inf, *q->sym_ptr_ptr);
                else
                {
                    asection *sym_sec;

                    sym_sec = bfd_get_section(*q->sym_ptr_ptr);
                    sym_name = bfd_get_section_name(aux->abfd, sym_sec);
                    if (sym_name == NULL || *sym_name == '\0')
                        sym_name = "*unknown*";
                    print_to_string(sym_name);
                }
            }

            if (q->addend)
            {
                bfd_signed_vma addend = q->addend;
                if (addend < 0)
                {
                    print_to_string("-0x");
                    addend = -addend;
                }
                else
                    print_to_string("+0x");
                objdump_print_value(addend, inf);
            }

            print_to_string("\n");

            ++(*relppp);
        }

        addr_offset += octets / opb;
    }

    free(sfile.buffer);
}

static void Disassembly::disassemble_section (bfd *abfd, asection *section, void *inf)
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
    if (! process_section_p (section))
        return;

    datasize = bfd_get_section_size(section);
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

        if ((section->flags & SEC_RELOC) != 0)
        {
            long relsize;

            relsize = bfd_get_reloc_upper_bound(abfd, section);
            if (relsize < 0)
                bfd_fatal(bfd_get_filename(abfd));

            if (relsize > 0)
            {
                rel_ppstart = rel_pp = (arelent **) malloc(relsize);
                rel_count = bfd_canonicalize_reloc(abfd, section, rel_pp, syms);
                if (rel_count < 0)
                    bfd_fatal(bfd_get_filename(abfd));

                /* Sort the relocs by address.  */
                qsort(rel_pp, rel_count, sizeof(arelent *), compare_relocs);
            }
        }
    }
    rel_ppend = rel_pp + rel_count;

    data = (bfd_byte *) malloc(datasize);

    bfd_get_section_contents(abfd, section, data, 0, datasize);

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

    print_to_string("\nDisassembly of section ");
    print_to_string(section->name);
    print_to_string(":\n");

    /* Find the nearest symbol forwards from our current position.  */
    paux->require_sec = TRUE;
    sym = (asymbol *) find_symbol_for_address(section->vma + addr_offset,
            (struct disassemble_info *) inf,
            &place);
    paux->require_sec = FALSE;

    /* PR 9774: If the target used signed addresses then we must make
       sure that we sign extend the value that we calculate for 'addr'
       in the loop below.  */
    if (bfd_get_flavour(abfd) == bfd_target_elf_flavour
            && (bed = get_elf_backend_data(abfd)) != NULL
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

        if (sym != NULL && bfd_asymbol_value(sym) <= addr)
        {
            int x;

            for (x = place;
                    (x < sorted_symcount
                     && (bfd_asymbol_value(sorted_syms[x]) <= addr));
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

        print_to_string("\n");
        objdump_print_addr_with_sym(abfd, section, sym, addr,
                                     pinfo);
        print_to_string(":\n");

        if (sym != NULL && bfd_asymbol_value(sym) > addr)
            nextsym = sym;
        else if (sym == NULL)
            nextsym = NULL;
        else
        {
#define is_valid_next_sym(SYM) \
  ((SYM)->section == section \
   && (bfd_asymbol_value(SYM) > bfd_asymbol_value(sym)) \
   && pinfo->symbol_is_valid(SYM, pinfo))

            /* Search forward for the next appropriate symbol in
               SECTION.  Note that all the symbols are sorted
               together into one big array, and that some sections
               may have overlapping addresses.  */
            while (place < sorted_symcount
                    && ! is_valid_next_sym(sorted_syms [place]))
                ++place;

            if (place >= sorted_symcount)
                nextsym = NULL;
            else
                nextsym = sorted_syms[place];
        }

        if (sym != NULL && bfd_asymbol_value(sym) > addr)
            nextstop_offset = bfd_asymbol_value(sym) - section->vma;
        else if (nextsym == NULL)
            nextstop_offset = stop_offset;
        else
            nextstop_offset = bfd_asymbol_value(nextsym) - section->vma;

        if (nextstop_offset > stop_offset
                || nextstop_offset <= addr_offset)
            nextstop_offset = stop_offset;

        /* If a symbol is explicitly marked as being an object
        rather than a function, just dump the bytes without
         disassembling them.  */

        insns = TRUE;

        disassemble_bytes(pinfo, paux->disassemble_fn, insns, data,
                           addr_offset, nextstop_offset,
                           rel_offset, &rel_pp, rel_ppend);

        addr_offset = nextstop_offset;
        sym = nextsym;
    }

    free(data);

    if (rel_ppstart != NULL)
        free(rel_ppstart);
}

void Disassembly::add_section (std::string sec)
{
    sections.push_back(sec);
}

/* Disassemble the contents of an object file.  */

std::string Disassembly::disassemble_data (ELFFile *E)
{
    struct disassemble_info disasm_info;
    struct objdump_disasm_info aux;
    long i;

    output = "";

    print_files = NULL;
    prev_functionname = NULL;
    prev_line = -1;
    prev_discriminator = 0;

    bfd *abfd = E->getBfd();
    syms = E->getSyms();
    symcount = E->getSymcount();
    dynsyms = E->getDSyms();
    dynsymcount = E->getDynSymcount();

    /* We make a copy of syms to sort.  We don't want to sort syms
       because that will screw up the relocs.  */
    sorted_symcount = symcount ? symcount : dynsymcount;
    sorted_syms = (asymbol **) malloc((sorted_symcount + synthcount)
                                       * sizeof(asymbol *));
    memcpy(sorted_syms, symcount ? syms : dynsyms,
            sorted_symcount * sizeof(asymbol *));

    sorted_symcount = remove_useless_symbols(sorted_syms, sorted_symcount);

    for (i = 0; i < synthcount; ++i)
    {
        sorted_syms[sorted_symcount] = synthsyms + i;
        ++sorted_symcount;
    }

    /* Sort the symbols into section and symbol order.  */
    qsort(sorted_syms, sorted_symcount, sizeof(asymbol *), compare_symbols);

    init_disassemble_info(&disasm_info, NULL, (fprintf_ftype) disassemble_print);
    disasm_info.application_data = (void *) &aux;

    aux.abfd = abfd;
    aux.require_sec = FALSE;
    aux.dynrelbuf = NULL;
    aux.dynrelcount = 0;
    aux.reloc = NULL;


    disasm_info.print_address_func = objdump_print_address;
    disasm_info.symbol_at_address_func = objdump_symbol_at_address;

    if (endian != BFD_ENDIAN_UNKNOWN)
    {
        struct bfd_target *xvec;

        xvec = (struct bfd_target *) malloc(sizeof(struct bfd_target));
        memcpy(xvec, abfd->xvec, sizeof(struct bfd_target));
        xvec->byteorder = endian;
        abfd->xvec = xvec;
    }

    /* Use libopcodes to locate a suitable disassembler.  */
    aux.disassemble_fn = disassembler(abfd);
    if (!aux.disassemble_fn)
    {
        std::cerr << "can't disassemble for architecture "
                  << bfd_printable_arch_mach(bfd_get_arch(abfd), 0) << std::endl;
        return output;
    }

    disasm_info.flavour = bfd_get_flavour(abfd);
    disasm_info.arch = bfd_get_arch(abfd);
    disasm_info.mach = bfd_get_mach(abfd);
    disasm_info.disassembler_options = NULL;
    disasm_info.octets_per_byte = bfd_octets_per_byte(abfd);
    disasm_info.skip_zeroes = DEFAULT_SKIP_ZEROES;
    disasm_info.skip_zeroes_at_end = DEFAULT_SKIP_ZEROES_AT_END;
    disasm_info.disassembler_needs_relocs = FALSE;

    if (bfd_big_endian(abfd))
        disasm_info.display_endian = disasm_info.endian = BFD_ENDIAN_BIG;
    else if (bfd_little_endian(abfd))
        disasm_info.display_endian = disasm_info.endian = BFD_ENDIAN_LITTLE;
    else
        /* ??? Aborting here seems too drastic.  We could default to big or little
           instead.  */
        disasm_info.endian = BFD_ENDIAN_UNKNOWN;

    /* Allow the target to customize the info structure.  */
    disassemble_init_for_target(& disasm_info);

    /* Pre-load the dynamic relocs if we are going
       to be dumping them along with the disassembly.  */

    long relsize = bfd_get_dynamic_reloc_upper_bound(abfd);

    if (relsize < 0)
        bfd_fatal(bfd_get_filename(abfd));

    if (relsize > 0)
    {
        aux.dynrelbuf = (arelent **) malloc(relsize);
        aux.dynrelcount = bfd_canonicalize_dynamic_reloc(abfd,
                          aux.dynrelbuf,
                          dynsyms);
        if (aux.dynrelcount < 0)
            bfd_fatal(bfd_get_filename(abfd));

        /* Sort the relocs by address.  */
        qsort(aux.dynrelbuf, aux.dynrelcount, sizeof(arelent *),
               compare_relocs);
    }
    disasm_info.symtab = sorted_syms;
    disasm_info.symtab_size = sorted_symcount;

    bfd_map_over_sections(abfd, disassemble_section, & disasm_info);

    if (aux.dynrelbuf != NULL)
        free(aux.dynrelbuf);
    free(sorted_syms);

    return output;
}
