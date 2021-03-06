# the question surrounding this code is whther it uses the turbo suppoesedly
# lossless jpeg or not.
# the wikipeida entry (https://en.wikipedia.org/wiki/Libjpeg) says yes, it can be lossless.
# so famously this will be about recovereing and using the DCT coefficients.
CC=gcc
CFLAGS=-g -Wall
SPECLIBS=-ljpeg -lm
# SLIBS2=-L/usr/lib64 -lMagickCore
SLIBS2=-lMagickCore
EXECUTABLES=jvba example jpegcrop jcro1 jcro2 ask2 jcro3 ask2a jcro3a jcro4 jcro4a magcore1

# jvba: smallest file, just reports isizeo f a barray pointer which, on a amd64 system, was always going to be 8.
jvba: jvba.c
	${CC} ${CFLAGS} -o $@ $^ ${SPECLIBS}

# using not libjpeg, but libmagickcore
magcore1: magcore1.c
	${CC} ${CFLAGS} -o $@ $^ ${SLIBS2}

magcore2: magcore2.c
	${CC} ${CFLAGS} -o $@ $^ ${SLIBS2}

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
