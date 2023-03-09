#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "polyonym.h"

#define ACCEPTABLE_ERROR 0.001
#define DEGREE 5
#define COEFFS_SIZE (DEGREE+1)
#define DELTA 0.001

void newton_raphson(Polyonym *p, double x0, int max_iter, int verbose);
void tangent(Polyonym *p, double x0, int max_iter, int verbose);

int main(int argc, char *argv[]) {

    // check that the number of arguments is the expected one
    if (argc < COEFFS_SIZE+1) {
        printf("Not enough arguments - there need to be exactly %d coefficients!\n", COEFFS_SIZE);
        exit(-1);
    }

    // parse the arguments into a polyonym
    Polyonym *p = init_poly(DEGREE, NULL);
    for(int i=0; i<COEFFS_SIZE; i++) {
        p->coeffs[i] = atoi(argv[i+1]);
    }

    // check whether verbosity is required
    int verbose = 0;
    for(int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
            break;
        }
    }

    print_poly(p);

    newton_raphson(p, 1, 30, verbose);
    tangent(p, 1, 30, verbose);

    free_poly(p);
    return 0;
}

void newton_raphson(Polyonym *p, double x0, int max_iter, int verbose) {

    Polyonym *d = differentiate(p);

    // iterate until we find a value of x where f(x) is close to 0
    double x = x0;
    int iter, found = 0;
    if (verbose) printf("\nNewton-Raphson iterations:\n");
    for(iter=0; iter<max_iter; iter++) {
        if (verbose) printf("[iter %d]\ttrying %lf, where f(x) = %lf\n", iter, x, eval_at(p, x));
        if (fabs(eval_at(p, x)) < ACCEPTABLE_ERROR) {
            found = 1;
            break;
        }
        x = x - (eval_at(p,x) / eval_at(d, x));
    }

    // cleanup and print stats
    free_poly(d);

    if (found) {
        printf("Newton-Raphson converged to %f after %d iterations\n", x, iter+1);
    } else {
        printf("Newton-Raphson did not manage to find a root after %d iterations. Final value: %fs\n", iter+1, x);
    }
}

void tangent(Polyonym *p, double x0, int max_iter, int verbose) {


    // iterate until we find a value of x where f(x) is close to 0
    double x = x0;
    int iter, found = 0;
    if (verbose) printf("\nTangent method iterations:\n");
    for(iter=0; iter<max_iter; iter++) {
        if (verbose) printf("[iter %d]\ttrying %lf, where f(x) = %lf\n", iter, x, eval_at(p, x));
        if (fabs(eval_at(p, x)) < ACCEPTABLE_ERROR) {
            found = 1;
            break;
        }
        x = x - (eval_at(p,x) / ((eval_at(p, x+DELTA) - eval_at(p,x)) / DELTA));
    }

    // print stats
    if (found) {
        printf("The tangent method converged to %f after %d iterations\n", x, iter+1);
    } else {
        printf("The tangent method did not manage to find a root after %d iterations. Final value: %fs\n", iter+1, x);
    }
}
