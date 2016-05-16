#ifndef ADDRESSBINDING_H
#define ADDRESSBINDING_H

#include <vector>
#include <unordered_map>

#include "symbol.h"
#include "elffile.h"
#include "tools.h"


class AddressBinding
{
public:
  void findBindings();

  std::vector<std::string> getSymbols() const;

  Symbol getSymbol(std::string) const;

  AddressBinding();
  AddressBinding(std::vector<ELFFile *>, ELFFile *);
  virtual ~AddressBinding();
protected:
private:
  std::unordered_map<std::string, Symbol> symbolTable;
  std::vector<ELFFile *> objfiles;
  ELFFile *exefile;
};

#endif // ADDRESSBINDING_H
