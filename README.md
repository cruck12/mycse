# About

Implement an LLVM pass that performs common subexpression elimination.

# Build

```
cd llvm-4.0.0.src
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install
```

# Testing

```
cd llvm-4.0.0.src/lib/Transforms/MyCSE
mv TestCase.c.txt TestCase.c
clang -emit-llvm -S -o TestCase.ll TestCase.c
opt -load ../../../build/lib/LLVMMyCSE.so -mycse -S < TestCase.ll > TestCase.opt.ll
```

# Report

See [llvm-4.0.0.src/lib/Transforms/MyCSE/Report.pdf](llvm-4.0.0.src/lib/Transforms/MyCSE/Report.pdf).
