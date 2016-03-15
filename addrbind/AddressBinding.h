#ifndef ADDRESSBINDING_H
#define ADDRESSBINDING_H

#include <vector>
#include <unordered_map>
#include "ELFFile.h"
#include "tools.h"


class AddressBinding
{
    public:
        void find_bindings();
        void dump_bindings();

        AddressBinding();
        AddressBinding(std::vector<ELFFile *>, ELFFile *);
        virtual ~AddressBinding();
    protected:
    private:
        std::unordered_map<std::string, addrbind> addrbinds;
        std::vector<ELFFile *> objfiles;
        ELFFile *exefile;
};

#endif // ADDRESSBINDING_H
