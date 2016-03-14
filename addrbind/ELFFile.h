#ifndef ELFFILE_H
#define ELFFILE_H

#include <string>
#include <bfd.h>

// Error codes

#define BFD_FILE_SIZE 10001
#define BFD_FILE_NULL 10002

class ELFFile
{
    public:
        std::string getName();
        bfd* getBfd();

        asymbol** getSyms();
        long getSymcount();

        void setSyms(asymbol **);
        void setSymcount(long);

        void initBfd();

        ELFFile();
        ELFFile(std::string);
        virtual ~ELFFile();
    protected:
    private:
        std::string filename;

        bfd *abfd;
        /* The symbol table.  */
        asymbol **syms;

        /* Number of symbols in `syms'.  */
        long symcount;

        /* internal error code */
        int error;
};

#endif // ELFFILE_H
