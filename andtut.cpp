/* I got the following code from Andrew White.
 * ref. http://www.andrewewhite.net/wordpress/2008/09/02/very-simple-jpeg-writer-in-c-c/
 * However, its very lazy .. one r two functions are mentioend which can't be found anywhere.
 * in Xlib nor on the net.
 * it seems to be pseudo code really.
 * and distracted me from my purpose as usual. */
/* First, do some setup and take a screen shot using Xlib. Note that Xlib this has next to nothing to do with the JPEG code and is only included to create a context. In the end, the info and screen_shot object will have some meta-data about the image that we will be making into a JPEG. The buffer vector will hold raw RGB values for screen shot. */
#include <jpeglib.h>

int main()
{
    vector<char> buffer;
    XInfo_t xinfo  = getXInfo(":0");
    XImage* screen_shot = takeScreenshot(xinfo, buffer);
    /* Now we need to open an output file for the jpeg. */
    FILE* outfile = fopen("/tmp/andtest.jpeg", "wb");

    if (!outfile)
        throw FormattedException("%s:%d Failed to open output file", __FILE__, __LINE__);
    /* Now, lets setup the libjpeg objects. Note this is setup for my screen shot so we are using RGB color space. */
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width      = xinfo.width;
    cinfo.image_height     = xinfo.height;
    cinfo.input_components = 3;
    cinfo.in_color_space   = JCS_RGB;
    /* Call the setup defualts helper function to give us a starting point. Note, don’t call any of the helper functions before you call this, they will no effect if you do. Then call other helper functions, here I call the set quality. Then start the compression.*/
    jpeg_set_defaults(&cinfo);
    /*set the quality [0..100]  */
    jpeg_set_quality (&cinfo, 75, true);
    jpeg_start_compress(&cinfo, true);
    /* Next we setup a pointer and start looping though lines in the image. Notice that the row_pointer is set to the first byte of the row via some pointer math/magic. Once the pointer is calculated, do a write_scanline. */
    JSAMPROW row_pointer;          /* pointer to a single row */

    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer = (JSAMPROW) &buffer[cinfo.next_scanline*(screen_shot->depth>>3)*screen_shot->width];
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }
    /* Finally, call finish and in my case close the connection to the X server. */
    jpeg_finish_compress(&cinfo);
    XCloseDisplay(xinfo.display); /* standard one: in Xlib.h */
}
