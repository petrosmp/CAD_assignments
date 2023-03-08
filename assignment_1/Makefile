all: poly-lib main

from-scratch: clean poly-lib main

poly-lib: polyonym.h polyonym.c
	gcc -Wall -shared -fpic -o libpoly.so $(word 2,$^) -lm -g
# -shared	produces a shared object which can then be linked with other
# 			objects to form an executable
# -fpic 	generates position independent code (pic), suitable for the
#       	shared library that we are creating here. Its usage is also
#			encouraged by gcc's manpage
# -lm 		includes math.h (libm.a to be exact)
# the $(word ...) contraption refers to the second prerequisite

main: main.c
	gcc -Wall -L. -Wl,-rpath=. $< -lpoly -lm -g
# -L.			adds the current directory (.) to the list of directories
#     			to be searched for -l, os our polyonym library can be found.
# -Wl,-rpath=.	passes 'rpath=.' as an option to the linker, so it can
#				dynamically link to our polyonym library which is in
#				the current dict (.)
# $< 			refers to the first prerequisite

clean:
	rm -f libpoly.so
	rm -f a.out
