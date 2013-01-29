#include "entropy.h"

#define SCALE 4096
#define RATE  32

void
compress ()
{
  u32 p[256];
  for (int i = 0; i < 256; ++i)
    p[i] = SCALE / 2;

  u32 low  = 0;
  u32 high = 0xFFFFFFFF;

  u32 remaining = write_header ();

  while (remaining--)
    {
      int sym = getc (input_file);
      for (u32 ctx = 1, mask = 1 << 7; mask != 0; mask >>= 1)
        {
          u32 mid = low + (high - low) / SCALE * p[ctx];
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
          while ((low >> 24) == (high >> 24))
            {
              putc (low >> 24, output_file);
              low  = (low  << 8);
              high = (high << 8) + 0xFF;
            }
        }
    }
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

  u32 low  = 0;
  u32 high = 0xFFFFFFFF;
  u32 code = 0;

  u32 remaining = read_header ();

  for (int i = 0; i != 4; ++i)
    code = (code << 8) + getc (input_file);

  while (remaining--)
    {
      int sym = 0;
      for (u32 ctx = 1, mask = 1 << 7; mask != 0; mask >>= 1)
        {
          u32 mid = low + (high - low) / SCALE * p[ctx];
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
          while ((low >> 24) == (high >> 24))
            {
              low  = (low  << 8);
              high = (high << 8) + 0xFF;
              code = (code << 8) + getc (input_file);
            }
        }
      putc (sym, output_file);
    }
}
