#include "entropy.h"

#define SCALE 4096
#define RATE  32
#define BUFSZ 65536

u8 ib[BUFSZ];
u8 ob[BUFSZ];

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

  write_header ();

  u8 *ip = ib;
  u8 *ie = ib;
  u8 *op = ob;
  u8 *oe = ob + BUFSZ;

  for (;;)
    {
      if (unlikely(ip == ie))
        {
          u32 sz = fread (ib, 1, BUFSZ, input_file);
          if (ferror (input_file)) perror (input_name), exit (1);
          ip = ib;
          ie = ib + sz;
          if (ib == ie)
            break;
        }
      int sym = *ip++;
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
                  *op++ = flux_first + hi32, flux_length--;
                  while (unlikely(op + flux_length >= oe))
                    {
                      while (op != oe)
                        *op++ = 0xFF + hi32, flux_length--;
                      fwrite (ob, 1, op - ob, output_file);
                      if (ferror (output_file)) perror (output_name), exit (1);
                      op = ob;
                    }
                  while (flux_length)
                    *op++ = 0xFF + hi32, flux_length--;
                  flux_first = lo32 >> 24;
                }
              ++flux_length;
              low = lo32 << 8;
              range <<= 8;
            }
        }
    }
  fwrite (ob, 1, op - ob, output_file);
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

  u8  *ip    = ib;
  u8  *ie    = ib;
  u8  *op    = ob;
  u8  *oe    = ob + BUFSZ;

  for (int i = 0; i != 5; ++i)
    cml = (cml << 8) + getc (input_file);

  for (;;)
    {
      if (unlikely(op == oe))
        {
          fwrite (ob, 1, op - ob, output_file);
          if (ferror (output_file)) perror (output_name), exit (1);
          if ((remaining -= op - ob) == 0)
            break;
          op = ob;
          oe = ob + (remaining < BUFSZ ? remaining : BUFSZ);
        }
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
          if (range <= 0xFFFFFF)
            {
              if (likely(ie - ip >= 4))
                do
                  {
                    cml = (cml << 8) + *ip++;
                    range <<= 8;
                  }
                while (unlikely(range <= 0xFFFFFF));
              else
                do
                  {
                    if (ip == ie)
                      {
                        u32 sz = fread (ib, 1, BUFSZ, input_file);
                        if (ferror (input_file)) perror (input_name), exit (1);
                        if (sz == 0) printf("%s: eof\n", input_name), exit (1);
                        ip = ib;
                        ie = ib + sz;
                      }
                    cml = (cml << 8) + *ip++;
                    range <<= 8;
                  }
                while (unlikely(range <= 0xFFFFFF));
            }
        }
      *op++ = sym;
    }
}
