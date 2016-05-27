#include "codeline.h"

#include <regex>

CodeLine::CodeLine() : line(""), address(""), hexValue(""), symbol("")
{}

CodeLine::~CodeLine()
{}

void CodeLine::setLine(std::string line)
{
  // remove '#' added sometimes by the disassembler
  // and leanding and trailling spaces
  try
  {
    line.erase(line.find_last_of("#"), line.length());
  } catch (const std::out_of_range& e)
  {
    // no '#' in string
  }

  this->line = line;

}

void CodeLine::setAddress(std::string addr)
{
  this->address = addr;
}

void CodeLine::setHexValue(std::string hexval)
{
  this->hexValue = hexval;
}

void CodeLine::setSymbol(std::string sym)
{
  this->symbol = sym;
}


std::string CodeLine::getLine() const
{
  return this->line;
}

std::string CodeLine::getAddress() const
{
  return this->address;
}

std::string CodeLine::getHexValue() const
{
  return this->hexValue;
}

std::string CodeLine::getSymbol() const
{
  return this->symbol;
}

std::string CodeLine::dumpData() const
{
  if (this->symbol.compare("") == 0)
    return "";

  return "This line refferences symbol: " + this->symbol;
}
