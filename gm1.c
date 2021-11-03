/* Testing Graphicmagick for valgrind errors */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <magick/api.h>

int main ( int argc, char **argv )
{
  // Image *image = (Image *) NULL;
  int exit_status = 0;

  ImageInfo *imageInfo;
  ExceptionInfo exception;

  InitializeMagick(NULL);
  // imageInfo=CloneImageInfo(0);
  // GetExceptionInfo(&exception);

  // if (argc != 2) {
  //     (void) fprintf ( stderr, "Usage: %s infile\n", argv[0] );
  //     (void) fflush(stderr);
  //     exit_status = 1;
  //     goto program_exit;
  //   }

  // strcpy(imageInfo->filename, argv[1]);
  // image = ReadImage(imageInfo, &exception);
  // if (image == (Image *) NULL) {
  //     CatchException(&exception);
  //     exit_status = 1;
  //     goto program_exit;
  //   }

 program_exit:

  // if (image != (Image *) NULL)
  //   DestroyImage(image);

  // if (imageInfo != (ImageInfo *) NULL)
  //   DestroyImageInfo(imageInfo);
  DestroyMagick();

  return exit_status;
}
