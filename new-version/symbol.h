#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>
#include <vector>

// this needs to be defined before any bfd.h include
// due to a 'won't fix' bug
#define PACKAGE "elfdetective"

#include <bfd.h>

const int SYMBOL_VAR = 0;
const int SYMBOL_FUNC = 1;

class Symbol
{
public:
  Symbol();

  bool isEmpty() const;
  bool isUndefined() const;
  bool isVariable() const;
  bool isFunction() const;

  std::string dumpExeData();
  std::string dumpObjData();

  std::string name;

  int type;
  bool defined;

  std::vector<std::string> undefined_in;
  std::string defined_in;
  std::string exe_name;

  std::string def_value;
  std::string exe_value;

  std::string defined_section;
  std::string section_name;
  std::string section_value;
  std::string def_section_value;
  std::string offset;

  bfd_vma sz;

private:
  void removeExtraZeros();
  bool cleared;
};

#endif // SYMBOL_H
