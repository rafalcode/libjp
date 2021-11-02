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
    Image *inim, *outim;
    RectangleInfo *rinf=calloc(1, sizeof(RectangleInfo));
    rinf->width=640;
    rinf->height=480;

    if (argc != 3) {
        printf("Usage: %s inimname outimname\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    MagickCoreGenesis(*argv, MagickTrue);
    exception=AcquireExceptionInfo();

    // ImageInfo ... we'll just need one of these, for reading it seems fairly essential.
    ImageInfo *iminf=CloneImageInfo((ImageInfo *) NULL);
    strcpy(iminf->filename, argv[1]);
    inim=ReadImage(iminf, exception);
    printf("2: %zu\n", inim->columns);
    printf("3: %zu\n", inim->rows);
    if (exception->severity != UndefinedException)
        CatchException(exception);

    // outim=ResizeImage(image, rinf->width, rinf->height, LanczosFilter, exception);
    outim=ResizeImage(inim, 3000, 2000, LanczosFilter, exception);
    printf("After resize: w:%zu/h=%zu\n", outim->columns, outim->rows);
    outim=CropImage(outim, rinf, exception);
    printf("After crop: w:%zu/h=%zu\n", outim->columns, outim->rows);
    if (outim == (Image *) NULL)
        MagickError(exception->severity,exception->reason,exception->description);

    strcpy(outim->filename, argv[2]);
    WriteImage(iminf, outim, exception);
    free(rinf);
    DestroyImage(inim);
    DestroyImage(outim);
    DestroyImageInfo(iminf);
    exception=DestroyExceptionInfo(exception);
    MagickCoreTerminus();
    return 0;
}
