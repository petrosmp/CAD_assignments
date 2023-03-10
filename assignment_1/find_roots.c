#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include "polynomial.h"
#include <ctype.h>

#define ACCEPTABLE_ERROR 0.001
#define DEGREE 5
#define COEFFS_SIZE (DEGREE+1)
#define DELTA 0.001
#define DEFAULT_X0 1.0
#define MAX_ITER 30

void newton_raphson(Polynomial *p, double x0, int max_iter, int verbose);
void tangent(Polynomial *p, double x0, double delta, int max_iter, int verbose);

int main(int argc, char *argv[]) {

    // check that the number of arguments is the expected one
    if (argc < COEFFS_SIZE+1) {
        printf("Not enough arguments - there need to be exactly %d coefficients!\n", COEFFS_SIZE);
        exit(-1);
    }

    // parse the arguments into a polynomial
    Polynomial *p = init_poly(DEGREE, NULL);
    for(int i=0; i<COEFFS_SIZE; i++) {
        p->coeffs[i] = atoi(argv[i+1]);
    }

    // parse the rest of the arguments
    char ch;
    int verbose = 0;
    double d = DELTA;
    double x0 = DEFAULT_X0;
    int max_iter = MAX_ITER;
    while ((ch = getopt(argc, argv, "vd:x:i:1234567890")) != -1) {
        
        // avoid mistaking negative numbers for options
        if (isdigit(ch) != 0) {
            continue;
        }

		switch (ch) {		
			case 'v':
				verbose = 1;
				break;
			case 'd':
				d = atof(optarg);
				break;
			case 'x':
				x0 = atof(optarg);
				break;
            case 'i':
				max_iter = atoi(optarg);
				break;
			default:
				printf("Usage: ./find_roots <coefficients> <options>\n");
                exit(0);
		}
	}

    print_poly(p);

    newton_raphson(p, x0, max_iter, verbose);
    tangent(p, x0, d, max_iter, verbose);

    free_poly(p);
    return 0;
}

void newton_raphson(Polynomial *p, double x0, int max_iter, int verbose) {

    Polynomial *d = differentiate(p);

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
        printf("Newton-Raphson did not manage to find a root after %d iterations. Final value of x: %f, where f(x)=%f\n", iter, x, eval_at(p, x));
    }
}

void tangent(Polynomial *p, double x0, double delta, int max_iter, int verbose) {

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
        x = x - (eval_at(p,x) / ((eval_at(p, x+delta) - eval_at(p,x)) / delta));
    }

    // print stats
    if (found) {
        printf("The tangent method converged to %f after %d iterations\n", x, iter+1);
    } else {
        printf("The tangent method did not manage to find a root after %d iterations. Final value of x: %f, where f(x)=%f\n", iter, x, eval_at(p, x));
    }
}
