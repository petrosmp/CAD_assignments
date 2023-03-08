#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "polyonym.h"

#define ACCEPTABLE_ERROR 0.001
#define DEGREE 5
#define COEFFS_SIZE (DEGREE+1)

int main(int argc, char *argv[]) {

    // check that the number of arguments is the expected one
    if (argc < COEFFS_SIZE+1) {
        printf("Not enough arguments - there need to be exactly %d coefficients!\n", COEFFS_SIZE);
        exit(-1);
    }

    // parse the arguments into a polyonym
    Polyonym *p = init_poly(DEGREE, NULL);
    for(int i=0; i<COEFFS_SIZE; i++) {
        p->coeffs[i] = (int)strtol(argv[i+1], (char **)NULL, 10);   // although we don't actually check for errors, strto* is still preferable to ato*, see https://wiki.sei.cmu.edu/confluence/display/c/ERR07-C.+Prefer+functions+that+support+error+checking+over+equivalent+functions+that+don%27t
    }

    print_poly(p);
    printf("The value of the polyonym at x=%d is: %f\n", 5, evaluate_poly_at_x(p, 5));
    Polyonym *d = differentiate(p);
    printf("The first derivative of p is the following:\n");
    
    /*
    for(int i=0; i<d.degree+1; i++){
        printf("der.coeffs[%d] = %d\n", i, d.coeffs[i]);
    }
    */
    
    print_poly(d);

    free_poly(p);
    free_poly(d);

    return 0;
}

