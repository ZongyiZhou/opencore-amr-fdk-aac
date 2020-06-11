# FDK-AAC++

This is a fork of the standalone library of the [Fraunhofer FDK AAC](https://sourceforge.net/projects/opencore-amr/).

Multiple optimization has been added to the AAC encoder, including faster math functions and SIMD-optmized FFT using inline assembly and intrinsic. It brings 24% ~ 37% speedup on x86 and ARM CPUs.

## **Benchmark**
### Audio file: Blank Space - Taylor Swift - 1989 (3:51)
### Encode settings: LC AAC, VBR 3

* x86-64 (gcc 8.4.0, Intel Core i7-6820HQ) : **28%** faster
* armv7-a (clang 9.0.8, Qualcomm MSM8974AC) : **24%** faster
* armv8-a (clang 9.0.8, ARM Cortex-A53) : **37%** faster

## **Recommended Compilers**
* g++ 5 or later
* clang 4.0 or later
* MSVC 2015 or later

## **Build**
### Use GNU toolchain
```
./autogen.sh
./configure
./make -j && make install
```
### Use CMake
```
N/A
```