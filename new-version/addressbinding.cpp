#include <iostream>

#include "addressbinding.h"
#include "elf-bfd.h"

void AddressBinding::findBindings()
{
  std::string name;

  char buf[30];
  asymbol **current;
  symvalue dec_value;

  // Parse each object file looking for any symbols
  for (ELFFile *E : objfiles)
    {
      current = E->getSyms();

      for (int i = 0; i < E->getSymcount(); ++i)
        {
          bfd *cur_bfd;
          Symbol entry;

          if (*current == NULL)
            continue;

          else if ((cur_bfd = bfd_asymbol_bfd(*current)) == NULL)
            continue;

          else if (!bfd_is_target_special_symbol(cur_bfd, *current))
            {

              // ignore symbols starting with "."
              if (strncmp((*current)->name, ".", 1) != 0 &&
                  !bfd_is_abs_section((*current)->section))
                {
                  name = bfd_asymbol_name(*current);
                  auto it = symbolTable.find(name);

                  if (it != symbolTable.end())
                    entry = it->second;

                  if (bfd_is_und_section((*current)->section))
                    {
                      entry.name = name;
                      entry.defined_in = E->getName();
                      entry.undefined_in.push_back(E->getName());

                      symbolTable[name] = entry;
                    }
                  else
                    {
                      entry.name = bfd_asymbol_name(*current);

                      entry.defined = true;
                      entry.defined_in = E->getName();

                      dec_value = bfd_asymbol_value(*current);
                      bfd_sprintf_vma(cur_bfd, buf, dec_value);
                      entry.def_value = buf;

                      entry.defined_section = (*current)->section->name;

                      symbolTable[name] = entry;
                    }
                }
            }

          current++;
        }

    }

  // Parse symbols from exefile
  std::string exename = exefile->getName();
  current = exefile->getSyms();
  for (int i = 0; i < exefile->getSymcount(); ++i)
    {
      bfd *cur_bfd;
      Symbol entry;

      if (*current == NULL)
        continue;

      else if ((cur_bfd = bfd_asymbol_bfd (*current)) == NULL)
        continue;

      else if (!bfd_is_target_special_symbol (cur_bfd, *current))
        {

          if (strncmp((*current)->name, ".", 1) != 0 &&
              !bfd_is_abs_section((*current)->section))
            {
              name = (*current)->name;
              auto it = symbolTable.find(name);

              if (it == symbolTable.end())
                {
                  current++;
                  continue;
                }

              entry = it->second;

              flagword type = (*current)->flags;
              entry.type = (type & BSF_FUNCTION) ? SYMBOL_FUNC : SYMBOL_VAR;

              dec_value = bfd_asymbol_value(*current);
              bfd_sprintf_vma(cur_bfd, buf, dec_value);
              entry.exe_value = buf;

              entry.section_name = (*current)->section->name;

              dec_value = bfd_asymbol_base(*current);
              bfd_sprintf_vma(cur_bfd, buf, dec_value);
              entry.section_value = buf;

              bfd_sprintf_vma(cur_bfd, buf, (*current)->value);
              entry.offset = buf;

              entry.sz = ((elf_symbol_type *) (*current))->internal_elf_sym.st_size;

              entry.exe_name = exename;

              symbolTable[name] = entry;
            }
        }

      current++;
    }
}

std::vector<std::string> AddressBinding::getSymbols() const
{
  std::vector<std::string> symbols;

  for (auto it = this->symbolTable.begin(); it != this->symbolTable.end(); ++it)
    {
      symbols.push_back(it->first);
    }

  return symbols;
}

Symbol AddressBinding::getSymbol(std::string symbolName) const
{
  auto it = symbolTable.find(symbolName);

  if (it == symbolTable.end())
    return Symbol();

  return it->second;
}

AddressBinding::AddressBinding()
{
  exefile = nullptr;
}

AddressBinding::AddressBinding(std::vector<ELFFile *> objs, ELFFile *exe)
  : objfiles(objs), exefile(exe)
{
  this->findBindings();
}

AddressBinding::~AddressBinding()
{
  this->exefile = nullptr;
  this->objfiles.clear();
}
