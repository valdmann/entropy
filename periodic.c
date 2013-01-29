#include "entropy.h"

#define SCALE_BITS  14
#define INDEX_BITS  10
#define SEARCH_BITS (SCALE_BITS - INDEX_BITS)
#define SCALE       (1 << SCALE_BITS)
#define INDEX_SIZE  (1 << INDEX_BITS)
#define RATE        8

void
compress ()
{
  u32 cnt[256];
  u32 cum[257];

  for (u32 i = 0; i != 257; ++i)
    cum[i]  = SCALE / 256 * i;
  for (u32 i = 0; i != 256; ++i)
    cnt[i]  = 1;

  u32 spc = SCALE - 256;

  u64 low         = 0;
  u32 range       = 0xFFFFFFFF;
  u32 flux_length = 1;
  u8  flux_first  = 0;

  u32 remaining = write_header ();

  while (remaining--)
    {
      u8 sym = getc (input_file);

      range /= SCALE;
      low   += range * cum[sym];
      range *= cum[sym + 1] - cum[sym];

      ++cnt[sym];
      if (--spc == 0)
        {
          for (u32 sum = 0, i = 0; i != 257; ++i)
            {
              cum[i]  = sum;
              sum    += cnt[i];
            }
          spc = SCALE;
          for (u32 i = 0; i != 256; ++i)
            {
              cnt[i] -= (1 < cnt[i] && cnt[i] < RATE) ? 1 : cnt[i] / RATE;
              spc    -= cnt[i];
            }
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
  u32 cnt[256];
  u32 cum[257];
  u32 idx[INDEX_SIZE];

  for (u32 i = 0; i != 257; ++i)
    cum[i]  = SCALE / 256 * i;
  for (u32 q = 0, i = 0; i != 256; ++i)
    for (; (q << SEARCH_BITS) < cum[i + 1]; ++q)
      idx[q] = i;
  for (u32 i = 0; i != 256; ++i)
    cnt[i]  = 1;

  u32 spc = SCALE - 256;

  u32 range = 0xFFFFFFFF;
  u32 cml   = 0;

  u32 remaining = read_header ();

  for (int i = 0; i != 5; ++i)
    cml = (cml << 8) + getc (input_file);

  while (remaining--)
    {
      range /= SCALE;

      u32 code = cml / range;
      u8 sym = idx[code >> SEARCH_BITS];
      while (code >= cum[sym + 1])
        ++sym;

      cml   -= range * cum[sym];
      range *= cum[sym + 1] - cum[sym];

      ++cnt[sym];
      if (--spc == 0)
        {
          for (u32 sum = 0, i = 0; i != 257; ++i)
            {
              cum[i]  = sum;
              sum    += cnt[i];
            }
          for (u32 q = 0, i = 0; i != 256; ++i)
            for (; (q << SEARCH_BITS) < cum[i + 1]; ++q)
              idx[q] = i;
          spc = SCALE;
          for (u32 i = 0; i != 256; ++i)
            {
              cnt[i] -= (1 < cnt[i] && cnt[i] < RATE) ? 1 : cnt[i] / RATE;
              spc    -= cnt[i];
            }
        }

      while (range <= 0xFFFFFF)
        {
          range <<= 8;
          cml = (cml << 8) + getc (input_file);
        }

      putc (sym, output_file);
    }
}
