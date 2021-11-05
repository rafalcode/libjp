#include <stdio.h>
#include <vips/vips.h>

#define XSZ 640
#define YSZ 480

int main( int argc, char **argv )
{
    VipsImage *in, out0, out2;

    if( VIPS_INIT( argv[0] ) )
        vips_error_exit( NULL ); 

    if( argc != 6 ) {
        printf("4 args: xpt is point of interest, x direction. ypt is same for y dir\n"); 
        vips_error_exit( "usage: %s infile xpt ypt outfile1 outfile2", argv[0] ); 
    }

    if( !(in = vips_image_new_from_file( argv[1], NULL )) )
        vips_error_exit( NULL );

    int xpt=atoi(argv[2]);
    int ypt=atoi(argv[3]);
    int iw = vips_image_get_width(in); 
    int ih = vips_image_get_height(in); 

    float xfac=(float)XSZ/iw;
    float yfac=(float)YSZ/ih;
    float ofac=(yfac>xfac)?yfac:xfac;
    if( vips_resize(in, &out2, ofac, NULL) ) {
        printf("Resize failed.\n");
        vips_error_exit(NULL);
    }
    float xpt2=(float)xpt*ofac;
    float ypt2=(float)ypt*ofac;
    printf("xpt2=%4.6f ypt2=%4.6f\n", xpt2, ypt2);
    int xpt2i=(int)(.5+xpt2);
    int ypt2i=(int)(.5+ypt2);
    printf("integerised xpt2=%i ypt2=%i\n", xpt2i, ypt2i);
    if( vips_crop(out0, &out2, sx, sy, XSZ, YSZ, NULL) )
        vips_error_exit( NULL );

    if( vips_image_write_to_file( out2, argv[4], NULL ) )
        vips_error_exit( NULL );

    g_object_unref( in ); 
    g_object_unref( out ); 
    g_object_unref( out2 ); 

    return( 0 );
}
