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
    float xptf0=(float)xpt/iw; // original xpt inset factor
    float yptf0=(float)ypt/ih; // original ypt inset factor
    printf("xptf0=%4.6f yptf0=%4.6f\n", xptf0, yptf0);
    int sx=xpt-(int)XSZ/2.;
    int sy=ypt-(int)YSZ/2.;
    float xptf2=(float)sx/iw; // secondary xpt inset factor
    float yptf2=(float)sy/ih; // secondary ypt inset factor
    printf("xptf2=%4.6f yptf2=%4.6f\n", xptf2, yptf2);
    printf("sx=%4.2f sy=%4.2f\n", sx, sy);
    if( (sx<0) | (sy <0)) {
        printf("Sorry point is too far in. Push further out.\n"); 
        vips_error_exit(NULL); 
    }

    if( vips_crop(in, &out, sx, sy, XSZ, YSZ, NULL) )
        vips_error_exit( NULL );

    g_object_unref(in); 

    if( vips_image_write_to_file( out, argv[4], NULL ) )
        vips_error_exit( NULL );

    g_object_unref( out ); 

    VipsImage *out2;
    float xfac=(float)XSZ/iw;
    float yfac=(float)YSZ/ih;
    float ofac=(yfac>xfac)?yfac:xfac;
    if( vips_resize(in, &out2, ofac, NULL) ) {
        printf("Resize failed.\n");
        vips_error_exit(NULL);
    }
    if( vips_image_write_to_file(out2, argv[5], NULL ) )
        vips_error_exit( NULL );

    g_object_unref( out2 ); 

    return( 0 );
}
