/**
 * the purpose of this file
 * ---
 * this file exists to trick KBuild and GCC/Clang into thinking they're compiling a C file when compiling `libkern/libclosure/runtime.cpp`
 *
 * this is necessary because
 * 	a) we don't want to compile it as C++, because then we would implicitly pull in more symbols that we'd have to define, and
 * 	b) that file actually doesn't use any C++; it can be compiled just fine as plain C
 */

#include "../libclosure/runtime.cpp"
