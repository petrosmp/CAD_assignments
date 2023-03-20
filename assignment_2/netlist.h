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
#define COMP_ID_PREFIX "U"          /* The prefix of every component id. When printing the a component c, COMP_ID_PREFIX<c.id> will be printed */
#define COMP_DELIM  " "             /* The delimiter separating the attributes of a component */

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
 * A component is an instance of a subsystem as part of a netlist.
 * 
 * For example, the following part of a netlist describes a system
 * with 2 components (full adders). Each one has an id, is assosiated
 * with a subsystem and has some signals as inputs.
 * 
 * COMP FULL_ADDER2 ; IN: A0,A1,B0,B1,Cin ; OUT: S0, S1, Cout
 * BEGIN FULL_ADDER2 NETLIST 
 * U1 FULL_ADDER A0, B0, Cin
 * U2 FULL_ADDER A1, B1, U1_Cout
 * ...
 * END FULL_ADDER2 NETLIST
*/
typedef struct {

    int id;                 /* the unique ID of the component */
    Subsystem *standard;    /* the subsystem that the component is an instance of */
    int _inputc;            /* the number of inputs the component has*/
    char **inputs;          /* the names of the input signals of the component */
} Component;

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
 * For the time being the file is assumed to have exactly 1 subsystem defined
 * in it (if there are more, there may be memory leaks or undefined behavior).
 * 
 * Also sets the subsystem's 'source' to the given filename.
*/
int read_subsystem_from_file(char *filename, Subsystem **s);

/**
 * Properly free up the memory allocated for component c and its
 * members.
 * 
 * DOES NOT FREE the subsystem pointed to by c.standard.
*/
void free_component(Component *c);

/**
 * Store a human readable string representation of the given
 * component in the given char*. Write no more than n bytes,
 * null terminator ('\0') included.
 * 
 * Returns:
 *  - 0 on success
 *  - NES on failure because of not enough space
 *  - NARG on failure because of null arguments.
*/
int comp_to_str(Component *c, char *str, int n);
