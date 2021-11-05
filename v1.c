 /* libvips hello world, but does it leak? */
#include <stdio.h>
#include <vips/vips.h>

int main (int argc, char **argv)
{
  VipsImage *im;
  double avg;

  if (VIPS_INIT (argv[0]))
      vips_error_exit ("unable to start VIPS");

  if (argc != 2)
      vips_error_exit ("usage: %s <filename>", g_get_prgname ());

  if (!(im = vips_image_new_from_file (argv[1], NULL)))
      vips_error_exit ("unable to open");
  if (vips_avg (im, &avg, NULL))
      vips_error_exit ("unable to find avg");
  g_object_unref (im);

  printf ("Hello World!\nPixel average of %s is %g\n", argv[1], avg);

  return (0);
}
