These are examples of using some well know image processing libraries.

MagickCore is Imagemagick's C API. There seems to be quite a difference between its version 6 and version 7.
Many examples on the web deal with version 6 which is not so relevant for version 7.

With even the barest of programs, such as vgim0, valgrind reports "still reachable" issues. These are not ultra import
but it seems hard to avoid them.

Different resize filters:
QuadraticFilter CubicFilter LanczosFilter

You'd expect libvips to be different,
but even it itslef is throwing mem errors with the hello world program.
