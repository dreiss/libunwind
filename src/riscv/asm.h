/* libunwind - a platform-independent unwind library
   Copyright (C) 2021 Zhaofeng Li

This file is part of libunwind.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

#if __riscv_xlen == 32
# define STORE sw
# define LOAD lw
# define SZREG 4
#elif __riscv_xlen == 64
# define STORE sd
# define LOAD ld
# define SZREG 8
#endif

#if __riscv_flen == 64
# define SZFREG 8
# define STORE_FP fsd
# define LOAD_FP fld
#elif __riscv_flen == 32
# define SZFREG 4
# define STORE_FP fsw
# define LOAD_FP flw
#elif defined(__riscv_float_abi_soft)
// Don't need these.
#else
# error "Unsupported RISC-V floating-point length"
#endif

