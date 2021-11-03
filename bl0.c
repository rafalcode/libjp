// not goign to work based on version 6.
// // also missing a miff input file.
#include <stdio.h>
#include <stdlib.h>
#include <MagickCore/MagickCore.h> // for Archlinux

void modify_img()
{
  size_t                        size;
  unsigned char                 *blob;
  MagickPixelPacket             background;
  ImageInfo                     *image_info;
  Image *image;
  ExceptionInfo                 *exception;
  FILE *file;

  memset(&background,0,sizeof(MagickPixelPacket));
  exception = AcquireExceptionInfo();
  image_info = AcquireImageInfo();
  strcpy(image_info->filename, "dummy.pnm");
  image_info->quality = 90;
  image = NewMagickImage(image_info, 512, 512, &background);
  size = 0;
  blob = ImageToBlob(image_info, image, &size, exception);
  if (exception->severity != UndefinedException)
    CatchException(exception);
  file=fopen("test.miff","wb");
  fwrite(blob, size, 1, file);
  fclose(file);
  free(blob);
  DestroyImage(image);
  DestroyImageInfo(image_info);
  DestroyExceptionInfo(exception);
}

int main()
{
  modify_img();
  return 0;
}
