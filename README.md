# ELF Detective

March 5, 2016: Adding some test code.

March 14, 2016: Added address binding code.

Note that for this to work it needs binutils-dev and libiberty-dev packages.
If you are using a Debian based OS, you'll need to change in bfd.h:
    ansidecl.h -> libiberty/ansidecl.h

Usage:
    ./elfdetective -obj obj1 ... obj n -exe exefile
