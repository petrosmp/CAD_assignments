all: poly_lib find_roots

from-scratch: clean poly_lib find_roots

poly_lib: polynomial.h polynomial.c
	gcc -Wall -shared -fpic -o libpoly.so $(word 2,$^) -lm -g
# -shared	produces a shared object which can then be linked with other
# 			objects to form an executable
# -fpic 	generates position independent code (pic), suitable for the
#       	shared library that we are creating here. Its usage is also
#			encouraged by gcc's manpage
# -lm 		includes math.h (libm.a to be exact)
# the $(word ...) contraption refers to the second prerequisite

find_roots: find_roots.c
	gcc -Wall -L. -Wl,-rpath=. $< -o $@ -lpoly -lm -g
# -L.			adds the current directory (.) to the list of directories
#     			to be searched for -l, os our polynomial library can be found.
# -Wl,-rpath=.	passes 'rpath=.' as an option to the linker, so it can
#				dynamically link to our polynomial library which is in
#				the current dict (.)
# $< 			refers to the first prerequisite

clean:
	rm -f libpoly.so
	rm -f find_roots
