#include "codeline.h"

#include <regex>
#include <sstream>
#include <cstring>

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

void CodeLine::setSymbol(std::string sym, std::string addr)
{
  this->symbol = sym;
  addr.erase(0, addr.find_first_not_of('0'));
  this->symbolAddress = "0x" + addr;
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

void CodeLine::additionalInformation(std::string addr)
{
  char str[256];
  char *token;
  std::string tok;
  std::string referredAddr;
  std::size_t pos;

  this->line.copy(str, this->line.length(), 0);
  token = strtok((char*)str, " ,");
  while (token != NULL)
    {
      tok = token;
      pos = tok.find("(%rip)");
      if (pos != std::string::npos)
        {
          referredAddr = tok.substr(0, pos);
          break;
        }

      token = strtok(NULL, " ,");
    }

  this->moreInfo = "%rip points to the next instruction =" + addr + "\n";
  this->moreInfo += "Symbol address is found by: " + addr + " + " + referredAddr + "\n";
}

std::string CodeLine::dumpData() const
{
  if (this->symbol.compare("") == 0)
    return "";

  return "This line references symbol: <" + this->symbol + ">(" + this->symbolAddress + ")\n" +
      (this->moreInfo == "" ? "" : this->moreInfo);
}
