#ifndef ELFFILE_H
#define ELFFILE_H

#include <iostream>
#include <string>

#include <bfd.h>

// Error codes

#define BFD_FILE_SIZE 10001
#define BFD_FILE_NULL 10002

class ELFFile
{
    public:
        std::string getName();

        void initBfd();
        bfd* getBfd() const;

        asymbol *getSynthsyms() const;
        asymbol **getSyms() const;
        asymbol **getDSyms() const;
        asymbol **getSortedSyms() const;
        long getSymcount() const;
        long getDynSymcount() const;
        long getSortedSymcount() const;
        long getSynthcount() const;

        void setSyms(asymbol **);
        void setDSyms(asymbol **);
        void setSynthsyms(asymbol *);
        void setSymcount(long);
        void setDynSymcount(long);
        void setSortedSymcount();
        void setSortedSymcount(long);
        void setSynthcount(long);

        void initSortedSyms();
        void remove_useless_symbols ();

        ELFFile();
        ELFFile(std::string);
        virtual ~ELFFile();

    protected:
    private:
        std::string filename;

        bfd *abfd;
        /* The symbol table.  */
        asymbol **syms;
        /* The sorted symbol table.  */
        asymbol **sorted_syms;
        /* The dynamic symbol table.  */
        asymbol **dynsyms;

        /* Number of symbols in `syms'.  */
        long symcount;
        long dynsymcount;
        long sorted_symcount;

        /* The synthetic symbol table.  */
    asymbol *synthsyms;
        long synthcount;

        /* internal error code */
        int error;
};

#endif // ELFFILE_H
