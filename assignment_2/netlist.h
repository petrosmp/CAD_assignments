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
#define MAX_INPUTS 10               /* The maximum number of inputs a subsystem can have. */
#define MAX_OUTPUTS 10              /* The maximum number of outputs a subsystem can have. */
#define MAX_SIGNAL_LENGTH 30        /* The maximum number of letters in a signal name. */


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

// to parse from file keep pointer to first input character and firs non input character for each input.
// then once all inputs are found, allocate memory for the inputs array and go around allocating memory
// for each individual input directly in the array. or you could just like allocate memory for each input
// separately, just as much as is needed (a buffer of size MAX_SIGNAL_SIZE should be enough to keep it until
// it ends and needs dynamic allocation, and another buffer of size MAX_INPUTS should be enough to keep all
// the pointers in it until the inputs array is allocated, nice.) Βεβαια ετσι θεωρητικα κανεις αλλου malloc
// και αλλου free αλλα σιγα που μας πειραζει αυτο. valgrind ΟΠΩΣΔΗΠΟΤΕ. αν ολα γινονται ετσι, ισως να μην
// χρειαζεται καν init_subsystem τελικα.

Subsystem *init_subsystem() {
    Subsystem *s = (Subsystem*) malloc(sizeof(Subsystem));
    s->inputs = (char**) malloc(sizeof(char*) * MAX_INPUTS);
    s->outputs = (char**) malloc(sizeof(char*) * MAX_OUTPUTS);

}

void free_subsystem(Subsystem *s) {

    // free inputs
    for(int i=0; i<s->_inputc; i++) {
        free(s->inputs[i]);
    }
    free(s->inputs);

    // free outputs
    for(int i=0; i<s->_outputc; i++) {
        free(s->outputs[i]);
    }
    free(s->outputs);

    // free name?

    // free source?

    // free s
    free(s);
}

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
int subsys_to_str(Subsystem* s, char* str, int n) {

    if (s==NULL || str==NULL) {
        return NARG;
    }

    int offset = 0;
    int _en;

    // write the first word, indicating that this is a component
    if(_en = write_at(str, COMP_DESIGNATION, offset, strlen(COMP_DESIGNATION))) return _en;
    offset += strlen(COMP_DESIGNATION);

    // write the name of the component
    if(_en = write_at(str, s->name, offset, strlen(s->name))) return _en;
    offset += strlen(s->name);

    // write the default delimiter
    if(_en = write_at(str, DEFAULT_DELIM, offset, strlen(DEFAULT_DELIM))) return _en;
    offset += strlen(DEFAULT_DELIM);

    // write the input designation
    if(_en = write_at(str, INPUT_DESIGNATION, offset, strlen(INPUT_DESIGNATION))) return _en;
    offset += strlen(INPUT_DESIGNATION);

    // write the inputs, separated by delimiter
    int list_bytes = 0;
    _en = write_list_at(s->_inputc, s->inputs, IN_OUT_DELIM, str, offset, n-1-offset, &list_bytes);
    if (_en) return _en;
    offset += list_bytes;

    // write the default delimiter
    if(_en = write_at(str, DEFAULT_DELIM, offset, strlen(DEFAULT_DELIM))) return _en;
    offset += strlen(DEFAULT_DELIM);

    // write the output designation
    if(_en = write_at(str, OUTPUT_DESIGNATION, offset, strlen(OUTPUT_DESIGNATION))) return _en;
    offset += strlen(OUTPUT_DESIGNATION);

    // write the outputs, separated by delimiter
    list_bytes = 0;
    _en = write_list_at(s->_outputc, s->outputs, IN_OUT_DELIM, str, offset, n-1-offset, &list_bytes);
    if (_en) return _en;
    offset += list_bytes;

    // manually null terminate the string
    str[offset] = '\0';

}

/**
 * Given a string of length n (if it is longer, only n bytes will be taken
 * into account) that declares a subsystem, set s to describe that subsystem
 * too. Does not set s->source. Allocates memory for every input and output
 * of s, and also for the lists.
 * 
 * Returns:
 *  - 0 on success
 *  - NES on failure because of not enough space
 *  - NARG on failure because of null arguments.
 * 
 * Remember to free str.
*/
int str_to_subsys(char *str, Subsystem *s, int n) {

    // any line declaring a subsystem starts with COMP_DESIGNATION which we ignore
    int offset = strlen(COMP_DESIGNATION);

    /* because split alters its first argument, and we need to keep
       the original address of str so we can free it, we use an internal
       variable to pass to split
    */
    char *_str = str+offset;

    // strtok interprets delim as a collection of delimiters, so if even one of them is
    // found, it splits. we need to split if ALL OF THEM are found CONSECUTIVELY.
    // so we create what we need (see split()).

    // get the fields of the line
    char *name        = split(&_str, DEFAULT_DELIM);
    char *raw_inputs  = split(&_str, DEFAULT_DELIM);
    char *raw_outputs = split(&_str, DEFAULT_DELIM);

    // get the input and output lists (get rid of the designations)
    char *_inputs = raw_inputs+strlen(INPUT_DESIGNATION);
    char *_outputs = raw_outputs+strlen(OUTPUT_DESIGNATION);

    // parse the input and output lists
    s->inputs = NULL;   // initialize to NULL so initial call to realloc is like malloc
    s->outputs = NULL;
    int _inputc = 0;
    int _outputc = 0;
    char *cur;

    while(_inputs != NULL) {

        // get the name of the input
        cur = split(&_inputs, IN_OUT_DELIM);
        
        // allocate memory for the entry in the input table
        s->inputs = realloc(s->inputs, sizeof(char*)*(_inputc+1));

        // allocate memory for the input's name (plus a null byte)
        s->inputs[_inputc] = malloc(strlen(cur)+1);

        // copy the name of the input into the table
        strncpy(s->inputs[_inputc], cur, strlen(cur)+1);  // strcpy will insert null bytes in empty space
        
        _inputc++;
    }

    s->_inputc = _inputc;

    while(_outputs != NULL) {

        // get the name of the output
        cur = split(&_outputs, IN_OUT_DELIM);
        
        // allocate memory for the entry in the output table
        s->outputs = realloc(s->outputs, sizeof(char*)*(_outputc+1));

        // allocate memory for the output's name (plus a null byte)
        s->outputs[_outputc] = malloc(strlen(cur)+1);

        // copy the name of the output into the table
        strncpy(s->outputs[_outputc], cur, strlen(cur)+1);  // strcpy will insert null bytes in empty space
        
        _outputc++;
    }

    s->_outputc = _outputc;


    // printfs for testing

    printf("The name is: [%s]\n", name);
    printf("Read %d inputs:", s->_inputc);
    for(int i=0; i<s->_inputc; i++) {
        printf(" [%s]", s->inputs[i]);
    }
    printf("\n");
    printf("Read %d outputs:", s->_outputc);
    for(int i=0; i<s->_outputc; i++) {
        printf(" [%s]", s->outputs[i]);
    }
    printf("\n");


    return 0;
}
