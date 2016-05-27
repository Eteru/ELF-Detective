#-------------------------------------------------
#
# Project created by QtCreator 2016-03-08T14:25:50
#
#-------------------------------------------------

QT       += core gui

LIBS += -lbfd -lopcodes
QMAKE_CXXFLAGS += -std=c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ELFDetective
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    addressbinding.cpp \
    elffile.cpp \
    tools.cpp \
    objecttab.cpp \
    symbol.cpp \
    disassemblemodule.cpp \
    function.cpp \
    codeline.cpp

HEADERS  += mainwindow.h \
    addressbinding.h \
    elffile.h \
    tools.h \
    elf-bfd.h \
    elf/aarch64.h \
    elf/alpha.h \
    elf/arc.h \
    elf/arm.h \
    elf/avr.h \
    elf/bfin.h \
    elf/common.h \
    elf/cr16.h \
    elf/cr16c.h \
    elf/cris.h \
    elf/crx.h \
    elf/d10v.h \
    elf/d30v.h \
    elf/dlx.h \
    elf/dwarf.h \
    elf/epiphany.h \
    elf/external.h \
    elf/fr30.h \
    elf/frv.h \
    elf/ft32.h \
    elf/h8.h \
    elf/hppa.h \
    elf/i370.h \
    elf/i386.h \
    elf/i860.h \
    elf/i960.h \
    elf/ia64.h \
    elf/internal.h \
    elf/ip2k.h \
    elf/iq2000.h \
    elf/lm32.h \
    elf/m32c.h \
    elf/m32r.h \
    elf/m68hc11.h \
    elf/m68k.h \
    elf/mcore.h \
    elf/mep.h \
    elf/metag.h \
    elf/microblaze.h \
    elf/mips.h \
    elf/mmix.h \
    elf/mn10200.h \
    elf/mn10300.h \
    elf/moxie.h \
    elf/msp430.h \
    elf/mt.h \
    elf/nds32.h \
    elf/nios2.h \
    elf/or1k.h \
    elf/pj.h \
    elf/ppc.h \
    elf/ppc64.h \
    elf/reloc-macros.h \
    elf/rl78.h \
    elf/rx.h \
    elf/s390.h \
    elf/score.h \
    elf/sh.h \
    elf/sparc.h \
    elf/spu.h \
    elf/tic6x-attrs.h \
    elf/tic6x.h \
    elf/tilegx.h \
    elf/tilepro.h \
    elf/v850.h \
    elf/vax.h \
    elf/visium.h \
    elf/vxworks.h \
    elf/x86-64.h \
    elf/xc16x.h \
    elf/xgate.h \
    elf/xstormy16.h \
    elf/xtensa.h \
    objecttab.h \
    symbol.h \
    disassemblemodule.h \
    function.h \
    codeline.h

FORMS    += mainwindow.ui \
    objecttab.ui

DISTFILES +=
