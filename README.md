# C Graphviz Library

Simple graphviz abstraction layer for C, see [example](main.cpp)

## Prerequisites

You need to have `C++` compiler of your choosing, `CMake` and `Ninja` installed to build your project. And `graphviz`, obviously, to use it. You install all of those with your distribution's package manager.

## Building

You can build it like any other `CMake` project:
```sh
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build .
```

Than run example with:
```sh
./build/main
```

It will spit out path to generated image, created somewhere in `/tmp/` with described in [main.cpp](main.cpp) graph. You can use than use whatever image viewer you prefer to view it.
