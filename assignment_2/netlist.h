/**
 * Custom set of structures and functions that make working with
 * component libraries (containing gates or subsystems) cleaner
 * and allow for code reuse and easy modification.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "str_util.h"

#define COMP_DESIGNATION "COMP "    /* The word that signifies that a line contains a component */
#define INPUT_DESIGNATION "IN: "    /* The word that signifies that the next stuff is the inputs of the component. */
#define OUTPUT_DESIGNATION "OUT: "  /* The word that signifies that the next stuff is the outputs of the component. */
#define IN_OUT_DELIM ", "           /* The delimeter that separates inputs/outputs from each other. i.e. for outputs A, B and C and INOUT_DELIM ",", the outputs will be: A,B,C */
#define DEFAULT_DELIM " ; "         /* The delimiter that is used by default to separate fields. */

/**
 * A gate has a variable number of inputs but only one output. It is
 * defined in a component library and can be used to create other gates
 * or subsystems.
*/
typedef struct {
    char *name;     /* The name of the gate (ASCII, human readable). */
    int _inputc;    /* The number of inputs the gate has (mainly for internal use). */
    char** inputs;  /* The names of the inputs of the gate. */
    char* source;   /* The name of the file that the gate was defined in. */
} Gate;

/**
 * A subsystem is a circuit with both inputs and outputs, comprised by
 * gates or other subsystems, defined in a subsystem library.
*/
typedef struct {
    char* name;     /* The name of this subsystem (ASCII, human readable). */
    int _inputc;    /* The number of inputs the subsystem has (mainly for internal use). */
    char** inputs;  /* The names of the inputs of the subsystem. */
    int _outputc;   /* The number of outputs the subsystem has (mainly for internal use). */
    char** outputs; /* The names of the outputs of the subsystem. */
    char* source;   /* The name of the file that the subsystem was defined in. */
} Subsystem;

/**
 * Free the memory that was allocated for subsystem s and all its
 * members.
*/
void free_subsystem(Subsystem *s);

/**
 * Store a human readable string representation of the given
 * subsystem in the given char*. Write no more than n bytes,
 * null terminator ('\0') included.
 * 
 * Returns:
 *  - 0 on success
 *  - NES on failure because of not enough space
 *  - NARG on failure because of null arguments.
 * 
 * strncpy could easily be used to do each part of this, but
 * it does not have an error return value so even though we
 * would be sure that no buffer overflows have occured, we
 * would not know whether the copying was completed properly
 * or not.
*/
int subsys_to_str(Subsystem* s, char* str, int n);

/**
 * Given a string of length n (if it is longer, only n bytes will be taken
 * into account) that declares a subsystem, set s to describe that subsystem
 * too. Does not set s->source. Allocates memory for every input and output
 * of s, and also for the lists, so using this on an already initialized
 * subsystem might cause memory leaks.
 * 
 * Returns:
 *  - 0 on success
 *  - NES on failure because of not enough space
 *  - NARG on failure because of null arguments.
 * 
 * Remember to free str.
*/
int str_to_subsys(char *str, Subsystem *s, int n);

/**
 * Parse the given file and insert the information it contains into the
 * given subsystem.
 * 
 * For the time being the file is assumed to only have 1 subsystem defined
 * in it (if there are more, there may be memory leaks or undefined behavior).
*/
int read_subsystem_from_file(char *filename, Subsystem **s);
