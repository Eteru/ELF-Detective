#include "symbol.h"

Symbol::Symbol()
{
  this->defined = false;
  this->cleared = false;
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

std::string Symbol::dumpExeData()
{
  if (!this->defined)
    return "This symbols is unkown at link time. Its value will be solved at runtime.";

  if (!this->cleared)
    this->removeExtraZeros();

  std::string ret;

  // Symbol information
  ret += "Symbol Type: ";
  ret += (this->type == SYMBOL_VAR) ? "Variable\n" : "Function\n";

  // Executable file information
  ret += "Address: " + this->exe_value + "\n";
  ret +="Section: " + this->section_name
      + " (address: " + this->section_value + ")\n";
  ret += "Symbol Offset: " + this->offset + "\n";

  // Undefined information
  if (this->undefined_in.size())
    {
      ret += "This symbol is also used object file:";
      for (std::string S : this->undefined_in)
        ret += " " + S;
      ret += "\n";
    }

  return ret;
}

std::string Symbol::dumpObjData()
{
  if (!this->defined)
    return "";

  if (!this->cleared)
    this->removeExtraZeros();

  std::string ret;

  // Symbol information
  ret += "Symbol Type: ";
  ret += (this->type == SYMBOL_VAR) ? "Variable\n" : "Function\n";

  // Defined information
  ret += "Address: " + this->def_value +
          (this->def_value.compare("0x") == 0 ? "0 (unbound)\n" : "\n");
  ret +="Section: " + this->section_name
      + " (address: " + this->def_section_value
      + (this->def_section_value.compare("0x") == 0 ? "0 (unbound) " : "") + ")\n";
  ret += "Symbol Offset: " + this->offset + "\n";

  return ret;
}

void Symbol::removeExtraZeros()
{
  this->cleared = true;

  this->def_value.erase(0, this->def_value.find_first_not_of('0'));
  this->def_value = "0x" + this->def_value;

  this->exe_value.erase(0, this->exe_value.find_first_not_of('0'));
  this->exe_value = "0x" + this->exe_value;

  this->section_value.erase(0, this->section_value.find_first_not_of('0'));
  this->section_value = "0x" + this->section_value;

  this->def_section_value.erase(0, this->def_section_value.find_first_not_of('0'));
  this->def_section_value = "0x" + this->def_section_value;

  this->offset.erase(0, this->offset.find_first_not_of('0'));
  this->offset = "0x" + this->offset;
}
