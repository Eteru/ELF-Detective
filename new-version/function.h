#ifndef FUNCTION_H
#define FUNCTION_H

#include <vector>
#include "codeline.h"

class Function
{
public:
  Function();
  virtual ~Function();

  void setName(std::string);
  std::string getName() const;

  void addCodeLine(CodeLine *);
  std::vector<CodeLine *> getCodeLines();

protected:
private:
  std::string name;
  std::vector<CodeLine *> codelines;

  bool hasRip;
};

#endif // FUNCTION_H
