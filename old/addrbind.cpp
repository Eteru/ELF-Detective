#include <iostream>
#include <vector>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cerrno>
#include <bfd.h>
#include <dis-asm.h>
#include <libiberty/safe-ctype.h>

#include "ELFFile.h"
#include "AddressBinding.h"

#include "tools.h"
#include "elf-bfd.h"
#include "disassemblemodule.h"

using namespace std;

static const string filename = "ELFDetective";
static ELFFile *exefile = nullptr;
static vector<ELFFile *> objfiles;

/* Read in the dynamic symbols.  */

static void slurp_dynamic_symtab (ELFFile *E)
{
    asymbol **sy = NULL;
    bfd *abfd = E->getBfd();
    long storage;
    long dynsymcount;

    storage = bfd_get_dynamic_symtab_upper_bound(abfd);
    if (storage < 0)
    {
        if (!(bfd_get_file_flags (abfd) & DYNAMIC))
        {
            cerr << bfd_get_filename (abfd) << " not a dynamic object" << endl;
            E->setDynSymcount(0);
            return;
        }
        bfd_fatal (bfd_get_filename(abfd));
    }

    if (storage)
        sy = (asymbol **) malloc(storage);

    dynsymcount = bfd_canonicalize_dynamic_symtab(abfd, sy);
    if (dynsymcount < 0)
        bfd_fatal (bfd_get_filename(abfd));
    E->setDynSymcount(dynsymcount);
    E->setDSyms(sy);
}

static void slurp_symtab (ELFFile *E)
{
    asymbol **sy = NULL;
    bfd *abfd = E->getBfd();
    long storage;
    long symcount;

    if (!(bfd_get_file_flags (abfd) & HAS_SYMS))
    {
        E->setSymcount(0);
        E->setSyms(NULL);
        return;
    }

    storage = bfd_get_symtab_upper_bound(abfd);
    if (storage < 0)
    {
        cerr << "failed to read symbol table from: "
             << bfd_get_filename (abfd)
             << endl;
        bfd_fatal (bfd_get_filename(abfd));
    }

    if (storage)
        sy = (asymbol **) malloc(storage);

    symcount = bfd_canonicalize_symtab(abfd, sy);
    if (symcount < 0)
        bfd_fatal(bfd_get_filename (abfd));

    E->setSymcount(symcount);
    E->setSyms(sy);
}

static void gather_symbols (ELFFile *E)
{
    long synthcount;
    bfd *abfd = E->getBfd();
    asymbol *synthsyms;

    cout << bfd_get_filename(abfd) << ": file format "
         << abfd->xvec->name << endl;

    slurp_symtab(E);
    slurp_dynamic_symtab(E);

    synthcount = bfd_get_synthetic_symtab (abfd, E->getSymcount(), E->getSyms(),
                                           E->getDynSymcount(), E->getDSyms(), &synthsyms);
    if (synthcount < 0)
        synthcount = 0;

    E->setSynthcount(synthcount);
    E->setSynthsyms(synthsyms);
}


static void prepare_file(ELFFile *file)
{
    char **matching;
    bfd *abfd = file->getBfd();

    if (!bfd_check_format_matches (abfd, bfd_object, &matching) ||
            !bfd_check_format_matches (abfd, bfd_core, &matching))
    {
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

    for (int i = 1; i < argc; ++i)
    {
        arg = args[i];

        if (arg == "-obj")
        {
            is_obj = true;
            is_exe = false;
            continue;
        }
        if (arg == "-exe")
        {
            is_obj = false;
            is_exe = true;
            continue;
        }

        if (is_obj)
            objfiles.push_back(new ELFFile(arg));

        if (is_exe)
        {
            if (exefile != nullptr)
            {
                print_usage();
                exit(0);
            }

            exefile = new ELFFile(arg);
        }
    }
}

static void cleanup()
{
    delete exefile;
    for (ELFFile *E : objfiles)
        delete E;
}

int main (int argc, char **argv)
{
    AddressBinding *AB;
    parse_args(argc, argv);

    bfd_init ();

    exefile->initBfd();

    for (ELFFile *E : objfiles)
        E->initBfd();

    prepare_file(exefile);

    for (ELFFile *E : objfiles)
        prepare_file(E);

    AB = new AddressBinding(objfiles, exefile);

    AB->find_bindings();
    AB->dump_bindings();

    Disassembly::add_section(".text");
    Disassembly::add_section(".data");
    Disassembly::add_section(".rodata");
    Disassembly::add_section(".bss");

    cout << Disassembly::disassemble_data(exefile);

    cleanup();

    return 0;
}
