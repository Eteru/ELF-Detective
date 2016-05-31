#ifndef CODELINE_H
#define CODELINE_H

#include <string>

class CodeLine
{
public:
  CodeLine();
  virtual ~CodeLine();

  void setLine(std::string);
  void setAddress(std::string);
  void setHexValue(std::string);
  void setSymbol(std::string, std::string);

  std::string getLine() const;
  std::string getAddress() const;
  std::string getHexValue() const;
  std::string getSymbol() const;

  void additionalInformation(std::string);
  std::string dumpData() const;

protected:
private:
  std::string line;
  std::string address;
  std::string hexValue;
  std::string symbol;
  std::string symbolAddress;
  std::string moreInfo;
};
#endif // CODELINE_H
