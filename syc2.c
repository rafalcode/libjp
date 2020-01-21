/* to call jpegtran from c */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    const char *command="./jpegtran -crop 320x240+0+0 ~/images/Messier_81_HST.jpg o.jpg";
    // const char *command="./jpegtran -crop 320x240+0+0 Messier_81_HST.jpg o.jpg";
    printf("Will execute: %s\n", command); 

    int retval;
    retval = system(command);

   return 0;
}
