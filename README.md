# evaVM
Bytecode Interpreter (VM) for the Eva language.

This repo houses my code to implement a virtual machine for the Eva programming
language, following along with Dmitry Soshnikov's
[course](http://dmitrysoshnikov.com/courses/virtual-machine/) on the topic. The
idea here is to have a fully working virtual machine/bytecode interpreter for
the Eva language, that is easy/simple to build, and easily distributed at the
source code level. 

Deps are minimal: all you should need is an up to date version of Clang or even
GCC. I've provided a Makefile which can build the VM directly, or you can use it
as a template for your own compilation script. 
