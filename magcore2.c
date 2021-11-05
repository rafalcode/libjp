/* MagickCore is the name of the ImageMagick API
 * apparenly t's for wizard programmer under which I most naturally number
 *
 *  compilte with 
 *  cc -o core `pkg-config --cflags --libs MagickCore` core.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// #include <magick/MagickCore.h>
// for Arch
#include <MagickCore/MagickCore.h>

int main(int argc,char **argv)
{
    ExceptionInfo *exception;

    Image *image, *images, *resize_image, *thumbnails;

    ImageInfo *image_info;

    if (argc != 2) {
        (void) fprintf(stdout,"Usage: 1 arg : %s imagefile\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    /* Initialize the image info structure and read an image.  */

//  printf("1: %s\n", image_info->size);
  /*
    *size,
    *extract,
    *page,
    *scenes;

  unsigned long
    scene,
    number_scenes,
    depth;

  InterlaceType
    interlace;

  EndianType
    endian;

  ResolutionType
    units;

  unsigned long
    quality;

  char
    *sampling_factor,
    *server_name,
    *font,
    *texture,
    *density;

  double
    pointsize,
    fuzz;

  PixelPacket
    background_color,
    border_color,
    matte_color;

  MagickBooleanType
    dither,
    monochrome;

  unsigned long
    colors;

  ColorspaceType
    colorspace;

  ImageType
    type;

  PreviewType
    preview_type;

  long
    group;

  MagickBooleanType
    ping,
    verbose;

  char
    *view,
    *authenticate;

  ChannelType
    channel;

  Image
    *attributes;

  void
    *options;

  MagickProgressMonitor
    progress_monitor;

  void
    *client_data,
    *cache;

  StreamHandler
    stream;

  FILE
    *file;

  void
    *blob;

  size_t
    length;

  char
    magick[MaxTextExtent],
    unique[MaxTextExtent],
    zero[MaxTextExtent],
    filename[MaxTextExtent];

  MagickBooleanType
    debug;

  char
    *tile;

  unsigned long
    signature;

  VirtualPixelMethod
    virtual_pixel_method;

  PixelPacket
    transparent_color;

  void
    *profile;

  MagickBooleanType
    synchronize;
}
*/

    printf("%s\n", *argv); 
    MagickCoreGenesis(*argv, MagickTrue);
    exception=AcquireExceptionInfo();
    image_info=CloneImageInfo((ImageInfo *) NULL);

    strcpy(image_info->filename,argv[1]);
    image=ReadImage(image_info,exception);
    printf("1: %s\n", image_info->size);
    printf("2: %zu\n", image->columns);
    printf("3: %zu\n", image->rows);
    if (exception->severity != UndefinedException)
        CatchException(exception);
    if (images == (Image *) NULL)
        exit(EXIT_FAILURE);

    /* Convert the image to a thumbnail. 
    thumbnails=NewImageList();
    while ((image=RemoveFirstImageFromList(&images)) != (Image *) NULL) {

        resize_image=ResizeImage(image, 106, 80, LanczosFilter, 1.0, exception);

        if (resize_image == (Image *) NULL)
            MagickError(exception->severity,exception->reason,exception->description);

        AppendImageToList(&thumbnails,resize_image);

        DestroyImage(image);
    }
    strcpy(thumbnails->filename,argv[2]);
    WriteImage(image_info,thumbnails);
       Destroy the image thumbnail and exit.
    thumbnails=DestroyImageList(thumbnails);
    */
    DestroyImage(image);
    image_info=DestroyImageInfo(image_info);
    exception=DestroyExceptionInfo(exception);
    MagickCoreTerminus();
    return 0;
}
