#include "symbol.h"

Symbol::Symbol()
{
  this->defined = false;
}

bool Symbol::isEmpty() const
{
  return (this->name == "");
}

bool Symbol::isUndefined() const
{
  return (this->defined == false);
}

bool Symbol::isVariable() const
{
  return (this->type == SYMBOL_VAR);
}

bool Symbol::isFunction() const
{
  return (this->type == SYMBOL_FUNC);
}

std::string Symbol::dumpData()
{
  if (!this->defined)
    return "This symbols is unkown at link time. Its value will be solved at runtime.";

  std::string ret;

  // Symbol information
  ret += "Symbol Type: ";
  if (this->type == SYMBOL_VAR)
    {
      ret += "Variable; size: ";
      ret += this->sz + "\n";
    }
  else
    {
      ret += "Function\n";
    }

  // Defined information
  //ret += "Section: " + this->defined_section + "\n";
  //ret += "Symbol definition in object file: " + this->defined_in + "\n";
  ret += "Address at definition is " + this->removeExtraZeros(this->def_value) + " (unbound)\n";


  // Undefined information
  if (this->undefined_in.size())
    {
      ret += "This symbol is also used object file:";
      for (std::string S : this->undefined_in)
        ret += " " + S;
      ret += "\n";
      //ret += "Address in *" + this->undefined_in + "* is " + this->undef_value + "\n";
    }

  // Executable file information
  ret += "Address in executable file is " + this->removeExtraZeros(this->exe_value) + "\n";
  ret +="Section: " + this->section_name
      + " (address: " + this->removeExtraZeros(this->section_value) + ")\n";
  ret += "Symbol Offset: " + this->removeExtraZeros(this->offset) + "\n";

  return ret;
}
#include <iostream>
std::string Symbol::removeExtraZeros(std::string addr)
{
  std::string aux = addr.substr(0, 8);

  if (aux.compare("00000000") == 0)
    {
      aux = addr;
      aux.replace(0, 8, "0x");

      return aux;
    }

  return addr;
}
