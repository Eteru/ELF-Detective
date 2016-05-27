#include <sstream>

#include "elffile.h"
#include "tools.h"

std::string ELFFile::getName()
{
  return this->filename;
}

int ELFFile::initBfd(int type)
{
  if (get_file_size (this->filepath.c_str()) < 1)
    {
      return BFD_FILE_SIZE;
    }

  this->abfd = bfd_openr (this->filepath.c_str(), NULL);
  this->abfd->flags |= BFD_DECOMPRESS;

  if (this->abfd == NULL)
    {
      return BFD_FILE_NULL;
    }

  switch(type)
    {
    case ELF_EXE_FILE:
      if (!bfd_check_format(this->abfd, bfd_core))
        {
          if (!bfd_check_format(this->abfd, bfd_object))
            return ELF_NOT_EXE;
        }
      break;
    case ELF_OBJ_FILE:
      if (!bfd_check_format(this->abfd, bfd_object))
        {
          return ELF_NOT_OBJ;
        }
      break;
    default:
      break;
    }

  this->gather_symbols();

  return 0;
}

void ELFFile::slurp_dynamic_symtab()
{
  long storage;

  storage = bfd_get_dynamic_symtab_upper_bound(this->abfd);
  if (storage < 0)
    {
      if (!(bfd_get_file_flags (this->abfd) & DYNAMIC))
        {
          this->dynsymcount = 0;
          return;
        }
      bfd_fatal(bfd_get_filename(abfd));
    }

  if (storage)
    this->dynsyms = (asymbol **) malloc(storage);

  this->dynsymcount = bfd_canonicalize_dynamic_symtab(this->abfd, this->dynsyms);

  if (this->dynsymcount < 0)
    bfd_fatal (bfd_get_filename(this->abfd));

}

void ELFFile::slurp_symtab()
{
  long storage;

  if (!(bfd_get_file_flags (this->abfd) & HAS_SYMS))
    {
      this->symcount = 0;
      return;
    }

  storage = bfd_get_symtab_upper_bound(this->abfd);
  if (storage < 0)
    {
      bfd_fatal(bfd_get_filename(this->abfd));
    }

  if (storage)
    this->syms = (asymbol **) malloc(storage);

  this->symcount = bfd_canonicalize_symtab(this->abfd, this->syms);
  if (this->symcount < 0)
    bfd_fatal(bfd_get_filename(this->abfd));
}


void ELFFile::gather_symbols()
{
  slurp_symtab();
  slurp_dynamic_symtab();

  this->synthcount = bfd_get_synthetic_symtab(
        this->abfd, this->symcount, this->syms,
        this->dynsymcount, this->dynsyms, &this->synthsyms);

  if (this->synthcount < 0)
    this->synthcount = 0;

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

long ELFFile::getSymcount() const
{
  return this->symcount;
}

long ELFFile::getDynSymcount() const
{
  return this->dynsymcount;
}

long ELFFile::getSynthcount() const
{
  return this->synthcount;
}

std::vector<std::string> ELFFile::getSymbolList() const
{
  std::vector<std::string> symbols;
  asymbol **current;

  current = this->syms;
  for (int i = 0; i < this->symcount; ++i)
    {
      bfd *cur_bfd;
      if (*current == NULL)
        continue;

      else if ((cur_bfd = bfd_asymbol_bfd (*current)) == NULL)
        continue;

      else if (!bfd_is_target_special_symbol (cur_bfd, *current))
        {
          std::string name = (*current)->name;

          // ignore symbols starting with "."
          if (strncmp((*current)->name, ".", 1) != 0 &&
              !bfd_is_abs_section((*current)->section))
            symbols.push_back(name);

          current++;
        }
    }

  return symbols;
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

void ELFFile::setSynthcount(long synthcount)
{
  this->synthcount = synthcount;
}

void ELFFile::addFunction(Function *f)
{
  this->functions.push_back(f);
}

std::vector<Function *> ELFFile::getFunctions() const
{
  return this->functions;
}

void ELFFile::setView(QWidget *v)
{
  this->view = v;
}
QWidget *ELFFile::getView() const
{
  return this->view;
}

ELFFile::ELFFile() : symcount(0), dynsymcount(0), synthcount(0) {}

ELFFile::ELFFile(std::string fname)
  : filepath(fname), symcount(0), dynsymcount(0), synthcount(0)
{
  std::istringstream iss(fname);
  std::string token;

  while(std::getline(iss, token, '/'));

  this->filename = token;
}

ELFFile::~ELFFile()
{
  if (this->syms)
    {
      free(this->syms);
      this->syms = nullptr;
    }

  if (this->dynsyms)
    {
      free(this->dynsyms);
      this->dynsyms = nullptr;
    }

  if (this->synthsyms)
    {
      free(this->synthsyms);
      this->synthsyms = nullptr;
    }

  if (this->abfd)
    {
      bfd_close(this->abfd);
      this->abfd = nullptr;
    }

  for (Function *f : this->functions)
    delete f;
}
