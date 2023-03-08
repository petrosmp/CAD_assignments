#include "polyonym.h"

void print_poly(Polyonym p) {
    
    printf("The polyonym is:");
    for(int i=0; i<p.degree+1; i++) {
        printf(" %dx^%d %c", p.coeffs[i], p.degree+1-i-1, i!=p.degree+1-1?p.coeffs[i]<0?'-':'+':'\n');
    }

}

float evaluate_poly_at_x(Polyonym p, int x) {

    float val = 0;

    for(int i=0; i<p.degree+1; i++) {
        val += p.coeffs[i]*(float)pow(x, p.degree-i);
    }

    return val;
}
