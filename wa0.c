// Last updated 2008/11/04 10:53

// convert logo: -filter lanczos -resize 50% -quality 95 logo_resize.jpg
// Read an image, resize it by 50% and sharpen it, and then save as
// a high quality JPG
// Note that ImageMagick's default quality is 75.

#include <windows.h>
#include <wand/magick_wand.h>

void test_wand(void)
{
	MagickWand *mw = NULL;
	
	int width,height;
	
	MagickWandGenesis();
	
	mw = NewMagickWand();
	// Read the image - all you need to do is change "logo:" to some other
	// filename to have this resize and, if necessary, convert a different file
	MagickReadImage(mw,"logo:");
	
	// Get the image's width and height
	width = MagickGetImageWidth(mw);
	height = MagickGetImageHeight(mw);
	
	// Cut them in half but make sure they don't underflow
	if((width /= 2) < 1)
        width = 1;
	if((height /= 2) < 1)
        height = 1;
	
	// Resize the image using the Lanczos filter
	// The blur factor is a "double", where > 1 is blurry, < 1 is sharp
	// I haven't figured out how you would change the blur parameter of MagickResizeImage
	// on the command line so I have set it to its default of one.
	MagickResizeImage(mw,width,height,LanczosFilter,1);
	
	// Set the compression quality to 95 (high quality = low compression)
	MagickSetImageCompressionQuality(mw,95);
	
	/* Write the new image */
	MagickWriteImage(mw,"logo_resize.jpg");
	
	/* Clean up */
	if(mw)mw = DestroyMagickWand(mw);
	
	MagickWandTerminus();
}
