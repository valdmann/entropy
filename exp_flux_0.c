#include "entropy.h"

#define SCALE 4096
#define RATE  32

void
compress ()
{
  u32 p[256];
  for (int i = 0; i < 256; ++i)
    p[i] = SCALE / 2;

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
          if (sym & mask)
            {
              range = mid;
              p[ctx] += (SCALE - p[ctx]) / RATE;
              ctx = (ctx << 1) + 1;
            }
          else
            {
              low   += mid;
              range -= mid;
              p[ctx] -= p[ctx] / RATE;
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
  u32 p[256];
  for (int i = 0; i < 256; ++i)
    p[i] = SCALE / 2;

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
          if (cml < mid)
            {
              range = mid;
              p[ctx] += (SCALE - p[ctx]) / RATE;
              ctx = (ctx << 1) + 1;
              sym |= mask;
            }
          else
            {
              range -= mid;
              cml   -= mid;
              p[ctx] -= p[ctx] / RATE;
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
