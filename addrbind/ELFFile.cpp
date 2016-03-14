#include "ELFFile.h"
#include "tools.h"

#include <iostream>

std::string ELFFile::getName()
{
    return this->filename;
}

bfd* ELFFile::getBfd()
{
    return this->abfd;
}

asymbol** ELFFile::getSyms()
{
    return this->syms;
}
long ELFFile::getSymcount()
{
    return this->symcount;
}

void ELFFile::setSyms(asymbol **syms)
{
    this->syms = syms;
}

void ELFFile::setSymcount(long symcount)
{
    this->symcount = symcount;
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

    if (this->abfd == NULL) {
        std::cerr << this->filename
                  << " cannot be parsed."
                  << std::endl;

        this->error = BFD_FILE_NULL;
        return;
    }
}

ELFFile::ELFFile() : error(0), symcount(0) {}

ELFFile::ELFFile(std::string fname) : error(0), symcount(0), filename(fname) {}

ELFFile::~ELFFile()
{
    free (this->syms);
    bfd_close (this->abfd);
}
