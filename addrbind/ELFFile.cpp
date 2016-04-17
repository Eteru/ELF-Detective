#include "ELFFile.h"
#include "tools.h"

std::string ELFFile::getName()
{
    return this->filename;
}

void ELFFile::initBfd()
{
    if (get_file_size (this->filename.c_str()) < 1) {
        std::cerr << this->filename
                  << " is empty."
                  << std::endl;

        this->error = BFD_FILE_SIZE;
        return;
    }

    this->abfd = bfd_openr (this->filename.c_str(), NULL);
    this->abfd->flags |= BFD_DECOMPRESS;

    if (this->abfd == NULL) {
        std::cerr << this->filename
                  << " cannot be parsed."
                  << std::endl;

        this->error = BFD_FILE_NULL;
        return;
    }
}

bfd *ELFFile::getBfd() const
{
    return this->abfd;
}

asymbol *ELFFile::getSynthsyms() const
{
    return this->synthsyms;
}

asymbol **ELFFile::getSyms() const
{
    return this->syms;
}

asymbol **ELFFile::getDSyms() const
{
    return this->dynsyms;
}

asymbol **ELFFile::getSortedSyms() const
{
    return this->sorted_syms;
}

long ELFFile::getSymcount() const
{
    return this->symcount;
}

long ELFFile::getDynSymcount() const
{
    return this->dynsymcount;
}

long ELFFile::getSortedSymcount() const
{
    return this->sorted_symcount;
}

void ELFFile::setSyms(asymbol **syms)
{
    this->syms = syms;
}

void ELFFile::setDSyms(asymbol **dsyms)
{
    this->dynsyms = dsyms;
}

void ELFFile::setSynthsyms(asymbol *synthsyms)
{
    this->synthsyms = synthsyms;
}

void ELFFile::setSymcount(long symcount)
{
    this->symcount = symcount;
}

void ELFFile::setDynSymcount(long dynsymcount)
{
    this->dynsymcount = dynsymcount;
}

void ELFFile::setSortedSymcount()
{
    this->sorted_symcount = this->symcount ? this->symcount : this->dynsymcount;
}

void ELFFile::setSortedSymcount(long sorted_symcount)
{
    this->sorted_symcount = sorted_symcount;
}

void ELFFile::setSynthcount(long synthcount)
{
    this->synthcount = synthcount;
}

void ELFFile::initSortedSyms()
{
    this->sorted_syms = (asymbol **) malloc((this->sorted_symcount + this->synthcount)
                                       * sizeof (asymbol *));
    memcpy (this->sorted_syms, this->symcount ? this->syms : this->dynsyms,
            this->sorted_symcount * sizeof (asymbol *));

    std::cout << "INIT & COPY: DONE" << std::endl;

    this->remove_useless_symbols();

    std::cout << "REMOVED: DONE" << std::endl;

    for (int i = 0; i < this->synthcount; ++i)
    {
        this->sorted_syms[this->sorted_symcount] = this->synthsyms + i;
        this->sorted_symcount += 1;
    }

    std::cout << "synth: DONE" << std::endl;

    /* Sort the symbols into section and symbol order.  */
    qsort(this->sorted_syms, this->sorted_symcount, sizeof(asymbol *), compare_symbols);

    std::cout << "qsort: DONE" << std::endl;
}

void ELFFile::remove_useless_symbols ()
{
    long count = this->sorted_symcount;
    asymbol** in_ptr = this->sorted_syms;
    asymbol** out_ptr = this->sorted_syms;

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
    this->sorted_symcount = out_ptr - this->sorted_syms;
}

ELFFile::ELFFile() : symcount(0), dynsymcount(0), sorted_symcount(0), synthcount(0), error(0) {}

ELFFile::ELFFile(std::string fname)
: filename(fname), symcount(0), dynsymcount(0), sorted_symcount(0), synthcount(0), error(0) {}

ELFFile::~ELFFile()
{
    free (this->syms);
    free (this->dynsyms);
    free (this->sorted_syms);
    free (this->synthsyms);
    bfd_close (this->abfd);
}
