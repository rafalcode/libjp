/* MagickCore impleemnt of multimage style resize and crop
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
#include <MagickCore/MagickCore.h> // for Archlinux

int main(int argc,char **argv)
{
    ExceptionInfo *exception;
    // Image *image, *resize_image;
    Image *image, *images, *resize_image, *thumbnails;
    RectangleInfo *rinf={0};
    rinf->width=640;
    rinf->height=480;

    ImageInfo *image_info, *iminf2;
    return 0;

    if (argc != 3) {
        printf("Usage: %s inimname outimname\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    /* Initialize the image info structure and read an image.  */
    /*
    MagickCoreGenesis(*argv, MagickTrue);
    exception=AcquireExceptionInfo();
    image_info=CloneImageInfo((ImageInfo *) NULL);

    iminf2=CloneImageInfo((ImageInfo *) NULL);

    strcpy(image_info->filename,argv[1]);
    strcpy(iminf2->filename,argv[2]);
    image=ReadImage(image_info,exception);
    if (exception->severity != UndefinedException)
        CatchException(exception);

    resize_image=ResizeImage(image, 106, 80, LanczosFilter, exception);
    // resize_image=ResizeImage(image, rinf->width, rinf->height, LanczosFilter, exception);
    if (exception->severity != UndefinedException)
        CatchException(exception);
    resize_image=CropImage(resize_image, rinf, exception);
    if (exception->severity != UndefinedException)
        CatchException(exception);
    if (resize_image == (Image *) NULL)
        MagickError(exception->severity,exception->reason,exception->description);

    WriteImage(iminf2, resize_image, exception);
    DestroyImage(image);
    image_info=DestroyImageInfo(image_info);
    exception=DestroyExceptionInfo(exception);
    MagickCoreTerminus();
    return 0;
    */
}
