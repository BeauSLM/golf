# Build

Short version: run `make` in the root of the project

``` sh
# from root directory of project

make         # default is a release build
make release # you can call the default rule this way
make debug   # build without optimizations and with debug symbols
make clean   # clean up build artifacts
```

Some notes:
- I'm using `clang++` as my compiler as `clangd` is my language server
- ALL my make rules have the flags `-Wall -Wextra` turned on
- to switch between release and debug builds without changing source, run `make clean` first
  - I'd like to fix at some point but it's just not a high priority

I'll re-iterate the last note - SWITCHING BETWEEN RELEASE AND DEBUG BUILDS MEANS YOU HAVE TO USE THE CLEAN RULE FIRST

# Usage

``` sh
$ ./golf                   # incorrect usage - provide one filename argument that points to your source file
> Usage: ./golf <filename> # output of above
$ ./golf <your_golf_file>  # correct usage
```

Short version: `golf` accepts one filename argument. To use, simply build as shown above and run `./golf <your_golf_file>`

Note that you must build and run `golf` from the root directory of the project
