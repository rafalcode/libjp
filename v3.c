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
    if( vips_crop( image, &page[i],
      left, page_height * i + top, width, height, NULL ) )
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

int 
main( int argc, char **argv )
{
  VipsImage *image;
  VipsObject *context;
  VipsImage *x;

  if( VIPS_INIT( NULL ) ) 
    vips_error_exit( NULL ); 

  if( !(image = vips_image_new_from_file( argv[1], "access", VIPS_ACCESS_SEQUENTIAL, NULL )) )
    vips_error_exit( NULL ); 

  context = VIPS_OBJECT( vips_image_new() );
  if( crop_animation( context, image, &x, 10, 10, 500, 500 ) ) {
    g_object_unref( image );
    g_object_unref( context );
    vips_error_exit( NULL ); 
  }
  g_object_unref( image );
  g_object_unref( context );
  image = x;

  if( vips_image_write_to_file( image, argv[2], NULL ) ) {
    g_object_unref( image );
    vips_error_exit( NULL ); 
  }

  g_object_unref( image );

  return 0 ;
}
