/* MagickCore impleemnt of multimage style resize and crop
 * apparenly t's for wizard programmer under which I most naturally number
 *
 *  compilte with 
 *  cc -o core `pkg-config --cflags --libs MagickCore` core.c
 *  NOTE: MAgickCore-config now exists for this
 *
 *  Checking out http://jeromebelleman.gitlab.io/posts/devops/magick/
 *  But .. won't compile
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <MagickCore/MagickCore.h> // for Archlinux

int main(int argc,char **argv)
{
    Image *inim, *outim;
    RectangleInfo *rinf=calloc(1, sizeof(RectangleInfo));
    rinf->width=640;
    rinf->height=480;

    if (argc != 3) {
        printf("Usage: %s inimname outimname\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    InitializeMagick(*argv);
    ExceptionInfo *e;
    GetExceptionInfo(e);
    ImageInfo *iminf=CloneImageInfo((ImageInfo *) NULL);
    ImageInfo *iminf2=CloneImageInfo((ImageInfo *) NULL);

    strcpy(iminf->filename,argv[1]);
    strcpy(iminf2->filename,argv[2]);
    printf("arg1 %s\n", iminf->filename);
    printf("arg2 %s\n", iminf2->filename);
    inim=ReadImage(iminf,e);
    printf("2: %zu\n", inim->columns);
    printf("3: %zu\n", inim->rows);
    if (e->severity != UndefinedException)
        CatchException(e);

    // outim=ResizeImage(image, rinf->width, rinf->height, LanczosFilter, e);
    outim=ResizeImage(inim, 3000, 2000, LanczosFilter, e);
    printf("After resize: w:%zu/h=%zu\n", outim->columns, outim->rows);
    outim=CropImage(outim, rinf, e);
    printf("After crop: w:%zu/h=%zu\n", outim->columns, outim->rows);
    if (outim == (Image *) NULL)
        MagickError(e->severity,e->reason,e->description);

    strcpy(outim->filename, argv[2]);
    WriteImage(iminf2, outim, e);
    free(rinf);
    DestroyImage(inim);
    DestroyImage(outim);
    DestroyImageInfo(iminf);
    DestroyImageInfo(iminf2);
    DestroyExceptionInfo(e);
    DestroyMagick();
    return 0;
}
