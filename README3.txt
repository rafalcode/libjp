


I was mystified by the output file parameter. In truth the program seems to want to printout the binary stream to STDOUT, so you have to pipe it into a jpg filename. I thought this was unusual, but it is stated somewhere, so it's part of the philosphy behind it.
the some sort fo scanline function in the rswicth.h header which is able to scan lines acording to data in a text file / script
tried to take it out.

I tried to free myself of the transupp, but the big issue is the workspace_request function, which is pretty big and uses alot fo functions, so most probably I shoudl be happy to stick with transupp.c

well I thought I had made some progress but not so:
jcro1 gives this:
Empty JPEG image (DNL not supported)


I thought I had it working, but clearly I don't.


Anyhow I managed to squeeze everything into jcro2.c

ANd finally with jcro3.c I had got rid of everything that wasn't a crop.

Unfortunately, "DCT coefficient out of range" is what I was getting.
this happens at the 
jpeg_read_coefficients(&srcinfo)
line
and I seemed to think that the parse arguments was what awas causing this.


I did include all the srcinfo components here but I sorted it all out.

However later on I was getting errors when the ofset was zero or close to zero. This has to o with the iMCU setting which ar eusually 8x8, and the crop has to start at an appropriate part of the image. I was also confused with the transformoption data struct, there being several memeber which sound the sae crop_xoffset and x_crop_offset begint he prime examples.

x_crop_offset is the one though, and if it's close to zero do_crop will not be called. SO the truth of the matter is that do_crop is actually not called at the origin. So how does it crop it? It seems to have alreayd been done before it.

ask2a
-----
I was stumped with the fn member not gettign some of the names right if cw<ch, but Ok if cw>ch. Turned out the I was gettign them mixed up.
vtimes: vertical is the number of rows which is controlled by ch, which is "canvas height" though you culd thnk the "h" meeant hoirzontal too.

What are the markers?
---------------------
I'm not sure, but for naive cropping they weren't used. The amrkers setuo needs to be called before reading.

Are they some sort of poitner to the src image: dicth them for the naive version */

However, things are simply not that easy.

jerror.h:JMESSAGE(JERR_BAD_VIRTUAL_ACCESS, "Bogus virtual array access")
------------------------------------------------------------------------
this was what I was getting. this is issued from jmemmgr, so we're really it he depths here now.

Poor testing results in some wasted work
----------------------------------------
When I moved from jcro3 to jcro4, I started to do tests with 0,0 offset. Big mistake. That's a very easy test and as I already knew does nt go through do_crop at all. So I kept on modifying jcro4 until I started getting errors. Too late.


Quick and dirty finished.
-------------------------
Because I ran into trouble with jcro4, I decided to hold back and be happy with jcro3, which I modified to take arguments and called jcro3a.
Then I modied the ask-series and ask2b now calls jcro3a.

These will more or less cut down a jpg losslessly. WIth something liek the family picture, it's quite fast, but the Messier takes ages.

BEWARE: th Messier test gives 1028 wide images at the rightmost edge. Is this an error, or is a DCT effect? 

OK... there seems to be a conclusion, but where?

Unfortunately after leavng this for a while I was under the impression 
that I had a sort of workabele, though in elegant solutions, whereby I would call one of these jpegtran hacks
and split up the file.

However, its names was undocumented.
