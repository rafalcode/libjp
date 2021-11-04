#include <stdio.h>
#include <vips/vips.h>

#define XSZ 640
#define YSZ 480

int main( int argc, char **argv )
{
    VipsImage *in;
    double mean;
    VipsImage *out;

    if( VIPS_INIT( argv[0] ) )
        vips_error_exit( NULL ); 

    if( argc != 5 ) {
        printf("4 args: xpt is point of interest, x direction. ypt is same for y dir\n"); 
        vips_error_exit( "usage: %s infile xpt ypt outfile", argv[0] ); 
    }

    if( !(in = vips_image_new_from_file( argv[1], NULL )) )
        vips_error_exit( NULL );

    int xpt=atoi(argv[2]);
    int ypt=atoi(argv[3]);
    int sx=xpt-(int)XSZ/2.;
    int sy=ypt-(int)YSZ/2.;
    if( (sx<0) | (sy <0)) {
        printf("Sorry point is too far in. Push further out.\n"); 
        vips_error_exit(NULL); 
    }

    int iw = vips_image_get_width(in); 
    int ih = vips_image_get_height(in); 

    if( vips_crop(in, &out, sx, sy, XSZ, YSZ, NULL) )
        vips_error_exit( NULL );

    g_object_unref(in); 

    if( vips_image_write_to_file( out, argv[4], NULL ) )
        vips_error_exit( NULL );

    g_object_unref( out ); 

    return( 0 );
}
