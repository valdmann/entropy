#include "entropy.h"

#define P_BITS 12
#define C_BITS 8
#define R_BITS 20
#define P_SCALE (1 << P_BITS)
#define C_LIMIT (1 << C_BITS)
#define R_SCALE (1 << R_BITS)

static u32 p[256];     // probabilities scaled by P_SCALE
static u32 c[256];     // frequency counts from 0 to C_LIMIT-1
static u32 r[C_LIMIT]; // reciprocals : r[c] == R_SCALE/(c+1)

void
initialize ()
{
  for (u32 i = 0; i != 256; ++i)
    p[i] = P_SCALE / 2;
  for (u32 i = 0; i != 256; ++i)
    c[i] = 0;
  for (u32 i = 0; i != C_LIMIT; ++i)
    r[i] = R_SCALE / (i + 1);
}

static inline u32
fast_div(u32 p, u32 c)
{
  return p * r[c] / R_SCALE;
}

void
compress ()
{
  initialize ();

  u64 low         = 0;
  u32 range       = 0xFFFFFFFF;
  u32 flux_length = 1;
  u8  flux_first  = 0;

  u32 remaining = write_header ();

  while (remaining--)
    {
      int sym = getc (input_file);
      for (u32 ctx = 1, mask = 1 << 7; mask != 0; mask >>= 1)
        {
          u32 mid = range / SCALE * p[ctx];
          if (c[ctx] + 1 < C_LIMIT)
            ++c[ctx];
          if (sym & mask)
            {
              range = mid;
              p[ctx] += fast_div (P_SCALE - p[ctx], c[ctx]);
              ctx = (ctx << 1) + 1;
            }
          else
            {
              low   += mid;
              range -= mid;
              p[ctx] -= fast_div (p[ctx], c[ctx]);
              ctx = (ctx << 1);
            }
          while (range <= 0xFFFFFF)
            {
              u32 lo32 = low, hi32 = low >> 32;
              if (lo32 < 0xFF000000 || unlikely(hi32))
                {
                  putc (flux_first + hi32, output_file);
                  while (--flux_length)
                    putc (0xFF + hi32, output_file);
                  flux_first = lo32 >> 24;
                }
              ++flux_length;
              low = lo32 << 8;
              range <<= 8;
            }
        }
    }
  u32 lo32 = low, hi32 = low >> 32;
  putc (flux_first + hi32, output_file);
  while (--flux_length)
    putc (0xFF + hi32, output_file);
  putc (lo32 >> 24, output_file);
  putc (lo32 >> 16, output_file);
  putc (lo32 >>  8, output_file);
  putc (lo32 >>  0, output_file);
}

void
decompress ()
{
  initialize ();

  u32 range = 0xFFFFFFFF;
  u32 cml   = 0;

  u32 remaining = read_header ();

  for (int i = 0; i != 5; ++i)
    cml = (cml << 8) + getc (input_file);

  while (remaining--)
    {
      int sym = 0;
      for (u32 ctx = 1, mask = 1 << 7; mask != 0; mask >>= 1)
        {
          u32 mid = range / SCALE * p[ctx];
          if (c[ctx] + 1 < C_LIMIT)
            ++c[ctx];
          if (cml < mid)
            {
              range = mid;
              p[ctx] += fast_div (P_SCALE - p[ctx], c[ctx]);
              ctx = (ctx << 1) + 1;
              sym |= mask;
            }
          else
            {
              range -= mid;
              cml   -= mid;
              p[ctx] -= fast_div (p[ctx], c[ctx]);
              ctx = (ctx << 1);
            }
          while (range <= 0xFFFFFF)
            {
              range <<= 8;
              cml = (cml << 8) + getc (input_file);
            }
        }
      putc (sym, output_file);
    }
}
