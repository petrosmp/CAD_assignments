/**
 * \file Custom set of structures and functions that make working with
 * component libraries (only containing subsystems) cleaner
 * and allow for code reuse and easy modification.
*/

#include "str_util.h"
#include <stdio.h>

#define DECL_DESIGNATION "COMP "    /* The word that signifies that a line declares a subsystem */
#define INPUT_DESIGNATION "IN: "    /* The word that signifies that the next part of a string is the inputs of the subsystem. */
#define OUTPUT_DESIGNATION "OUT: "  /* The word that signifies that the next part of a string is the outputs of the subsystem. */
#define IN_OUT_DELIM ", "           /* The delimeter that separates inputs/outputs from each other. i.e. for outputs A, B and C and INOUT_DELIM "," the output list will be: A,B,C */
#define GENERAL_DELIM " ; "         /* The delimiter that separates fields. */
#define COMP_ID_PREFIX "U"          /* The prefix of every component id. When printing component c, COMP_ID_PREFIX<c.id> will be printed */
#define COMP_DELIM  " "             /* The delimiter separating the attributes of a component */
#define MAX_LINE_LEN 512            /* The maximum allowed length of a line in a netlist file */
#define COMMENT_PREFIX "%%"         /* The prefix of any comment line */
#define KEYWORD_PREFIX "**"         /* The prefix of any keyword line */
#define NETLIST_START "BEGIN "      /* The word that signifies that a netlist is contained in the following lines */
#define NETLIST_END "END "          /* The word that signifies that a netlist ends in this line */
#define UNEXPECTED_EOF -2           /* Error code returned when a file ends unexpectedly, leaving a netlist incomplete */
#define SYNTAX_ERROR -3             /* Error code returned when a syntax error (of any kind) is detected while parsing a netlist */
#define UNKNOWN_COMP -4             /* Error code returned when a netlist contains a component that we have not seen in any library */
#define MAP_DELIM " = "             /* The delimiter between a mapping's name and its "target" in a netlist */
#define ENTITY_START "ENTITY"       /* The string that indicates that an entity declaration starts in this line */
#define ENTITY_END "END"            /* The string that indicates that an entity declaration ends in this line */
#define VAR_DECLARATION "VAR"       /* The string that indicates that a line contains a variable declaration */
#define VAR_ASSIGNMENT "= "         /* The string that lies between a variables name and its value */
#define PORT_START "PORT ("         /* The string that indicates that a port map begins in this line */
#define PORT_END ");"               /* The string that indicates that a port map begins in this line */
#define PORT_MAP_INPUT "IN"         /* The string that indicates that a port map line contains an input signal */
#define PORT_MAP_OUTPUT "OUT"       /* The string that indicates that a port map line contains an output signal */
#define PORT_MAP_DELIM " "          /* The delimiter that separates fields in a port map line */
#define PORT_MAP_SIGNAL_DELIM " , " /* The delimiter that separates (input/output) signals in a port map */
#define PORT_MAP_COLON ": "         /* The delimiter between the input/output declarations and the signal names */
#define REQUIREMENT_DECL "LIB"      /* The string that indicates that a required subsystem is specified in this line */
#define MAP_COMP_OUT_SEP "_"        /* The string that separates the component ID from the output name in a mapping */
#define GENERIC_ERROR -7            /* Error code indicating an error that does not fall under a specific category. An error message will usually be printed to clarify. */


/**
 * Since a single node structure is used for all linked list needs of the
 * program, we need a way to have a way to tell what each node contains.
 * 
 * Remember to add a way for @ref search_in_llist to find the 'name' of 
 * the new node type if you add a new node type.
*/
enum NODE_TYPE {
    STANDARD,       /* The node contains a standard subsystem (as read from a library) */
    SUBSYSTEM_N,    /* The node contains a functional subsystem (as described in a netlist) */
    COMPONENT,      /* The node contains a component (any part of a subsystem) */
    ALIAS           /* The node contains an alias of a signal */
};

/**
 * Since only one structure is used to represent items read from a library,
 * we need a way to tell what each item is.
 * 
 * TODO: The same enumeration is used to mark the types of libraries, so a 
 * change of name may be in order.
 * 
 * If you add a new standard type you also may need to add a way for @ref
 * search_in_llist to find the 'name' of the new standard type.
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
        struct standard *std;       /* The standard the node contains */
        struct subsystem *subsys;   /* The subsystem the node contains */
        struct component *comp;     /* The component the node contains */
        struct alias *alias;        /* The alias the node contains */
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
    enum STANDARD_TYPE type;        /* The possible types are the same  */
    struct linked_list *contents;   /* The contents of the library */
    char *file;                     /* The file the library was defined in */
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
    int truth_table;                /* The truth table of the gate, represented as a bitstring (integer) */
} Gate;

/**
 * A subsystem is a circuit with both inputs and outputs, comprised by
 * gates or other subsystems.
 * 
 * Subsystems are initially defined in a subsystem library and once defined
 * they can be used as components in other subsystems.
*/
typedef struct subsystem {
    char* name;                     /* The name of this subsystem (ASCII, human readable). */
    int _inputc;                    /* The number of inputs the subsystem has (mainly for internal use). */
    char** inputs;                  /* The names of the inputs of the subsystem. */
    int _outputc;                   /* The number of outputs the subsystem has (mainly for internal use). */
    char** outputs;                 /* The names of the outputs of the subsystem. */
    struct linked_list *components; /* The list of the subsystem components */
    char **output_mappings;         /* The list of the mappings of internal signals to the subsystem's outputs (if it is a functional one) */
    int is_standard;                /* Boolean flag indicating whether the subsystem is a standard one */
    Mapping **o_maps;               /* If the subsystem is a standard one, along the outputs there will be output mappings */
    struct linked_list *aliases;    /* The signal aliases that the netlist in which the subsystem was defined used. Useful only during parsing. */
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
    int buffer_index;       /* The index of the component in the simulation buffers */
} Component;

/**
 * A singly linked list, containing pointers to its first and last nodes.
*/
typedef struct linked_list {
    Node *head;     /* the first element of the list */
    Node *tail;     /* the last element in the list */
} LList;

/**
 * @brief An alias is a way to assign a name to a signal in a netlist to
 *        make it more readable.
 * 
 * @details For example, consider the following netlist:
 * 
 * @verbatim
 * COMP MUX3 ; IN: A, B, C, E, C1, C2 ; OUT: D
 *      BEGIN MUX3 NETLIST
 *          U1 MUX A, B, C1
 *          U99 NOT C1
 *          U2 MUX C, E, U99
 *          U3 MUX U1_D, U2_D, C2
 *          D = U3_D
 *      END MUX3 NETLIST
 * @endverbatim
 * 
 * We could use aliases to make it more readable (though a bit longer):
 * 
 * @verbatim
 * COMP MUX3 ; IN: A, B, C, E, C1, C2 ; OUT: D
 *      BEGIN MUX3 NETLIST \n
 *          U1 MUX A, B, C1
 *          MUX1_OUT = U1_D
 *          U99 NOT C1
 *          C1' = U99
 *     ->   U2 MUX C, E, C1'
 *          MUX2_OUT = U2_D
 *     ->   U3 MUX MUX1_OUT, MUX2_OUT, C2
 *          MUX3_OUT = U3_D
 *     ->   D = MUX3_OUT
 *      END MUX3 NETLIST
 * @endverbatim
 * 
 * Notice that the lines with the arrow (->) can be understood on their own:
 * we can understand what <code>'U3 MUX MUX1_OUT, MUX2_OUT, C2'</code> means just by
 * reading it, while <code>'U3 MUX U1_D, U2_D, C2'</code> forces us to go back and check
 * what U1 and U2 are.
 * 
*/
typedef struct alias {
    char *name;         /* the name of the alias (how it will be referred to in a netlist) */
    Mapping *mapping;   /* a mapping to the thing that this is an alias of */
} Alias;

/**
 * @brief   A testbench is an instance of a simulation of a circuit. It consists of the UUT,
 *          the values that will be tested as inputs and the outputs that will be displayed.
 */
typedef struct testbench {
    Subsystem *uut;     /* the Unit Under Test, the subsystem whose function will be simulated */
    char ***values;     /* the list of values that will be tried for each input */
    int v_c;            /* the number of values (and thus simulations) that this testbench provides */
    int *outs_display;  /* a list of booleans indicating whether or not each output should be displayed (all 0 by default) */
} Testbench;

/**
 * @brief Initialize a linked list instance.
 * 
 * @details Allocate memory for the list and sets head and tail to NULL.
 * 
 * @return The newly created list
 */
LList* ll_init();

/**
 * Add the given node to the given linked list.
*/
int ll_add(LList *ll, Node* new_element);

/**
 * Properly free the list pointed to by l, node by node.
 * The 'complete' flag indicates whether (1) or not (0) the contents
 * of the nodes should also be free()'d.
*/
void ll_free(LList *l, int complete);

/**
 * @brief Print the contents of the given linked list.
 * 
 * @details Only here to make debugging easier :)
 * 
 * @param l The list whose contents will be printed.
 */
void ll_print(LList *l);

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
int str_to_gate(char *str, Gate *g, int n, int parse_tt);

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
 * Read up to n bytes from str and parse the information into the fields
 * of a. Everything that a refers to will be looked for in s. @see resolve_mapping()
 * 
 * Does not allocate memory for the alias itself, but does for the mapping.
 * 
 * @return exactly what @ref str_to_mapping does.
 * 
 * @retval 0 on success
 * @retval NES on failure because of not enough space
 * @retval UNKNOWN_COMP if str references a comp that cannot be found in subsys
 * @retval GENERIC_ERROR if any other error occurs
*/
int str_to_alias(char *str, Alias* a, Subsystem *s, int n);

/**
 * Writes an ASCII representation of the given alias to str, writing no more
 * than n bytes, null terminator ('\0') included.
 * 
 *  @retval 0 on success
 *  @retval NES on failure because of not enough space
 *  @retval NARG on failure because of null arguments.
*/
int alias_to_str(Alias *a, char *str, int n);

/**
 * @brief Properly free up the memory allocated for and taken up by the given alias.
 * 
 * @param a The alias whose memory should be freed.
 */
void free_alias(Alias *a);

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
 * @brief Properly free up the memory occupied by the given testbench structure.
 * 
 * @note    Always free the UUT subsystem AFTER the testbench. This function
 *          depends on the UUT's attributes to be able to do its job correctly, and
 *          behavio if the UUT is free'd before this is called is undefined.
 * 
 * @param tb    A pointer to the memory that will be freed.
 */
void free_tb(Testbench *tb);

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
 * @brief   Store a human readable string representation of the given
 *          mapping in the given char*. Write no more than n bytes,
 *          null terminator ('\0') included.
 * 
 *          If the mapping refers to a component that is a gate, then
 *          out_index should be set to -1 for this to work properly.
 */
void mapping_to_str(Mapping *m, char *str, int n);

/**
 * @brief   Resolve m to the name of what it points to and write it to str.
 * 
 * @details The returned string is always null terminated, and just as long as
 *          it has to be.
 *
 *          The subsystem where the mapping was found (and thus withing which
 *          it must be resolved) should be passed to this function in s.
 * 
 * @example Let m = {type=SUBSYS_COMP, index=5, out_index=1}. If the 6'th component
 *          of s has ID 89 and its second output is named "BAR", str will be set to
 *          'U89_BAR'.
 * 
 * @param m     The mapping to be resolved
 * @param s     The subsystem within which the mapping will be resolved
 * 
 * @note    Note that the string returned from this will need freeing.
 * 
 * @retval  a pointer to the string on success (null terminated)
 * @retval  NULL on any error
 */
char* resolve_mapping(Mapping *m, Subsystem *s);

/**
 * @brief   Turn a string that contains a mapping definition into a @ref Mapping struct.
 * 
 * @details Given a string that assigns a name to a signal (either marks it as an
 *          output or just an alias) and the @ref Subsystem that the signal belongs to
 *          (either as an input or as an output of a component), create a @ref Mapping
 *          that references it.
 * 
 * @param str       The string that contains the definition (something like 'BAR = FOO')
 * @param subsys    The subsystem that contains the thing to which the mapping points (
 *                  the one that contains 'FOO' in the above example)
 * @param m         A pointer to the mapping that will be altered to reflect the given
 *                  definition
 * @param n         The maximum number of bytes that will be read from str.
 * 
 * @retval 0 on success
 * @retval NES on failure because of not enough space
 * @retval UNKNOWN_COMP if str references a comp that cannot be found in subsys
 * @retval GENERIC_ERROR if any other error occurs
 */
int str_to_mapping(char *str, Subsystem *subsys, Mapping *m, int n);

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
int str_to_comp(char *str, Component *c, int n, Netlist *lib, Subsystem *s, int is_standard, int *buffer_index);

/**
 * Move x positions (forward) in the given list. Returns the node
 * that is there if the move is valid, NULL otherwise
*/
Node *move_in_list(int x, LList* list);

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
 * @brief   Write the netlist for the given (non-standard) subsystem (inputs,
 *          components, outputs) to the given filename.
 * 
 *          Non-standard means that the subsystem and its components have to have
 *          explicit input/output mappings and not dynamic ones. If there are dynamic
 *          ones they will be ignored, if there are no explicit ones nothing will be
 *          printed.
 * 
 *          If the mode is an 'append' one (see fopen()'s documentation), an extra
 *          newline will be written to the file before the subsystem's netlist.
 * 
 * @param s         The subsystem whose contents will be written to the file
 * @param filename  The file to which the output will be written
 * @param mode      The mode with which the file will be opened (passed verbatim
 *                  to fopen())
*/
void subsystem_to_file(Subsystem *s, char *filename, char *mode);

/**
 * @brief   The same as subsystem_to_file() but takes as an argument a file pointer
 *          instead of a filename so that subsequent calls can be optimized, in the
 *          sense that the file will be opened and closed once instead of once for
 *          every call.
 * 
 * @param s     The subsystem whose contents will be written to the file
 * @param fp    A pointer to the file
 */
void s2f(Subsystem *s, FILE *fp);

/**
 * @brief   Write the contents of a given netlist to a file.
 *
 *          A netlist may represent any HDL file, as long as it contains one or more
 *          non-standard subsystems.
 * 
 *          If the subsystems in the given netlist are standard (i.e. have the is_standard
 *          flag set and/or contain dynamic mappings), no error will be explicitly raised,
 *          the dynamic parts will just be ignored.
 * 
 *          If the subsystems in the given netlist (or their components) have no explicit
 *          input/output lists, no error will be explicitly raised but nothing will be
 *          printed (since there is nothing to be printed).
 * 
 * @param netlist   The netlist whose contents will be printed.
 * @param filename  The file to which the output will be written
 * @param mode      The mode with which the file will be opened (passed verbatim
 *                  to fopen())
 */
void netlist_to_file(Netlist *netlist, char *filename, char *mode);

/**
 * @brief   Print a given library's contents in a file.
 *
 *          This is to be used with actual libraries (i.e. whose contents are
 *          standards, not gates/subsystems). Using this function with a netlist
 *          whose contents are just subsystems (e.g. one that may represent an HDL
 *          file) will cause problems. Use netlist_to_file with those netlists
 *          instead.
 * 
 * @param lib       The library whose contents will be printed
 * @param filename  The filename that the output will be written to
 * @param mode      The mode with which the file will be opened (passed verbatim
 *                  to fopen())
 */
void lib_to_file(Netlist *lib, char *filename, char *mode);

/**
 * @brief   Print a given library's contents in a file, but in the form that
 *          is more useful for debugging than for "normal" use. 
 * 
 *          That basically means that the any mappings (either as outputs of
 *          a subsystem of as inputs of its components) will not be resolved,
 *          but printed as mappings.
 * 
 *          For example, if a subsystem output is mapped to the 3'rd output of
 *          the 4'th component, we would expect something like 'U4_OUT3' to be
 *          printed along that output.
 *          This function will instead print 'component 4's output 3'.
 * 
 *          This allows us to identify (and thus solve) problems that may arise
 *          from mapping misconfigurations easily, and also gives the potential
 *          user an insight as to what is actually being done "under the hood".
 * 
 *          Note that gates have nothing to do with dynamic mappings so they are
 *          treated the same here as in lib_to_file().
 * 
 * @param lib       The library whose contents will be printed
 * @param filename  The filename that the output will be written to
 * @param mode      The mode with which the file will be opened (passed verbatim
 *                  to fopen())
 */
void lib_to_file_debug(Netlist *lib, char *filename, char *mode);

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

/**
 * @brief   Create an instance of the given standard as a component.
 * 
 * @param std           The standard subsystem/gate that the new component will be an instance of
 * @param id            The ID of the new component
 * @param inputs        The inputs of the new component
 * @param inputc        The number of inputs that the new component will have (has to match the
 *                      number of inputs that the standard has, though this is not checked)
 * @return (a pointer to) the newly instantiated component
 */
Component *instantiate_component(Standard *std, int id, char **inputs, int inputc);

/**
 * @brief Search teh given library for a standard with the given name.
 * 
 * @param lib   The library to search in
 * @param name  The name of the desired standard
 * @return (a pointer to) the standard with the given name if found, NULL otherwise.
 */
Standard *find_in_lib(Netlist *lib, char *name);

/**
 * @brief Search in the given list for a node of the given type that contains
 *        *something* with the given name or ID.
 * 
 * @details In order to be able to work on any linked list, no matter the type of
 *          the contents of its nodes, we depend on the type of each node to
 *          determine which attribute of its data to compare to str. Because of
 *          that, if a type has no 'name' attibute, this will not work. If you
 *          add a new NODE_TYPE, remember to add a way for this function to find
 *          the 'name' of that type here.
 * 
 *          If the type of the node we are looking for is COMPONENT, then we will
 *          search with the ID. In that case str will be ignored (as will the id
 *          in every other case).
 *
 *          Optionally, if index is not NULL, the function will store there the
 *          index of the found element in the list.
 * 
 * @param list      The list to search in
 * @param t         The type of the node that we are looking for
 * @param str       The name of the thing we are looking for
 * @param n         The maximum number of bytes that will be compared to str
 * @param id        The id of the node that we are looking for
 * @param index     The position where the index of the item in the list will be written.
 * @retval  A pointer to the node that matches the search criteria if found
 * @retval  NULL if no such node is found or an error occured (NULL args for example)
 */
Node* search_in_llist(LList *list, enum NODE_TYPE t, char *str, int n, int id, int *index);

/**
 * @brief   Given a string representing a truth table of a gate (in the format described in the project
 *          specification), make it a bitstring and return it (in the form of an integer).
 * 
 * @note    Any non-bit characters (not '1' or '0') will be skipped.
 * 
 * @example     Suppose the input is '0, 1, 1, 0'. The bitstring would then be '0110', which means that
 *              the number 6 will be returned.
 * 
 * @example     Suppose that the input is '0, 1'. The bitstring would then be '01', which means that the
 *              number 1 will be returned.
 * 
 * @param _tt   The string containing the truth table.
 * @return      The bitstring representing the given truth table.
 */
int parse_truth_table(char *_tt);

/**
 * @brief   Given a truth table in bitstring (integer) form and a set of inputs
 *          as an array of characters, return a truth value (as an integer).
 * 
 * @note    The inputs array must be null terminated.
 * 
 * @param tt        The truth table.
 * @param inputs    The inputs.
 * @return The truth value of the table with the given inputs (1 or 0)
 */
int eval_at(int tt, char *inputs);

/**
 * @brief   Given a truth table in bitstring (integer) form, print it in a nice, human readable way.
 * 
 * @details To make debugging easy :)
 * 
 * @param tt        The truth table to be printed.
 * @param inputs    The number of inputs that the truth table is expected to accomodate.
 */
void print_as_truth_table(int tt, int inputs);





void old_lib_to_file(Netlist *lib, char *filename, char *mode, int mod);
