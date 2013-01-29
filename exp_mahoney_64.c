#include "entropy.h"

#define SCALE 4096
#define RATE  32

void
compress ()
{
  u32 p[256];
  for (int i = 0; i < 256; ++i)
    p[i] = SCALE / 2;

  u64 low  = 0;
  u64 high = 0xFFFFFFFFFFFFFFFF;

  u32 remaining = write_header ();

  while (remaining--)
    {
      int sym = getc (input_file);
      for (u32 ctx = 1, mask = 1 << 7; mask != 0; mask >>= 1)
        {
          u64 mid = low + (high - low) / SCALE * p[ctx];
          if (sym & mask)
            {
              high = mid;
              p[ctx] += (SCALE - p[ctx]) / RATE;
              ctx = (ctx << 1) + 1;
            }
          else
            {
              low = mid + 1;
              p[ctx] -= p[ctx] / RATE;
              ctx = (ctx << 1);
            }
          while ((low >> 56) == (high >> 56))
            {
              putc (low >> 56, output_file);
              low  = (low  << 8);
              high = (high << 8) + 0xFF;
            }
        }
    }
  putc (low >> 56, output_file);
  putc (low >> 48, output_file);
  putc (low >> 40, output_file);
  putc (low >> 32, output_file);
  putc (low >> 24, output_file);
  putc (low >> 16, output_file);
  putc (low >>  8, output_file);
  putc (low >>  0, output_file);
}

void
decompress ()
{
  u32 p[256];
  for (int i = 0; i < 256; ++i)
    p[i] = SCALE / 2;

  u64 low  = 0;
  u64 high = 0xFFFFFFFFFFFFFFFF;
  u64 code = 0;

  u32 remaining = read_header ();

  for (int i = 0; i != 8; ++i)
    code = (code << 8) + getc (input_file);

  while (remaining--)
    {
      int sym = 0;
      for (u32 ctx = 1, mask = 1 << 7; mask != 0; mask >>= 1)
        {
          u64 mid = low + (high - low) / SCALE * p[ctx];
          if (code <= mid)
            {
              high = mid;
              p[ctx] += (SCALE - p[ctx]) / RATE;
              ctx = (ctx << 1) + 1;
              sym |= mask;
            }
          else
            {
              low = mid + 1;
              p[ctx] -= p[ctx] / RATE;
              ctx = (ctx << 1);
            }
          while ((low >> 56) == (high >> 56))
            {
              low  = (low  << 8);
              high = (high << 8) + 0xFF;
              code = (code << 8) + getc (input_file);
            }
        }
      putc (sym, output_file);
    }
}
