# the question surrounding this code is whther it uses the turbo suppoesedly
# lossless jpeg or not.
# the wikipeida entry (https://en.wikipedia.org/wiki/Libjpeg) says yes, it can be lossless.
# so famously this will be about recovereing and using the DCT coefficients.
CC=gcc
CFLAGS=-g -Wall
SPECLIBS=-ljpeg -lm
# SLIBS2=-L/usr/lib64 -lMagickCore
SLIBS2=-lMagickCore
EXECUTABLES=jvba example jpegcrop jcro1 jcro2 ask2 jcro3 ask2a jcro3a jcro4 jcro4a magcore1 magcore2 rescro1 rescro2 rescro3 rescro2a rescro3a vgim0 bl0 gm0 gm1
ARCHINCS=-I/usr/include/ImageMagick-7 -fopenmp -DMAGICKCORE_HDRI_ENABLE=1 -DMAGICKCORE_QUANTUM_DEPTH=16
ARCHLIBS=-lMagickCore-7.Q16HDRI
# particularly for wand on Archlinux:
ARCHINCSW=-I/usr/include/ImageMagick-7 -fopenmp -DMAGICKCORE_HDRI_ENABLE=1 -DMAGICKCORE_QUANTUM_DEPTH=16 # particularly for wand on Archlinux
ARCHLIBSW=-lMagickWand-7.Q16HDRI -lMagickCore-7.Q16HDRI
# particularly for GraphicsMagick on Archlinux:
# GraphicsMagick-config --cppflags --ldflags --libs
ARCHINCSG=-I/usr/include/GraphicsMagick
ARCHLIBSG=-L/usr/lib -Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now -lGraphicsMagick -llcms2 -lfreetype -lXext -lSM -lICE -lX11 -llzma -lbz2 -lz -lltdl -lm -lpthread -lgomp

# jvba: smallest file, just reports isizeo f a barray pointer which, on a amd64 system, was always going to be 8.
jvba: jvba.c
	${CC} ${CFLAGS} -o $@ $^ ${SPECLIBS}

# using not libjpeg, but libmagickcore
magcore1: magcore1.c
	# ${CC} ${CFLAGS} -o $@ $^ ${SLIBS2}
	${CC} ${ARCHINCS} -o $@ $^ ${ARCHLIBS}

magcore2: magcore2.c
	# ${CC} ${CFLAGS} -o $@ $^ ${SLIBS2}
	${CC} ${ARCHINCS} -o $@ $^ ${ARCHLIBS}

rescro1: rescro1.c
	${CC} ${ARCHINCS} -o $@ $^ ${ARCHLIBS}
# following seems to do an nplace resize and ignores the ImageInfo!
rescro2: rescro2.c
	${CC} ${ARCHINCS} -o $@ $^ ${ARCHLIBS}
rescro2a: rescro2a.c
	${CC} ${ARCHINCS} -o $@ $^ ${ARCHLIBS}
rescro3: rescro3.c
	${CC} ${ARCHINCS} -o $@ $^ ${ARCHLIBS}
rescro3a: rescro3a.c
	${CC} ${ARCHINCS} -o $@ $^ ${ARCHLIBS}
vgim0: vgim0.c
	${CC} ${ARCHINCS} -o $@ $^ ${ARCHLIBS}
bl0: bl0.c
	${CC} ${ARCHINCS} -o $@ $^ ${ARCHLIBS}

# I had trouble with supposed memory leaks in IM so I move over to the GraphicsMagick fork
gm0: gm0.c
	${CC} ${ARCHINCSG} -o $@ $^ ${ARCHLIBSG}
# This prog does nothing. It just initialises the GM API environemnt and then destroys it.
# still reachable: 19,324 bytes in 19 blocks
gm1: gm1.c
	${CC} ${ARCHINCSG} -o $@ $^ ${ARCHLIBSG}

# unsure what this is about.
rdswitch.o: rdswitch.c
	${CC} ${CFLAGS} -c $^ ${SPECLIBS}

cdjpeg.o: cdjpeg.c
	${CC} ${CFLAGS} -c $^ ${SPECLIBS}

transupp.o: transupp.c
	${CC} ${CFLAGS} -c $^ ${SPECLIBS}

# transupp reduced just for cropping
transupp2.o: transupp2.c
	${CC} ${CFLAGS} -c $^ ${SPECLIBS}

jpegtran: jpegtran.c transupp.o cdjpeg.o rdswitch.o
	${CC} ${CFLAGS} -o $@ -DTWO_FILE_COMMANDLINE $^ ${SPECLIBS}

# Was a bit hurried with this original
jpegtran2: jpegtran2.c transupp.o cdjpeg.o rdswitch.o
	${CC} ${CFLAGS} -o $@ -DTWO_FILE_COMMANDLINE $^ ${SPECLIBS}

# sq: an attempt at something that compiles and runs but gives a blank image
jpegcrop: jpegcrop.c transupp.o
	${CC} ${CFLAGS} -o $@ -DTWO_FILE_COMMANDLINE $^ ${SPECLIBS}

# this would seem to work. Lumped all transupp in jpegcrop...
# got partially rid of the other transforms Lumped all transupp in jpegcrop...
jcro1: jcro1.c
	${CC} ${CFLAGS} -o $@ -DTWO_FILE_COMMANDLINE $^ ${SPECLIBS}

# experimental on jcro1
jcro2: jcro2.c
	${CC} ${CFLAGS} -o $@ -DTWO_FILE_COMMANDLINE $^ ${SPECLIBS}

# For getting the coords and giving the ouput filenames.
ask2: ask2.c
	${CC} ${CFLAGS} -o $@ $^

# further cleaning 
ask2a: ask2a.c
	${CC} ${CFLAGS} -o $@ $^

# further cleaning 
ask2b: ask2b.c
	${CC} ${CFLAGS} -o $@ $^ $(SPECLIBS)

# It's possible to use jpegtran from system calls actually.
ask3: ask3.c
	${CC} ${CFLAGS} -o $@ $^

# OK, hacking big time now and wanting to stick in the ask2 output.
jcro3: jcro3.c
	${CC} ${CFLAGS} -o $@ -DTWO_FILE_COMMANDLINE $^ ${SPECLIBS}

jcro3a: jcro3a.c
	${CC} ${CFLAGS} -o $@ -DTWO_FILE_COMMANDLINE $^ ${SPECLIBS}

jcro4: jcro4.c
	${CC} ${CFLAGS} -o $@ -DTWO_FILE_COMMANDLINE $^ ${SPECLIBS}

# I take a step backwards, because jcro4 lost its way, so actually this is a copy of jcro3
jcro4a: jcro4a.c
	${CC} ${CFLAGS} -o $@ -DTWO_FILE_COMMANDLINE $^ ${SPECLIBS}

.PHONY: clean

clean:
	rm -f ${EXECUTABLES} *.o
