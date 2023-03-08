#include "polyonym.h"
#include <stdlib.h>

Polyonym *init_poly(int deg, int *coeffs) {

    Polyonym *np = (Polyonym*) malloc(sizeof(Polyonym));
    np->degree = deg;
    np->coeffs = malloc(sizeof(int) * (deg+1));

    if (coeffs != NULL) {
        for (int i=0; i<deg+1; i++) {
            np->coeffs[i] = coeffs[i];
        }
    }

    return np;
}

void free_poly(Polyonym *p) {
    free(p->coeffs);
    free(p);
}

void print_poly(Polyonym *p) {
    
    printf("The polyonym (of degree %d) is:", p->degree);
    for(int i=0; i<p->degree+1; i++) {
        printf(" %dx^%d %c", p->coeffs[i], p->degree-i, i!=p->degree?p->coeffs[i]<0?'-':'+':'\n');
    }

}

float evaluate_poly_at_x(Polyonym *p, int x) {

    float val = 0;

    for(int i=0; i<p->degree+1; i++) {
        val += p->coeffs[i]*(float)pow(x, p->degree-i);
    }

    return val;
}

Polyonym *differentiate(Polyonym *p) {

    Polyonym *der = init_poly(p->degree-1, NULL);

    // calculate the derivative coefficients
    for(int i=0; i<der->degree+1; i++) {
        der->coeffs[i] = p->coeffs[i]*(p->degree-i);
    }

    return der;

}
