#include "function.h"

Function::Function()
{
}

Function::~Function()
{
  for (CodeLine *C : this->codelines)
    delete C;
}

void Function::setName(std::string name)
{
  this->name = name;
}
std::string Function::getName() const
{
  return this->name;
}

void Function::addCodeLine(CodeLine *c)
{
  this->codelines.push_back(c);
}

std::vector<CodeLine *> Function::getCodeLines()
{
  return this->codelines;
}
