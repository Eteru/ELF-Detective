#include <QFileDialog>
#include <QMessageBox>
#include <iostream>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "objecttab.h"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  bfd_init();

  ui->exeToolBox->setItemText(0, "Data");
  ui->exeToolBox->setItemText(1, "Functions");

  ui->exeFunctionPage->activateWindow();

  ui->objTabs->setTabsClosable(true);

  ui->objTabs->removeTab(0);
  ui->objTabs->removeTab(0);
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::on_addObj_clicked()
{
  QString filename = QFileDialog::getOpenFileName(this, tr("Add object file"), "/home");
  QStringList tokens = filename.split("/");

  if (filename == "")
    return;

  this->objfiles.push_back(new ELFFile(filename.toStdString()));

  filename = tokens.value(tokens.length() - 1);

  ui->objTabs->insertTab(ui->objTabs->count(), new objecttab(), QIcon(QString("")), filename);
}

void MainWindow::on_addExe_clicked()
{
  QString filename = QFileDialog::getOpenFileName(this, tr("Add executable file"), "/home");

  if (filename == "")
    return;

  this->exefile = new ELFFile(filename.toStdString());

  ui->addExe->setDisabled(true);
}

void MainWindow::on_runProj_clicked()
{
  int errCount = 0, errCode;
  QString errors;

  if (this->exefile)
    {
      errCode = this->exefile->initBfd(ELF_EXE_FILE);

      if (errCode)
        {
          errCount++;
          errors += this->errorMessage(errCode, this->exefile);
        }
    }
  else
    {
      errCount++;
      errors += "Can't find executable file!\n";
    }

  if (this->objfiles.size())
    {
      for (ELFFile *E : this->objfiles)
        {
          errCode = E->initBfd(ELF_OBJ_FILE);

          if (errCode)
            {
              errCount++;
              errors += this->errorMessage(errCode, E);
            }
        }
    }
  else
    {
      errCount++;
      errors += "Can't find any object file!";
    }

  if (errCount)
    {
      QMessageBox::critical(this, tr("Errors parsing project"), errors);
      return;
    }

  this->AB = new AddressBinding(objfiles, exefile);

  ui->runProj->setDisabled(true);

  this->showSymbols();
}

void MainWindow::on_clearProj_clicked()
{
  if (this->exefile)
    {
      delete this->exefile;
      this->exefile = nullptr;
    }

  for (ELFFile *E : this->objfiles)
    {
      delete E;
    }
  this->objfiles.clear();

  if (this->AB)
    {
      delete this->AB;
      this->AB = nullptr;
    }

  int tabsNo = ui->objTabs->count();
  for (int i = 0; i < tabsNo; ++i)
    {
      ui->objTabs->removeTab(0);
    }

  ui->exeDataList->clear();

  ui->addExe->setDisabled(false);
  ui->runProj->setDisabled(false);
}

void MainWindow::on_objTabs_tabCloseRequested(int index)
{
  ELFFile *E = this->objfiles[index];

  this->objfiles.erase(this->objfiles.begin() + index);
  delete E;

  ui->objTabs->removeTab(index);
}

void MainWindow::showSymbols() const
{
  std::vector<std::string> symbols = this->AB->getSymbols();

  // exe file gets the symbols directly from the address binding module
  // so it can easily ignore useless symbols
  for (std::string S : symbols)
    {
      Symbol sym = this->AB->getSymbol(S);
      if (sym.isUndefined() || sym.isVariable())
        ui->exeDataList->addItem(QString::fromStdString(S));
    }

  for (unsigned int i = 0; i < this->objfiles.size(); ++i)
    {
      symbols.clear();
      symbols = this->objfiles[i]->getSymbolList();

      objecttab* tui = (objecttab *) ui->objTabs->widget(i);

      for (std::string S : symbols)
        {
          Symbol sym = this->AB->getSymbol(S);
          if (sym.isUndefined() || sym.isVariable())
            tui->addSymbol(S);
        }
    }
}

QString MainWindow::errorMessage(int errCode, ELFFile *E) const
{
  QString msg;

  switch (errCode)
    {
    case ELF_NOT_EXE:
      msg = QString::fromStdString("Wrong format for file " + E->getName() + "!\n");

      break;
    case ELF_NOT_OBJ:
      msg = QString::fromStdString("Wrong format for file " + E->getName() + "!\n");

      break;
    case BFD_FILE_NULL:
      msg = QString::fromStdString(E->getName() + " cannot be parsed!\n");

      break;
    case BFD_FILE_SIZE:
      msg = QString::fromStdString(E->getName() + " is empty!\n");

      break;
    default:
      break;
    }

  return msg;
}

void MainWindow::on_exeDataList_clicked(const QModelIndex &index)
{

  QString symbolName = ui->exeDataList->item(index.row())->text();
  Symbol sym = AB->getSymbol(symbolName.toStdString());

  ui->infoOutput->clear();
  ui->infoOutput->append(QString::fromStdString(sym.dumpData()));
}
