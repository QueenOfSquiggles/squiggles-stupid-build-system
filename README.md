Squiggles's Stupid Build System

A really stupid build system for C++ that assumes that:
- All of your files are in `./src`
- More deeply nested files should be compiled first
- You do not have an overly complex project
- you have `g++` installed (all this does is construct detailed `g++` calls)
- no two source files (`.c` or `cpp`) have the same file name (object files are generated in a flat directory "structure")

I have absolutely no clue why anyone would ever want to use this. But I do because I am super lazy and just want to turn typing time into executable binaries so I made this.