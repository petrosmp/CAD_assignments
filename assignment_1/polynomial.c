#include "polynomial.h"
#include <stdlib.h>

Polynomial *init_poly(int deg, int *coeffs) {

    Polynomial *np = (Polynomial*) malloc(sizeof(Polynomial));
    np->degree = deg;
    np->coeffs = malloc(sizeof(int) * (deg+1));

    if (coeffs != NULL) {
        for (int i=0; i<deg+1; i++) {
            np->coeffs[i] = coeffs[i];
        }
    }

    return np;
}

void free_poly(Polynomial *p) {
    free(p->coeffs);
    free(p);
}

void print_poly(Polynomial *p) {
    
    printf("The polynomial (of degree %d) is:", p->degree);
    for(int i=0; i<p->degree+1; i++) {
        printf(" %dx^%d %c", abs(p->coeffs[i]), p->degree-i, i!=p->degree?p->coeffs[i+1]<0?'-':'+':'\n');
    }

}

double eval_at(Polynomial *p, double x) {

    double val = 0;

    for(int i=0; i<p->degree+1; i++) {
        val += p->coeffs[i]*pow(x, p->degree-i);
    }

    return val;
}

Polynomial *differentiate(Polynomial *p) {

    Polynomial *der = init_poly(p->degree-1, NULL);

    // calculate the derivative coefficients
    for(int i=0; i<der->degree+1; i++) {
        der->coeffs[i] = p->coeffs[i]*(p->degree-i);
    }

    return der;

}
