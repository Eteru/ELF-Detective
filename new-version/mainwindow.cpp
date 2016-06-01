#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QIcon>
#include <QShortcut>
#include <iostream>
#include <vector>
#include <sstream>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "objecttab.h"
#include "disassemblemodule.h"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  bfd_init();

  // pages
  ui->exeToolBox->setItemText(0, "Data");
  ui->exeToolBox->setItemText(1, "Functions");

  ui->exeFunctionPage->activateWindow();

  // button icons
  ui->addExe->setIcon(QIcon("icons/icon-add-exe.ico"));
  ui->addObj->setIcon(QIcon("icons/icon-add-obj.ico"));
  ui->runProj->setIcon(QIcon("icons/icon-run.ico"));
  ui->clearProj->setIcon(QIcon("icons/icon-clear.ico"));

  ui->addExe->setIconSize(QSize(25, 25));
  ui->addObj->setIconSize(QSize(25, 25));
  ui->runProj->setIconSize(QSize(25, 25));
  ui->clearProj->setIconSize(QSize(25, 25));

  // tabs
  ui->objTabs->setTabsClosable(true);
  ui->objTabs->removeTab(0);
  ui->objTabs->removeTab(0);

  // tree
  ui->exeFunctionsTree->header()->hide();
  ui->exeFunctionsTree->headerItem()->setText(0, "Offset");

  // output table
  ui->infoOutputTable->setColumnCount(2);

  ui->infoOutputTable->setShowGrid(false);
  ui->infoOutputTable->verticalHeader()->setVisible(false);
  ui->infoOutputTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  ui->infoOutputTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->infoOutputTable->setSelectionMode(QAbstractItemView::SingleSelection);

  ui->infoOutputTable->setHorizontalHeaderLabels(QStringList()<<"Executable"<<"Object");
  ui->infoOutputTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  ui->infoOutputTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  ui->infoOutputTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  ui->infoOutputTable->horizontalHeader()->setSectionsClickable(false);

  // shortcuts
  new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_E), this, SLOT(on_addExe_clicked()));
  new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_O), this, SLOT(on_addObj_clicked()));
  new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_R), this, SLOT(on_runProj_clicked()));
  new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_C), this, SLOT(on_clearProj_clicked()));
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::on_addObj_clicked()
{
  if (!ui->addObj->isEnabled())
    return;

  QString filename = QFileDialog::getOpenFileName(this, tr("Add object file"), "/home");
  QStringList tokens = filename.split("/");

  if (filename == "")
    return;

  ELFFile *obj = new ELFFile(filename.toStdString());
  this->objfiles.push_back(obj);

  filename = tokens.value(tokens.length() - 1);

  QWidget *view = new objecttab();
  obj->setView(view);

  ui->objTabs->insertTab(ui->objTabs->count(), view, QIcon(QString("")), filename);
}

void MainWindow::on_addExe_clicked()
{
  if (!ui->addExe->isEnabled())
    return;

  QString filename = QFileDialog::getOpenFileName(this, tr("Add executable file"), "/home");

  if (filename == "")
    return;

  this->exefile = new ELFFile(filename.toStdString());

  ui->addExe->setDisabled(true);

  this->exefile->setView(this);
}

void MainWindow::on_runProj_clicked()
{
  if (!ui->runProj->isEnabled())
    return;

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

  Disassembly::add_section(".text");
  Disassembly::disassemble_data(exefile);
  for (ELFFile *E : this->objfiles)
    Disassembly::disassemble_data(E);

  ui->runProj->setDisabled(true);
  ui->addObj->setDisabled(true);

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
  ui->exeFunctionsTree->clear();
  this->removeTableRows();

  ui->addExe->setDisabled(false);
  ui->addObj->setDisabled(false);
  ui->runProj->setDisabled(false);

  ui->exeFunctionsTree->header()->hide();
}

void MainWindow::on_objTabs_tabCloseRequested(int index)
{
  ELFFile *E = this->objfiles[index];

  this->objfiles.erase(this->objfiles.begin() + index);
  delete E;

  ui->objTabs->removeTab(index);
}

void MainWindow::initFunctionTree(ELFFile *E, QTreeWidget *parent) const
{
  parent->setColumnCount(3);
  parent->setColumnWidth(0, 150);
  parent->headerItem()->setText(1, "Instruction");

  parent->setColumnHidden(2, true);
  parent->headerItem()->setText(2, "Opcode");

  parent->header()->show();

  for (Function *f : E->getFunctions())
    {
      if (this->AB->getSymbol(f->getName()).isEmpty())
        continue;

      QTreeWidgetItem *itm = new QTreeWidgetItem(parent);

      itm->setText(0, QString::fromStdString(f->getName()));
      ui->exeFunctionsTree->addTopLevelItem(itm);

      this->addCodeLines(f, itm);
    }
}

void MainWindow::addCodeLines(Function *f, QTreeWidgetItem *parent) const
{
  for (CodeLine *c : f->getCodeLines())
    {
      QTreeWidgetItem *itm = new QTreeWidgetItem(parent);

      itm->setText(0, QString::fromStdString(c->getAddress()));
      itm->setText(1, QString::fromStdString(c->getLine()));
      itm->setText(2, QString::fromStdString(c->getHexValue()));

      parent->addChild(itm);
    }
}

void MainWindow::showSymbols() const
{
  std::vector<std::string> symbols = this->AB->getSymbols();

  // exe file gets the symbols directly from the address binding module
  // so it can easily ignore useless symbols
  initFunctionTree(this->exefile, ui->exeFunctionsTree);
  for (std::string S : symbols)
    {
      Symbol sym = this->AB->getSymbol(S);
      if (sym.isUndefined() || sym.isVariable())
        ui->exeDataList->addItem(QString::fromStdString(S));

      if (sym.isFunction() && sym.isUndefined())
        {
          QTreeWidgetItem *itm = new QTreeWidgetItem(ui->exeFunctionsTree);

          itm->setText(0, QString::fromStdString(sym.name));
          ui->exeFunctionsTree->addTopLevelItem(itm);
        }
    }

  for (unsigned int i = 0; i < this->objfiles.size(); ++i)
    {
      symbols.clear();
      symbols = this->objfiles[i]->getSymbolList();

      objecttab* tui = (objecttab *) ui->objTabs->widget(i);

      initFunctionTree(this->objfiles[i], tui->getTree());

      for (std::string S : symbols)
        {
          Symbol sym = this->AB->getSymbol(S);
          if (sym.isUndefined() || sym.isVariable())
            tui->addSymbol(S);
        }
    }
}

// adds a line of text to each column
void MainWindow::addRows(std::string info1, std::string info2)
{
  std::stringstream ss(info1);
  std::stringstream ss2(info2);
  std::string line;
  std::string line2;

  while (std::getline(ss, line, '\n'))
    {
      ui->infoOutputTable->insertRow(ui->infoOutputTable->rowCount());
      ui->infoOutputTable->setItem(ui->infoOutputTable->rowCount() - 1,
                                   0, new QTableWidgetItem(QString::fromStdString(line)));

      // object file should always have the same number of rows or less
      std::getline(ss2, line2, '\n');

      ui->infoOutputTable->setItem(ui->infoOutputTable->rowCount() - 1,
                                   1, new QTableWidgetItem(QString::fromStdString(line2)));
    }
}

// clear table content
void MainWindow::removeTableRows()
{
  while (ui->infoOutputTable->rowCount() > 0)
    ui->infoOutputTable->removeRow(0);
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

  std::string filename = sym.defined_in;

  for (unsigned int i = 0; i < this->objfiles.size(); ++i)
    {
      if (this->objfiles[i]->getName().compare(filename) == 0)
        {
          objecttab *ot = (objecttab *)this->objfiles[i]->getView();
          ui->objTabs->setCurrentIndex(i);
          ot->selectSymbol(symbolName);
          break;
        }
    }

  this->removeTableRows();
  this->addRows(sym.dumpExeData(), sym.dumpObjData());
}

void MainWindow::on_exeFunctionsTree_itemClicked(QTreeWidgetItem *item, int column)
{
  (void)column;

  QTreeWidgetItem *parent = item->parent();
  QString symbolName = (parent == nullptr) ? item->text(0) : parent->text(0);
  Symbol sym = AB->getSymbol(symbolName.toStdString());

  if (!sym.isUndefined())
    {
      std::string filename = sym.defined_in;

      for (unsigned int i = 0; i < this->objfiles.size(); ++i)
        {
          if (this->objfiles[i]->getName().compare(filename) == 0)
            {
              objecttab *ot = (objecttab *)this->objfiles[i]->getView();
              ui->objTabs->setCurrentIndex(i);

              if (parent == nullptr)
                {
                  ot->selectFunction(symbolName);
                  this->removeTableRows();
                  this->addRows(sym.dumpExeData(), sym.dumpObjData());
                }
              else
                {
                  int itemAt = ui->exeFunctionsTree->currentIndex().row();
                  std::vector<Function *> exeFuncs = this->exefile->getFunctions();
                  auto exeIt = find_if(exeFuncs.begin(), exeFuncs.end(),
                                    [&sym](Function *F) {return F->getName().compare(sym.name) == 0;});

                  std::vector<Function *> objFuncs = this->objfiles[i]->getFunctions();
                  auto objIt = find_if(objFuncs.begin(), objFuncs.end(),
                                    [&sym](Function *F) {return F->getName().compare(sym.name) == 0;});

                  this->removeTableRows();

                  if (exeIt != exeFuncs.end())
                    {
                      std::vector<CodeLine *> c1 = (*exeIt)->getCodeLines();
                      std::vector<CodeLine *> c2 = (*objIt)->getCodeLines();
                      ot->selectFunctionLine(symbolName, itemAt);

                      this->addRows(c1[itemAt]->dumpData(), c2[itemAt]->dumpData());
                    }
                }

              break;
            }
        }
    }
}

void MainWindow::on_checkHex_clicked(bool checked)
{
  ui->exeFunctionsTree->setColumnHidden(1, checked);
  ui->exeFunctionsTree->setColumnHidden(2, !checked);

  for (ELFFile *E : this->objfiles)
    ((objecttab *)E->getView())->toggleHex(checked);
}
