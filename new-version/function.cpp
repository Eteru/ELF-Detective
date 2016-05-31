#include "function.h"

Function::Function()
{
  this->hasRip = false;
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
  if (this->hasRip)
    {
      CodeLine *last = this->codelines.back();
      last->additionalInformation(c->getAddress());
    }

  std::size_t pos = c->getLine().find("(%rip)");
  if (pos != std::string::npos)
    this->hasRip = true;
  else
    this->hasRip = false;

  this->codelines.push_back(c);
}

std::vector<CodeLine *> Function::getCodeLines()
{
  return this->codelines;
}
