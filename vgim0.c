/* Valgrind tests on MagickCore */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MagickCore/MagickCore.h> // for Archlinux

int main(int argc,char **argv)
{
    ExceptionInfo *exception;
    Image *inim;

    if (argc != 2) {
        printf("This program tests magickcore compliance with valgrind. As it show a tendency towards memory leaks.");
        printf("Usage: %s inimname\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    // MagickCoreGenesis(*argv, MagickTrue);
    MagickCoreGenesis(*argv, MagickFalse);
    printf("supposed path: %s\n", *argv); 
    exception=AcquireExceptionInfo();

    // ImageInfo ... we'll just need one of these, for reading it seems fairly essential.
    ImageInfo *iminf=CloneImageInfo((ImageInfo *) NULL);
    strcpy(iminf->filename, argv[1]);
    inim=ReadImage(iminf, exception);
    printf("2: %zu\n", inim->columns);
    printf("3: %zu\n", inim->rows);
    if (exception->severity != UndefinedException)
        CatchException(exception);

    exception=DestroyExceptionInfo(exception);
    DestroyImageInfo(iminf);
    DestroyImage(inim);
    MagickCoreTerminus();
    return 0;
}
