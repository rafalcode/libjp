#include <stdio.h>
#include <vips/vips.h>

#define XSZ 640
#define YSZ 480

int main( int argc, char **argv )
{
    VipsImage *in, *out0, *out2;

    if( VIPS_INIT( argv[0] ) )
        vips_error_exit( NULL ); 

    if( argc != 5 ) {
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
    ofac+=.3;
    if( vips_resize(in, &out0, ofac, NULL) ) {
        printf("Resize failed.\n");
        vips_error_exit(NULL);
    }
    float xpt2=(float)xpt*ofac;
    float ypt2=(float)ypt*ofac;
    int iw2 = vips_image_get_width(out0); 
    int ih2= vips_image_get_height(out0); 
    printf("In resized image, ptofint is float: xpt2=%4.6f ypt2=%4.6f\n", xpt2, ypt2);
    int xpt2i=(int)(.5+xpt2);
    int ypt2i=(int)(.5+ypt2);
    printf("integerised in resized uncropped image %i & %i\n", xpt2i, ypt2i);
    int sxi=xpt2i-XSZ/2;
    int syi=ypt2i-YSZ/2;
    printf("For viewport we would offset at sxi=%i syi=%i\n", sxi, syi);
    if( vips_crop(out0, &out2, sxi, syi, XSZ, YSZ, NULL) )
        vips_error_exit( NULL );

    if( vips_image_write_to_file( out2, argv[4], NULL ) )
        vips_error_exit( NULL );

    g_object_unref( in ); 
    g_object_unref( out0 ); 
    g_object_unref( out2 ); 

    return( 0 );
}
