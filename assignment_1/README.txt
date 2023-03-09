COMPILATION INSTRUCTIONS:
    - On Linux, just run make
    - On Windows:
        > gcc -Wall -shared -fpic -o libpoly.dll polyonym.c -lm -g
        > gcc -Wall -L. main.c -lpoly -lm -g

ARGUMENTS:
    There need to be 6 coefficients, representing a 5-th degree polyonym.
    For example, for the polyonym 8x^5 + 3x^4 + 6x^3 + 2x^2 + 0x + 12,
    the arguments would be 8 3 6 2 0 12.

    There is a verbose mode, where x and f(x) is printed for every iteration.
    To enable it, pass '-v' as an argument.
