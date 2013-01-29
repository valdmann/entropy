#ifndef ENTROPY_H
#define ENTROPY_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define likely(expr) __builtin_expect(expr, 1)
#define unlikely(expr) __builtin_expect(expr, 0)

#ifdef __GLIBC__
#undef putc
#undef getc
#define putc putc_unlocked
#define getc getc_unlocked
#endif

#ifdef _MSC_VER
#undef putc
#undef getc
#define putc _fputc_nolock
#define getc _fgetc_nolock
#endif

extern FILE *input_file, *output_file;
extern char *input_name, *output_name;

u32  write_header ();
u32  read_header ();
void compress ();
void decompress ();

#endif
