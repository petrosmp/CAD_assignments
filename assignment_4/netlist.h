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
#define NETLIST_START "BEGIN "      /* The word that signifies that a netlist is contained in the following lines */
#define NETLIST_END "END "          /* The word that signifies that a netlist ends in this line */
#define UNEXPECTED_EOF -2           /* Error code returned when a file ends unexpectedly, leaving a netlist incomplete */
#define SYNTAX_ERROR -3             /* Error code returned when a syntax error (of any kind) is detected while parsing a netlist */
#define UNKNOWN_COMP -4             /* Error code returned when a netlist contains a component that we have not seen in any library */
#define OUTPUT_MAP_DELIM " = "      /* The delimiter between a subsystem output name and its mapping in a netlist */

/**
 * Since a single node structure is used for all linked list needs of the
 * program, we need a way to have a way to tell what each node contains.
*/
enum NODE_TYPE {
    STANDARD,       /* The node contains a standard subsystem (as read from a library) */
    SUBSYSTEM_N,    /* The node contains a functional subsystem (as described in a netlist) */
    COMPONENT       /* The node contains a component (any part of a subsystem) */
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
 * Each input/output in a standard subsystem's component is mapped to either
 * an input of the subsystem itself or to another gate (or subsystem,
 * but ultimately a gate).
*/
enum MAPPING_TYPE {
    SUBSYS_INPUT,   /* The input/output is coming from the subsystem's input  list */
    SUBSYS_COMP     /* The input/output is another component (gate) in the subsystem */
};

/**
 * A component contained in a standard subsystem has a dynamic way of
 * mapping its inputs/outputs to the subsystem, so that with any given
 * set of inputs to the subsystem, the component's inputs/outputs can
 * be set accordingly.
 * 
 * Let c be a component contained in a subsystem s. Then c can have in
 * each of its inputs either an input of s or an output of another
 * component of s.
 * 
 * The same is true for any output mapping of s.
*/
typedef struct mapping {
    enum MAPPING_TYPE type; /* The type of the mapping */
    int index;              /* The index (either of the subsystems input or of the component) */
    int out_index;          /* The index of the output in case this refers to a component */
} Mapping;

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
typedef struct netlist {
    enum STANDARD_TYPE type;    /* The possible types are the same  */
    Node *contents;             /* The contents of the library */
    Node *_tail;                /* The last node of the list, for O(1) insertions */
    char *file;                 /* The file the library was defined in */
} Netlist;

/**
 * A gate is assumed to be the basic building block of everything. It only
 * has one outputs but can have an arbitrary number of inputs.
 * 
 * Gates are defined in a component library and can be used as components
 * of subsystems.
*/
typedef struct gate {
    char* name;                     /* The name of this gate (ASCII, human readable). */
    int _inputc;                    /* The number of inputs the gate has (mainly for internal use). */
    char** inputs;                  /* The names of the inputs of the gate. */
} Gate;

/**
 * A subsystem is a circuit with both inputs and outputs, comprised by
 * gates or other subsystems.
 * 
 * Subsystems are initially defined in a subsystem library and once defined
 * they can be used as components in other subsystems.
*/
typedef struct subsystem {
    char* name;                 /* The name of this subsystem (ASCII, human readable). */
    int _inputc;                /* The number of inputs the subsystem has (mainly for internal use). */
    char** inputs;              /* The names of the inputs of the subsystem. */
    int _outputc;               /* The number of outputs the subsystem has (mainly for internal use). */
    char** outputs;             /* The names of the outputs of the subsystem. */
    struct node *components;    /* The list of the subsystem components (if it is a functional one) */
    struct node *_tail;         /* Pointer to the last component for O(1) insertion */
    char **output_mappings;     /* The list of the mappings of internal signals to the subsystem's outputs (if it is a functional one) */
    int is_standard;            /* Boolean flag indicating whether the subsystem is a standard one */
    Mapping **o_maps;           /* If the subsystem is a standard one, along the outputs there will be output mappings */
} Subsystem;

/**
 * A standard is a subsystem or gate read from a library file and
 * used inside components to indicate the type and basic properties
 * of each component.
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
    Netlist *defined_in;    /* The library in which this standard is defined */

} Standard;

/**
 * A component is an instance of a subsystem as part of a circuit.
 * 
 * Components defined inside standard subsystems (read from libraries)
 * will also have a series of input mappings to represent the way that
 * the inputs of the component are mapped inside the subsystem (to other
 * components or to the subsystem's inputs).
 * 
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
    Standard *prototype;    /* The subsystem that the component is an instance of */
    int is_standard;        /* Boolean flag indicating whether the component is contained in a standard subsystem */
    int _inputc;            /* The number of inputs (more precisely, input mappings) the component has */
    char **inputs;          /* The names of the input signals of the component */
    Mapping **i_maps;       /* If the component is part of a standard subsystem, along the inputs there will be input mappings */
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
 * 
 * complete is a flag indicating whether the contents of the node should
 * be deleted too.
*/
void free_node(Node *n, int complete);

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
int add_to_lib(Netlist *lib, void* s, int is_standard, enum STANDARD_TYPE type);

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
int gate_lib_from_file(char *filename, Netlist* lib);

/**
 * Properly free up the memorey allocated for and used by a library and
 * its members.
*/
void free_lib(Netlist *lib);

/**
 * Free the memory that was allocated for subsystem s and all its
 * members.
 * 
 * free_comp is a flag that indicates whether the individual components
 * of the subsystem should be freed.
*/
void free_subsystem(Subsystem *s, int free_comp);

/**
 * Store a human readable string representation of the header (*)
 * information of the given subsystem in the given char*. Write
 * no more than n bytes, null terminator ('\0') included.
 * 
 * * name, input and output names
 * 
 * Returns:
 *  - 0 on success
 *  - NES on failure because of not enough space
 *  - NARG on failure because of null arguments.
*/
int subsys_hdr_to_str(Subsystem* s, char* str, int n);

/**
 * Given a string of length n (if it is longer, only n bytes will be taken
 * into account) that declares a subsystem header (*), set s to describe
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
 * * name, input and output names
*/
int str_to_subsys_hdr(char *str, Subsystem *s, int n);

/**
 * Add the given component to the component list of the given subsystem.
*/
int subsys_add_comp(Subsystem *s, Component *c);

/**
 * Parse the contents of the given file into standards and store them in the given
 * library. All components used in the given file has to be defined in the given
 * lookup_library.
 * 
 * TODO: the lookup should be a list of libraries, possibly containing
 * the one currently being parsed
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
int subsys_lib_from_file(char *filename, Netlist *lib, Netlist *lookup_lib);

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
 * Search (in a serial fashion) the given library for a standard
 * with the given name.
 * 
 * Case sensitive.
 * 
 * Returns a pointer to the standard if found, NULL otherwise.
*/
Standard* search_in_lib(Netlist *lib, char *name);

/**
 * Given a string of length n (if it is longer, only the first n
 * bytes will be taken into account), parse its contents into the
 * given component. Will look for a standard of the component in
 * the given lib.
 * 
 * The subsystem s is the subsystem that contains the component
 * that is contained in str. This is needed so that in case the
 * subsystem is a standard one (see is_standard), the function is
 * able to determine whether the inputs are subsystem inputs or
 * other subsystem components. 
 * 
 * is_standard is a flag that indicated whether the component that
 * will be created is read from a library and should thus include
 * input mappings or not.
 * 
 * Returns:
 *  - 0 on success
 *  - NARG on failure because of null arguments
 *  - UNKNOWN_COMP on failure because of unseen component type
*/
int str_to_comp(char *str, Component *c, int n, Netlist *lib, Subsystem *s, int is_standard);

/**
 * Write the netlist for the given (functional) subsystem to the given file,
 * with all its components and output mappings.
 * 
 * The mode argument can be used to control the mode in which the file is opened
 * (it is passed directly to fopen). If mode = append (starts with "a"), a newline
 * is written to the file before the netlist to separate it from the previous one(s).
*/
int netlist_to_file(Subsystem *s, char *filename, char *mode);

/**
 * Move x positions (forward) in the given list. Returns the node
 * that is there if the move is valid, NULL otherwise
*/
Node *move_in_list(int x, Node *list);

/**
 * Given a standard for a subsystem read from a library, create an instance
 * of it with the given inputs. The components in it and the outputs will be
 * mapped according to the standard.
 * 
 * Returns the comp_id number that the next component should have.
 * A negative value means some sort of error.
 * 
 * ns is assumed to be already allocated but nothing more.
 * Must be freed by the caller.
*/
int create_custom(Subsystem *ns, Standard *std, int inputc, char **inputs, int starting_index);

/**
 * Given a netlist (in the form of a library in order to avoid creating another
 * struct), parse the subsystems in it, and create a netlist for each one using
 * only gates (translate each subsystem all the way down to the gates it is defined
 * as in the library it is defined in).
 * 
 * Stores the translated compponents in dest, which is assumed to be allocated.
 * 
 * Starts the component ID numbering from the given one.
 * 
 * Returns:
 *  - 0 on success
 *  - -1 on failure
*/
int netlist_to_gate_only(Netlist *dest, Netlist *netlist, int component_id);

/**
 * Print the contents of the given library in the file with the given filename.
*/
void lib_to_file(Netlist *lib, char *filename, char *mode);

/**
 * Given a standard subsystem, create an instance of it with the given inputs/outputs.
 * 
 * Creates all components that std says, maps each component's inputs to whatever std says,
 * maps all subsystem outputs to whatever std says.
 * 
 * This function is completely subsystem agnostic, meaning that (assuming a proper standard)
 * virtually ANY subsystem can be instantiated with it.
*/
Subsystem *instantiate_subsys(Subsystem *std, char **inputs, int inputc, char **outputs, int outputc);
