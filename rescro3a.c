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

char *mktmpd(void)
{
    struct timeval tsecs;
    gettimeofday(&tsecs, NULL);
    char *myt=calloc(14, sizeof(char));
    strncpy(myt, "tmpdir_", 7);
    sprintf(myt+7, "%lu", tsecs.tv_usec);

    DIR *d;
    while((d = opendir(myt)) != NULL) { /* see if a directory witht the same name exists ... very low likelihood though */
        gettimeofday(&tsecs, NULL);
        sprintf(myt+7, "%lu", tsecs.tv_usec);
        closedir(d);
    }
    closedir(d);
    mkdir(myt, S_IRWXU);

    return myt;
}

int main(int argc,char **argv)
{
    ExceptionInfo *exception;
    Image *image, *outim;
    RectangleInfo *rinf=calloc(1, sizeof(RectangleInfo));
    rinf->width=NEWSZX;
    rinf->height=NEWSZY;
    int i;

    if (argc != 2) {
        printf("Usage: %s inimname\n",argv[0]);
        exit(EXIT_FAILURE);
    }
	char *tmpd=mktmpd();
	printf("Your subimages will go into directory \"%s\"\n", tmpd);

    MagickCoreGenesis(*argv, MagickTrue);
    exception=AcquireExceptionInfo();
    ImageInfo *iminf=CloneImageInfo((ImageInfo *) NULL);

    strcpy(iminf->filename,argv[1]);
    printf("arg1 %s\n", iminf->filename);
    image=ReadImage(iminf,exception);
    printf("2: %zu\n", image->columns);
    printf("3: %zu\n", image->rows);
    if (exception->severity != UndefinedException)
        CatchException(exception);

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
    for(i=0;i<lim;i+=1) {
        xpc=(float)(NEWSZX+i*steps)/image->columns;
        ypc=(float)(NEWSZY+i*steps)/image->rows;
        outim=ResizeImage(image, NEWSZX+i*steps, NEWSZY+i*steps, LanczosFilter, exception);
        printf("After resize: w:%zu/h=%zu\n", outim->columns, outim->rows);
        c0xi=(int)(0.5 + image->columns*xpc/2.);
        c0yi=(int)(0.5 + image->rows*ypc/2.);
        printf("new center: (%i,%i)\n", c0xi, c0yi);
        rinf->width=NEWSZX;
        rinf->height=NEWSZY;
        // rinf->x=(int)((i*steps/2.) - (float)NEWSZX/2.);
        // rinf->y=(int)((i*steps/2.) - (float)NEWSZY/2.);
        rinf->x=(int)(i*steps/2.);
        rinf->y=(int)(i*steps/2.);
        printf("offset: (%i,%i)\n", rinf->x, rinf->y);
        outim=CropImage(outim, rinf, exception);
        printf("After crop: w:%zu/h=%zu\n", outim->columns, outim->rows);
        if (outim == (Image *) NULL)
            MagickError(exception->severity,exception->reason,exception->description);

        sprintf(tc, "%s/im%03i.jpg", tmpd, i);
        strcpy(outim->filename, tc);
        printf("destdir: %s\n", outim->filename);
        WriteImage(iminf, outim, exception);
    }

    // liberate
    free(rinf);
    free(tmpd);
    DestroyImage(image);
    DestroyImage(outim);
    DestroyImageInfo(iminf);
    exception=DestroyExceptionInfo(exception);
    MagickCoreTerminus();
    return 0;
}
