#include <vips/vips.h>

static int crop_animation( VipsObject *context, VipsImage *image, VipsImage **out, int left, int top, int width, int height )
{
  int page_height = vips_image_get_page_height( image );
  int n_pages = image->Ysize / page_height;
  VipsImage **page = (VipsImage **) vips_object_local_array( context, n_pages );
  VipsImage **copy = (VipsImage **) vips_object_local_array( context, 1 );

  int i;

  /* Split the image into cropped frames.
   */
  for( i = 0; i < n_pages; i++ )
    if( vips_crop( image, &page[i], left, page_height * i + top, width, height, NULL ) )
      return( -1 );

  /* Reassemble the frames and set the page height. You must copy before 
   * modifying metadata.
   */
  if( vips_arrayjoin( page, &copy[0], n_pages, "across", 1, NULL ) ||
    vips_copy( copy[0], out, NULL ) )
    return( -1 );
  vips_image_set_int( *out, "page-height", height );

  return( 0 );
}

int main( int argc, char **argv )
{
	if(argc!=3) {
		printf("Error. Pls supply 2 arguments: 1) input image 2) output image files.\n");
		exit(EXIT_FAILURE);
	}
  if(VIPS_INIT(NULL)) 
    vips_error_exit(NULL); 

  VipsImage *imin;
  VipsImage *imout;

  if( !(imin = vips_image_new_from_file( argv[1], "access", VIPS_ACCESS_SEQUENTIAL, NULL )) )
    vips_error_exit( NULL ); 

  // context = VIPS_OBJECT( vips_image_new() );
  // if( crop_animation( context, image, &x, 10, 10, 500, 500 ) ) {
  //   g_object_unref( image );
  //   g_object_unref( context );
  //   vips_error_exit( NULL ); 
  // }
 vips_crop(imin, &imout, 200, 100, 640, 480, NULL); // final one libdocs say "NULL-terminated list of optional named arguments"

  if( vips_image_write_to_file(imout, argv[2], NULL ) ) {
    g_object_unref(imout);
    vips_error_exit(NULL); 
  }

  g_object_unref(imin);
  return 0 ;
}
