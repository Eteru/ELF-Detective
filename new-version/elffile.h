#ifndef ELFFILE_H
#define ELFFILE_H

#include <iostream>
#include <vector>
#include <string>

#include <bfd.h>

const int BFD_FILE_SIZE = 10001;
const int BFD_FILE_NULL = 10002;
const int ELF_NOT_EXE = 10003;
const int ELF_NOT_OBJ = 10004;

const int ELF_EXE_FILE = 20001;
const int ELF_OBJ_FILE = 20002;
const int ELF_WRONG_TYPE = 20003;

class ELFFile
{
    public:
        std::string getName();

        int initBfd(int type);

        void slurp_dynamic_symtab();
        void slurp_symtab();
        void gather_symbols();

        bfd* getBfd() const;

        asymbol *getSynthsyms() const;
        asymbol **getSyms() const;
        asymbol **getDSyms() const;
        long getSymcount() const;
        long getDynSymcount() const;
        long getSynthcount() const;

        std::vector<std::string> getSymbolList() const;

        void setSyms(asymbol **);
        void setDSyms(asymbol **);
        void setSynthsyms(asymbol *);
        void setSymcount(long);
        void setDynSymcount(long);
        void setSynthcount(long);

        ELFFile();
        ELFFile(std::string);
        virtual ~ELFFile();

    protected:
    private:
        std::string filepath;
        std::string filename;

        bfd *abfd = nullptr;
        /* The symbol table.  */
        asymbol **syms = nullptr;
        /* The dynamic symbol table.  */
        asymbol **dynsyms = nullptr;
        /* The synthetic symbol table.  */
        asymbol *synthsyms = nullptr;

        /* Number of symbols in `syms'.  */
        long symcount;
        long dynsymcount;
        long synthcount;
};

#endif // ELFFILE_H
