/* MagickCore is the name of the ImageMagick API
 * apparenly t's for wizard programmer under which I most naturally number
 *
 *  compilte with 
 *  cc -o core `pkg-config --cflags --libs MagickCore` core.c
 *  NOTE: MAgickCore-config now exists for this
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

    if (argc != 3) {
        (void) fprintf(stdout,"Usage: %s image thumbnail\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    /* Initialize the image info structure and read an image.  */
    MagickCoreGenesis(*argv, MagickTrue);
    exception=AcquireExceptionInfo();
    image_info=CloneImageInfo((ImageInfo *) NULL);

    (void) strcpy(image_info->filename,argv[1]);
    images=ReadImage(image_info,exception);
    if (exception->severity != UndefinedException)
        CatchException(exception);
    if (images == (Image *) NULL)
        exit(EXIT_FAILURE);

    /* Convert the image to a thumbnail.  */
    thumbnails=NewImageList();
    while ((image=RemoveFirstImageFromList(&images)) != (Image *) NULL) {

        // resize_image=ResizeImage(image, 106, 80, LanczosFilter, 1.0, exception);
        resize_image=ResizeImage(image, 106, 80, LanczosFilter, exception);
        if (resize_image == (Image *) NULL)
            MagickError(exception->severity,exception->reason,exception->description);

        AppendImageToList(&thumbnails,resize_image);

        DestroyImage(image);
    }
    /* Write the image thumbnail.  */
    strcpy(thumbnails->filename,argv[2]);
    WriteImage(image_info,thumbnails, exception);
    /*
       Destroy the image thumbnail and exit.
       */
    thumbnails=DestroyImageList(thumbnails);
    image_info=DestroyImageInfo(image_info);
    exception=DestroyExceptionInfo(exception);
    MagickCoreTerminus();
    return 0;
}
