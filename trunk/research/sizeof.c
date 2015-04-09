/*
  $Id$

  Simple sizeof experiment to judge appropriate types to replace
  the DASM chaos with. Looks like we can easily go for "int" all
  over the place these days. If we want to be compatible with 16
  bit machines like the Amiga, we may want to go for "long". The
  best choice might be to typedef "int32_t" to a usable name and
  stick with that, then we'd only rule out some 8 bit machines.
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define S(type) printf("sizeof(" #type ") == %zu\n", sizeof(type))

struct STRLIST {
  struct STRLIST *next;
  char data[];
};

int main(void)
{
  S(char);
  S(int8_t);
  S(short);
  S(int16_t);
  S(int);
  S(bool);
  S(long);
  S(int32_t);
  S(long long);
  S(int64_t);
  S(float);
  S(double);
  S(void*);
  S(char*);
  S(int*);
  S(struct STRLIST);
  return EXIT_SUCCESS;
}

/*
Darwin Agnes.local 8.11.0 Darwin Kernel Version 8.11.0: Wed Oct 10 18:26:00 PDT 2007; root:xnu-792.24.17~1/RELEASE_PPC Power Macintosh powerpc PowerBook6,8 Darwin

sizeof(char) == 1
sizeof(int8_t) == 1
sizeof(short) == 2
sizeof(int16_t) == 2
sizeof(int) == 4
sizeof(bool) == 4
sizeof(long) == 4
sizeof(int32_t) == 4
sizeof(long long) == 8
sizeof(int64_t) == 8
sizeof(float) == 4
sizeof(double) == 8

Linux ugradx.cs.jhu.edu 2.6.26.5-45.fc9.i686 #1 SMP Sat Sep 20 03:45:00 EDT 2008 i686 i686 i386 GNU/Linux

sizeof(char) == 1
sizeof(int8_t) == 1
sizeof(short) == 2
sizeof(int16_t) == 2
sizeof(int) == 4
sizeof(bool) == 1
sizeof(long) == 4
sizeof(int32_t) == 4
sizeof(long long) == 8
sizeof(int64_t) == 8
sizeof(float) == 4
sizeof(double) == 8

SunOS peregrine 5.10 Generic_118822-11 sun4u sparc

sizeof(char) == 1
sizeof(int8_t) == 1
sizeof(short) == 2
sizeof(int16_t) == 2
sizeof(int) == 4
sizeof(bool) == 1
sizeof(long) == 4
sizeof(int32_t) == 4
sizeof(long long) == 8
sizeof(int64_t) == 8
sizeof(float) == 4
sizeof(double) == 8

Linux eggbert 3.18.9-gentoo #1 SMP Thu Apr 2 19:53:32 EDT 2015 x86_64 AMD Athlon(tm) 64 X2 Dual Core Processor 6000+ AuthenticAMD GNU/Linux
ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux.so.2, for GNU/Linux 2.6.32, not stripped

sizeof(char) == 1
sizeof(int8_t) == 1
sizeof(short) == 2
sizeof(int16_t) == 2
sizeof(int) == 4
sizeof(bool) == 1
sizeof(long) == 4
sizeof(int32_t) == 4
sizeof(long long) == 8
sizeof(int64_t) == 8
sizeof(float) == 4
sizeof(double) == 8
sizeof(void*) == 4
sizeof(char*) == 4
sizeof(int*) == 4
sizeof(struct STRLIST) == 4

Linux eggbert 3.18.9-gentoo #1 SMP Thu Apr 2 19:53:32 EDT 2015 x86_64 AMD Athlon(tm) 64 X2 Dual Core Processor 6000+ AuthenticAMD GNU/Linux
ELF 64-bit LSB executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux 2.6.32, not stripped

sizeof(char) == 1
sizeof(int8_t) == 1
sizeof(short) == 2
sizeof(int16_t) == 2
sizeof(int) == 4
sizeof(bool) == 1
sizeof(long) == 8
sizeof(int32_t) == 4
sizeof(long long) == 8
sizeof(int64_t) == 8
sizeof(float) == 4
sizeof(double) == 8
sizeof(void*) == 8
sizeof(char*) == 8
sizeof(int*) == 8
sizeof(struct STRLIST) == 8
*/
