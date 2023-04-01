/**
 * Custom set of structures and functions that make working with
 * component libraries (only containing subsystems) cleaner
 * and allow for code reuse and easy modification.
*/

#include "str_util.h"

#define DECL_DESIGNATION "COMP "    /* The word that signifies that a line declares a subsystem */
#define INPUT_DESIGNATION "IN: "    /* The word that signifies that the next part of a string is the inputs of the subsystem. */
#define OUTPUT_DESIGNATION "OUT: "  /* The word that signifies that the next part of a string is the outputs of the subsystem. */
#define IN_OUT_DELIM ", "           /* The delimeter that separates inputs/outputs from each other. i.e. for outputs A, B and C and INOUT_DELIM "," the output list will be: A,B,C */
#define GENERAL_DELIM " ; "         /* The delimiter that separates fields. */
#define COMP_ID_PREFIX "U"          /* The prefix of every component id. When printing the a component c, COMP_ID_PREFIX<c.id> will be printed */
#define COMP_DELIM  " "             /* The delimiter separating the attributes of a component */
#define MAX_LINE_LEN 512            /* The maximum allowed length of a line in a netlist file */
#define COMMENT_PREFIX "%%"         /* The prefix of any comment line */
#define KEYWORD_PREFIX "**"         /* The prefix of any keyword line */

/**
 * The possible types of subsystems.
 * 
 * The subsystem struct is used to represent both functional circuits and reference
 * subsystems. This is a way to designate what kind of subsystem each instance of
 * the struct is, so it can be handled accordingly.
 * 
 * See the declaration of subsystem for an explanation of the types.
*/
enum SUBSYSTEM_TYPE {
	REFERENCE,  /* Only contains information regarding inputs, outputs, name and source file */
	FUNCTIONAL, /* A functional subsystem representation, with its components and their mappings */
};

/**
 * Since a single node structure is used for all linked list needs of the
 * program, we need a way to have a way to tell what each node contains.
*/
enum NODE_TYPE {
    STANDARD,   /* The node contains a standard subsystem (as read from a library) */
    SUBSYSTEM_N,  /* The node contains a functional subsystem (as described in a netlist) */
    COMPONENT   /* The node contains a component (a part of any type of subsystem) */
};

/**
 * Since only one structure is used to represent items read from a library,
 * we need a way to tell what each item is.
 * 
 * TODO: The same enumeration is used to mark the types of libraries, so a 
 * change of name may be in order.
*/
enum STANDARD_TYPE {
    GATE,       /* The standard describes a gate */
    SUBSYSTEM   /* The standard describes a subsystem */
};

/**
 * Nodes are the building blocks of the linked lists that contain
 * the standards defined in libraries.
 * 
 * They contain a standard an a pointer to the next node in the list.
*/
typedef struct node {
    enum NODE_TYPE type;    /* The type of data the node contains */
    union {
        struct standard *std;      /* The standard the node contains */
        struct subsystem *subsys;  /* The subsystem the node contains */
        struct component *comp;    /* The component the node contains */
    };
    struct node *next;             /* The next node in the list */
} Node;

/**
 * A library is a collection of standards, usually read from a file.
 * It can either contain subsystems or gates (technically it can contain
 * both, but we use separate libraries for each).
 * 
 * It contains a linked list with the standards it contains, as well as
 * the name of the file in which they were defined.
*/
typedef struct lib {
    enum STANDARD_TYPE type;    /* The possible types are the same  */
    Node *contents;             /* The contents of the library */
    Node *_tail;                /* The last node of the list, for O(1) insertions */
    char *file;                 /* The file the library was defined in */
} Library;


/**
 * A gate is assumed to be the basic building block of everything. It only
 * has one outputs but can have an arbitrary number of inputs.
*/
typedef struct gate {
    char* name;                     /* The name of this gate (ASCII, human readable). */
    int _inputc;                    /* The number of inputs the gate has (mainly for internal use). */
    char** inputs;                  /* The names of the inputs of the gate. */
} Gate;

/**
 * A subsystem is a circuit with both inputs and outputs, comprised by
 * gates or other subsystems, defined in a subsystem library.
 * 
 * A subsystem can be of one of the following types:
 *      - A "functional subsystem": contains pointers to all its components, as well as information
 *        regarding its output mappings.
 *      - A "reference subsystem": does not contain either of the above, and serves as a prototype
 *        when creating an instance of a subsystem the implementation of which is already known
 *        (e.g. it's implemented in a library). It only contains information regarding the number
 *        and names of the inputs and the name of the circuit.
 * 
 * TODO: Would it be better design if those 2 were different structs? It would save ~32 bytes per
 *       reference subsystem and avoid cyclic dependencies
*/
typedef struct subsystem {
    char* name;                     /* The name of this subsystem (ASCII, human readable). */
    int _inputc;                    /* The number of inputs the subsystem has (mainly for internal use). */
    char** inputs;                  /* The names of the inputs of the subsystem. */
    int _outputc;                   /* The number of outputs the subsystem has (mainly for internal use). */
    char** outputs;                 /* The names of the outputs of the subsystem. */
    char* source;                   /* The name of the file that the subsystem was defined in. */
    enum SUBSYSTEM_TYPE type;       /* Whether this is a functional subsystem or a reference one */
    int _componentc;                /* The number of components the subsystem is made of (if it is a functional one) */
    struct component **components;  /* The list of the subsystem components (if it is a functional one) */
    char **output_mappings;         /* The list of the mappings of internal signals to the subsystem's outputs (if it is a functional one) */
} Subsystem;

/**
 * A standard is a subsystem or gate read from a library file and
 * used inside components to indicate the "type" of each component.
 * 
 * For example, if "c" is a component we know nothing about, we can
 * find information regarding its name, inputs and outputs from its
 * standard, which must have been read from a library.
*/
typedef struct standard {

    enum STANDARD_TYPE type;/* The type of circuit this standard defines (gate or subsystem) */
    union {
        Subsystem* subsys;  /* The subsystem this standard defines */
        Gate* gate;         /* The gate this standard defines */
    };
    Library *defined_in;    /* The library in which this standard is defined */

} Standard;

/**
 * A component is an instance of a subsystem as part of a circuit.
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
typedef struct component {
    int id;                 /* The unique ID of the component */
    Subsystem *standard;    /* The subsystem that the component is an instance of */
    int _inputc;            /* The number of inputs (more precisely, input mappings) the component has */
    char **inputs;          /* The names of the input signals of the component */
} Component;

/**
 * Reads up to n bytes from str (which is assumed to contain the data
 * necessary to define a gate) and parses the information into the fields
 * of g.
 * 
 * Does not allocate memory for the gate itself but does for every input
 * and the input list itself.
 * 
 * Returns:
 *  - 0 on success
 *  - NES on failure because of not enough space
 *  - NARG on failure because of null arguments.
*/
int str_to_gate(char *str, Gate *g, int n);

/**
 * Writes an ASCII representation of the given gate to str, writing no more
 * than n bytes, null terminator ('\0') included.
 * 
 * Returns:
 *  - 0 on success
 *  - NES on failure because of not enough space
 *  - NARG on failure because of null arguments.
*/
int gate_to_str(Gate *g, char *str, int n);

/**
 * Properly free up the memory allocated for and used by the given gate. 
*/
void free_gate(Gate *g);

/**
 * Properly free up the memory allocated for and used by a standard.
*/
void free_standard(Standard *s);

/**
 * Properly free up the memory allocated for and used by a node.
*/
void free_node(Node *n);

/**
 * Add the given standard to (the end of) the given library.
 * 
 * Allocates memory for the new node that the standard will
 * use to connect to the library.
 * 
 * The standard is assumed to already be initialized. The data
 * it contains are not modified.
 * 
 * Returns:
 *  - 0 on success
 *  - NARG on failure because of null arguments.
*/
int add_to_lib(Library *lib, Standard* s);

/**
 * Parse the contents of a file into standards and store them in the given
 * library.
 * 
 * Does not allocate memory for the library, this has to be done by the caller.
 * The memory is assumed to be newly allocated, so if called on an already
 * initialized library, memory leaks are around the corner.
 * 
 * Reads the file line by line, and ignores any line starting with "%%" or
 * "**".
 * 
 * Returns:
 *  - 0 on success
 *  - NES on failure because of not enough space
 *  - NARG on failure because of null arguments.
*/
int gate_lib_from_file(char *filename, Library* lib);

/**
 * Properly free up the memorey allocated for and used by a library and
 * its members.
*/
void free_lib(Library *lib);

/**
 * Free the memory that was allocated for subsystem s and all its
 * members.
 * 
 * Works on both types of subsystems (reference and functional, see
 * the definition of the subsystem struct for more).
*/
void free_subsystem(Subsystem *s);

/**
 * Store a human readable string representation of the reference (*)
 * information of the given subsystem in the given char*. Write
 * no more than n bytes, null terminator ('\0') included.
 * 
 * *see the definition of the subsystem struct for more
 * 
 * Returns:
 *  - 0 on success
 *  - NES on failure because of not enough space
 *  - NARG on failure because of null arguments.
*/
int subsys_to_ref_str(Subsystem* s, char* str, int n);

/**
 * Given a string of length n (if it is longer, only n bytes will be taken
 * into account) that declares a subsystem reference (*), set s to describe
 * that subsystem too.
 * 
 * The string is assumed to be of the following format (no newlines):
 *  <COMP_DESGNATION><GENERAL_DELIM><name><GENERAL_DELIM><INPUT_DESIGNATION>
 *  <input list, separated by INOUT_DELIM><GENERAL_DELIM><OUTPUT_DESIGNATION>
 *  <output list, separated by INOUT_DELIM>
 * 
 * Does not allocate memory for the subsystem itself, but does for every input
 * and output (and also for the lists), so using this on an already initialized
 * subsystem might cause memory leaks, as pointers to the previously allocated
 * memory will be lost.
 * 
 * Does not set s->source.
 * 
 * Returns:
 *  - 0 on success
 *  - NES on failure because of not enough space
 *  - NARG on failure because of null arguments.
 * 
 * *see the definition of the subsystem struct for more
*/
int str_to_subsys_ref(char *str, Subsystem *s, int n);

/**
 * Parse the given file and insert the information it contains into the
 * given reference (*) subsystem.
 * 
 * Does not allocate memory for the new subsystem, but it does for its
 * members whenever needed.
 * 
 * Also sets the subsystem's 'source' to the given filename and its type
 * to REFERENCE.
 * 
 * Returns:
 *  - 0 on success
 *  - NES on failure because of not enough space
 *  - NARG on failure because of null arguments.
 * 
 * *see the definition of subsystem struct for more
 * 
 * NOTE:
 * For the time being the file is assumed to have exactly 1 subsystem defined
 * in it (if there are more, there may be memory leaks or undefined behavior).
 * 
*/
int read_ref_subsystem_from_file(char *filename, Subsystem **s);

/**
 * Properly free up the memory allocated for component c and its
 * members.
 * 
 * DOES NOT FREE the subsystem pointed to by c->standard.
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

/**
 * Write the netlist for the given (functional) subsystem to the given file,
 * with all its components and output mappings.
 * 
 * The mode argument can be used to control the mode in which the file is opened
 * (it is passed directly to fopen). If mode = append (starts with "a"), a newline
 * is written to the file before the netlist to separate it from the previous one(s).
*/
int netlist_to_file(Subsystem *s, char *filename, char *mode);
