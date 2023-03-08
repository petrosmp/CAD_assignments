/**
 * Custom set of structs (and functions acting on them) that make
 * handling polyonyms cleaner, more concise and more readable.
*/
#include <stdio.h>
#include <math.h>

/**
 * A polyonym, described by its degree and its coefficients.
*/
typedef struct {
    
    /**
     * The degree of the polyonym (the highest power to which x
     * is raised to in it). Keep in mind that the coefficients are
     * one more than the degree of the polyonym.
    */
    int degree;

    /**
     * The coefficients of the polyonym. Keep in mind that the size
     * of this array will be one more than the degree of the polyonym.
     * 
     * All coefficients are assumed to be integer.
    */
    int *coeffs;
} Polyonym;

/**
 * Print the polyonym that the given coefficients represent.
 * 
 * Leaves the cursor on the next line
*/
void print_poly(Polyonym p);


/**
 * Evaluate the given polyonym at the given point.
*/
float evaluate_poly_at_x(Polyonym p, int x);
