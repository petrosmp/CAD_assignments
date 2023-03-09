/**
 * Custom set of structs (and functions acting on them) that make
 * handling polynomials cleaner, more concise and more readable.
*/
#include <stdio.h>
#include <math.h>

/**
 * A polynomial, described by its degree and its coefficients.
*/
typedef struct {
    
    /**
     * The degree of the polynomial (the highest power to which x
     * is raised to in it). Keep in mind that the coefficients are
     * one more than the degree of the polynomial.
    */
    int degree;

    /**
     * The coefficients of the polynomial. Keep in mind that the size
     * of this array will be one more than the degree of the polynomial.
     * 
     * All coefficients are assumed to be integers.
    */
    int *coeffs;
} Polynomial;

/**
 * Allocate memory for a polynomial with the given degree and coefficients.
 * If the coefficients argument is NULL, memory is reserved for them but
 * they are not initialized, so their values are undefined. Make sure to
 * define them before using them when doing that.
 * 
 * Remember to free!
*/
Polynomial *init_poly(int deg, int *coeffs);

/**
 * Properly free up the memory that was allocated for the given polynomial.
 * 
 * This includes freeing the memory in which the coefficients were stored.
*/
void free_poly(Polynomial *p);

/**
 * Print the polynomial that the given coefficients represent.
 * 
 * Leaves the cursor on the next line
*/
void print_poly(Polynomial *p);

/**
 * Evaluate the given polynomial at the given point.
*/
double eval_at(Polynomial *p, double x);

/**
 * Calculate the first derivative of the given polynomial and return
 * it as a new polynomial.
 * 
 * This is done according to the differentiation rules, in an
 * "analytic" way.
 * 
 * This also allocates memory for a new Polynomial, so remember to free
 * that too.
*/
Polynomial *differentiate(Polynomial *p);
