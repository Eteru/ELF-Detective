#include "AddressBinding.h"

#include <iostream>

#include "elf-bfd.h"

void AddressBinding::find_bindings()
{
    std::string name;

    char buf[30];
    addrbind ab;
    asymbol **current;
    symvalue dec_value;

    // Parse each object file looking for undefined values
    for (ELFFile *E : objfiles)
    {
        current = E->getSyms();

        for (int i = 0; i < E->getSymcount(); ++i)
        {
            bfd *cur_bfd;

            if (*current == NULL)
                continue;

            else if ((cur_bfd = bfd_asymbol_bfd (*current)) == NULL)
                continue;

            else if (!bfd_is_target_special_symbol (cur_bfd, *current))
            {

                if (bfd_is_und_section((*current)->section))
                {
                    ab.name = bfd_asymbol_name(*current);

                    ab.undefined_in = E->getName();
                    dec_value = bfd_asymbol_value(*current);

                    bfd_sprintf_vma(cur_bfd, buf, dec_value);
                    ab.undef_value = buf;

                    ab.defined_in = "UNK";
                    ab.exe_name = exefile->getName();

                    ab.type = "foo";

                    addrbinds[ab.name] = ab;
                }
            }

            current++;
        }

    }

    // Parse each object file looking for definitions of what we found
    for (ELFFile *E : objfiles)
    {
        current = E->getSyms();

        for (int i = 0; i < E->getSymcount(); ++i)
        {
            bfd *cur_bfd;
            if (*current == NULL)
                continue;

            else if ((cur_bfd = bfd_asymbol_bfd (*current)) == NULL)
                continue;

            else if (!bfd_is_target_special_symbol (cur_bfd, *current))
            {

                if (!bfd_is_und_section((*current)->section))
                {
                    name = (*current)->name;


                    auto it = addrbinds.find(name);

                    if (it == addrbinds.end())
                    {
                        current++;
                        continue;
                    }

                    ab = it->second;

                    if (ab.name == E->getName())
                        continue;

                    ab.defined_in = E->getName();
                    dec_value = bfd_asymbol_value(*current);

                    bfd_sprintf_vma(cur_bfd, buf, dec_value);
                    ab.def_value = buf;

                    ab.undefined_section = (*current)->section->name;

                    addrbinds[name] = ab;
                }
            }

            current++;
        }
    }
    // Parse symbols from exefile
    current = exefile->getSyms();
    for (int i = 0; i < exefile ->getSymcount(); ++i)
    {
        bfd *cur_bfd;
        if (*current == NULL)
            continue;

        else if ((cur_bfd = bfd_asymbol_bfd (*current)) == NULL)
            continue;

        else if (!bfd_is_target_special_symbol (cur_bfd, *current))
        {

            if (!bfd_is_und_section((*current)->section))
            {
                name = (*current)->name;
                auto it = addrbinds.find(name);

                if (it == addrbinds.end())
                {
                    current++;
                    continue;
                }

                ab = it->second;

                flagword type = (*current)->flags;
                ab.type = (type & BSF_FUNCTION) ? "Function" : "Variable";

                dec_value = bfd_asymbol_value(*current);
                bfd_sprintf_vma(cur_bfd, buf, dec_value);
                ab.exe_value = buf;

                ab.section_name = (*current)->section->name;

                dec_value = bfd_asymbol_base(*current);
                bfd_sprintf_vma(cur_bfd, buf, dec_value);
                ab.section_value = buf;

                bfd_sprintf_vma(cur_bfd, buf, (*current)->value);
                ab.offset = buf;

                ab.sz = ((elf_symbol_type *) (*current))->internal_elf_sym.st_size;

                addrbinds[name] = ab;
            }
        }

        current++;
    }
}

void AddressBinding::dump_bindings() const
{
    for (auto symbol : addrbinds)
    {
        if (symbol.second.defined_in == "UNK")
            continue;

        std::cout << std::endl << "--------------------------"
                  << std::endl;

        // Symbol information
        std::cout << "Symbol name: " << symbol.first << std::endl;
        std::cout << "Symbol Type: " << symbol.second.type << std::endl;
        if (symbol.second.type == "Variable")
            std::cout << "Variable size: " << symbol.second.sz << std::endl;

        // Undefined information
        std::cout << std::endl;
        std::cout << "\tThis symbol is used, but unknown in object file: "
                  << symbol.second.undefined_in << std::endl;
        std::cout << "\tAddress in *" << symbol.second.undefined_in
                  << "* is " << symbol.second.undef_value << std::endl;

        // Defined information
        std::cout << std::endl;
        std::cout << "\tThis symbol is defined in: "
                  << symbol.second.defined_in << std::endl;
        std::cout << "\tAddress in *" << symbol.second.defined_in
                  << "* is " << symbol.second.def_value << std::endl;
        std::cout << "\tSection: " << symbol.second.undefined_section << std::endl;

        // Executable file information
        std::cout << std::endl;;
        std::cout << "\tAddress in executable file *" << symbol.second.exe_name
                  << "* is " << symbol.second.exe_value << std::endl;
        std::cout << "\tSection: " << symbol.second.section_name
                  << " at address: " << symbol.second.section_value << std::endl;
        std::cout << "\tOffset: " << symbol.second.offset << std::endl;
    }
}

AddressBinding::AddressBinding()
{
    exefile = nullptr;
}

AddressBinding::AddressBinding(std::vector<ELFFile *> objs, ELFFile *exe)
    : objfiles(objs), exefile(exe) {}

AddressBinding::~AddressBinding()
{
}
