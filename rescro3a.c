/* MagickCore impleemnt of multimage style resize and crop
 * apparenly t's for wizard programmer under which I most naturally number
 *
 *  compilte with 
 *  cc -o core `pkg-config --cflags --libs MagickCore` core.c
 *  NOTE: MAgickCore-config now exists for this
 */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<dirent.h> 
#include<sys/time.h>
#include<sys/stat.h>
#include<MagickCore/MagickCore.h> // for Archlinux

#define NEWSZX 640
#define NEWSZY 480

int main(int argc,char **argv)
{
    ExceptionInfo *exception;
    Image *image;
    int i;

    if (argc != 2) {
        printf("Usage: %s inimname\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    MagickCoreGenesis(*argv, MagickTrue);
    exception=AcquireExceptionInfo();
    ImageInfo *iminf=CloneImageInfo((ImageInfo *) NULL);
    RectangleInfo *rinf=calloc(1, sizeof(RectangleInfo));
    rinf->width=NEWSZX;
    rinf->height=NEWSZY;

    strcpy(iminf->filename,argv[1]);
    printf("arg1 %s\n", iminf->filename);
    image=ReadImage(iminf,exception);
    printf("2: %zu\n", image->columns);
    printf("3: %zu\n", image->rows);
    if (exception->severity != UndefinedException)
        CatchException(exception);
    printf("target frame is %i by %i\n", NEWSZX, NEWSZY);

    char tc[1024]={0};
    int lim=1000, steps=2;
    // the default point to focus on should be the middle. By no means all the time, but mostly likely if will be that.
    float xpc, ypc;
    xpc=(float)rinf->width/image->columns;
    ypc=(float)rinf->height/image->rows;
    float c0x=image->columns*xpc/2.; // newim center in x dir
    float c0y=image->rows*ypc/2.; // newim center in y dir
    int c0xi=(int)(0.5+c0x);
    int c0yi=(int)(0.5+c0y);
    printf("Orig center: (%i,%i)\n", c0xi, c0yi);
    for(i=0;i<lim;i+=50) {
        xpc=(float)(NEWSZX+i*steps)/image->columns;
        ypc=(float)(NEWSZY+i*steps)/image->rows;

        c0xi=(int)(0.5 + image->columns*xpc/2.);
        c0yi=(int)(0.5 + image->rows*ypc/2.);
        rinf->x=(int)(i*(float)steps/2.);
        rinf->y=(int)(i*(float)steps/2.);
        printf("Resize factor: w:%4.4f/h=%4.4f new center: (%i,%i) offset: (%zu,%zu)\n", xpc, ypc, c0xi, c0yi, rinf->x, rinf->y);
    }

    // liberate
    free(rinf);
    DestroyImage(image);
    DestroyImageInfo(iminf);
    exception=DestroyExceptionInfo(exception);
    MagickCoreTerminus();
    return 0;
}
