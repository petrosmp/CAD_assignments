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
 * Allocate memory for a polyonym with the given degree and coefficients.
 * If the coefficients argument is NULL, memory is reserved for them but
 * they are not initialized, so their values are undefined. Make sure to
 * define them before using them when doing that.
 * 
 * Remember to free!
*/
Polyonym *init_poly(int deg, int *coeffs);

/**
 * Properly free up the memory that was allocated for the given polyonym.
 * 
 * This includes freeing the memory in which the coefficients were stored.
*/
void free_poly(Polyonym *p);

/**
 * Print the polyonym that the given coefficients represent.
 * 
 * Leaves the cursor on the next line
*/
void print_poly(Polyonym *p);


/**
 * Evaluate the given polyonym at the given point.
*/
double eval_at(Polyonym *p, double x);

/**
 * Calculate the first derivative of the given polyonym and return
 * it as a new polyonym.
 * 
 * This is done according to the differentiation rules, in an
 * "analytic" way.
 * 
 * This also allocates memory for a new Polyonym, so remember to free
 * that too.
*/
Polyonym *differentiate(Polyonym *p);

