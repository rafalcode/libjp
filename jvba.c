#include<stdio.h>
#include<stdlib.h>
#include<jpeglib.h>

int main(int argc, char *argv[])
{
//    printf("%zu\n", sizeof(struct jvirt_barray_control)); 
    printf("the size of the barray_ptr is %zu\n", sizeof(jvirt_barray_ptr)); 
   return 0;
}
