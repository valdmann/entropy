#include "entropy.h"

FILE *input_file, *output_file;
char *input_name, *output_name;

u32
write_header ()
{
  fseek(input_file, 0, SEEK_END);
  u32 length = ftell(input_file);
  fseek(input_file, 0, SEEK_SET);

  putc(length >> 24, output_file);
  putc(length >> 16, output_file);
  putc(length >>  8, output_file);
  putc(length >>  0, output_file);

  return length;
}

u32
read_header ()
{
  u32 length = 0;
  length += getc(input_file) << 24;
  length += getc(input_file) << 16;
  length += getc(input_file) <<  8;
  length += getc(input_file) <<  0;

  return length;
}

int
main (int argc, char **argv)
{
  if (argc != 4 || (argv[1][0] != 'c' && argv[1][0] != 'd'))
    {
      printf ("To compress:   %s c INPUT OUTPUT\n", argv[0]);
      printf ("To decompress: %s d INPUT OUTPUT\n", argv[0]);
      exit (1);
    }

  input_name  = argv[2];
  output_name = argv[3];

  if ((input_file = fopen (input_name, "rb")) == NULL)
    perror (input_name), exit (1);
  if ((output_file = fopen (output_name, "wb")) == NULL)
    perror (output_name), exit (1);

  if (argv[1][0] == 'c') compress ();
  else decompress ();

  fflush(output_file);
  if (ferror( input_file)) perror ( input_name), exit (1);
  if (ferror(output_file)) perror (output_name), exit (1);
  fclose( input_file);
  fclose(output_file);

  return 0;
}
