#include <stdio.h>
#include <vips/vips.h>
#include<sys/time.h>
#include<sys/stat.h>

#define XSZ 640
#define YSZ 480

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

int main( int argc, char **argv )
{
    VipsImage *in0, *in, *imtmp, *out;
    double mean;

    if( VIPS_INIT( argv[0] ) )
        vips_error_exit( NULL ); 

    if( argc != 4 ) {
        printf("3 args: xpt is point of interest, x direction. ypt is same for y dir\n"); 
        vips_error_exit("usage: %s infile xpt ypt", argv[0]); 
    }

    if( !(in = vips_image_new_from_file( argv[1], NULL )) )
        vips_error_exit( NULL );

    // point of interest
    int xpt=atoi(argv[2]);
    int ypt=atoi(argv[3]);
//    float sxf=(float)xpt-(float)XSZ/2.;
//    float syf=(float)ypt-(float)YSZ/2.;
//    printf("sxf=%4.6f syf=%4.6f\n", sxf, syf);
    int sx=xpt-(int)XSZ/2.;
    int sy=ypt-(int)YSZ/2.;
    if( (sx<0) | (sy <0)) {
        printf("Sorry point is too far in. Push further out.\n"); 
        vips_error_exit(NULL); 
    }

    char tc[1024]={0};
    int lim=1000, steps=2;
    char *tmpd=mktmpd();
    printf("Your subimages will go into directory \"%s\"\n", tmpd);

    int iw = vips_image_get_width(in); 
    int ih = vips_image_get_height(in); 
    printf("rszw:%ih:%i\n", iw, ih);
    printf("sxfrac=%4.6f syfrac=%4.6f\n", (float)sx/iw, (float)sy/ih);
    float xfac, yfac; // resize factor in either direction.
    float c0x=sx*xfac; // newim center in x dir
    float c0y=sy*yfac; // newim center in x dir

    int i;
    float ofac;
    for(i=380;i<400;i+=1) {
        xfac=(float)(XSZ+i*steps)/iw;
        yfac=(float)(YSZ+i*steps)/ih;
        ofac=(yfac>xfac)?yfac:xfac;
        printf("xfac=%4.4f yfac=%4.4f\n", xfac, yfac); 
        // if( vips_resize(in, &out, xfac, yfac, NULL) ) {
        if( vips_resize(in, &imtmp, ofac, NULL) ) {
            printf("Resize failed @ i=%i.\n", i); 
            vips_error_exit(NULL);
        }
#ifdef DBG2
        printf("rszw:%ih:%i\n",vips_image_get_width(imtmp), vips_image_get_height(imtmp));
        printf("sx=%i goes to %4.4f\n", xpt, (float)xpt*ofac);
        printf("sy=%i goes to %4.4f\n", ypt, (float)ypt*ofac);
#endif
        sx=(int)(0.+(float)xpt*ofac-(float)XSZ/2.);
        sy=(int)(0.+(float)ypt*ofac-(float)YSZ/2.);

        printf("sx=%i sy=%i\n", sx, sy); 
        if( (sx<0) | (sy <0))
            continue;
        if( vips_crop(imtmp, &out, sx, sy, XSZ, YSZ, NULL) ) {
            printf("Crop failed @ i=%i.\n", i); 
            vips_error_exit(NULL);
        }
        sprintf(tc, "%s/im%04i.jpg", tmpd, i);
        if( vips_image_write_to_file( out, tc, NULL ) ) {
            printf("imwrite failed @ i=%i.\n", i); 
            vips_error_exit( NULL );
        }
    }
    g_object_unref(out); 
    g_object_unref(in); 
    g_object_unref(imtmp); 
    return( 0 );
}
