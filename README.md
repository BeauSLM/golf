# Build

Short version: run `make` in the root of the project

``` sh
# from root directory of project

make         # default is a release build
make release # you can call the default rule this way
make clean   # clean up build artifacts
```

Some notes:
- I'm using `clang++` as my compiler as `clangd` is my language server
- ALL my make rules have the flags `-Wall -Wextra` turned on

# Usage

``` sh
$ ./golf                   # incorrect usage - provide one filename argument that points to your source file
> Usage: ./golf <filename> # output of above
$ ./golf <your_golf_file>  # correct usage
```

Short version: `golf` accepts one filename argument. To use, simply build as shown above and run `./golf <your_golf_file>`

Note that you must build and run `golf` from the root directory of the project
