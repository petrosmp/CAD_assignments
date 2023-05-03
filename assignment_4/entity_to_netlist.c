#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "netlist.h"
#include "str_util.h"

#define FILENAME "sample_input.txt"
#define SIZE 100
#define GATE_LIB_NAME   "component.lib"
#define SUBSYS_LIB_NAME "subsystem.lib"
#define SINGLE_BIT_FA_NAME  "FULL_ADDER"
#define OUTPUT_FILE "netlist.txt"

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
#define COUT "COUT"
#define S "S"

void usage(char *name);
Subsystem* create_full_adder(char *name, char **inputs, int inputc, char **outputs, int outputc, int nbits,Standard *single_bit_std);

int main(int argc, char *argv[]) {

    // parse command line arguments to see if any default overriding is needed
    char ch;
    char *input = FILENAME;
    char *output = OUTPUT_FILE;
    char *gate_lib_name = GATE_LIB_NAME;
    char *subsys_lib_name = SUBSYS_LIB_NAME;
    char *single_bit_fa_name = SINGLE_BIT_FA_NAME;
    while ((ch = getopt(argc, argv, "f:g:s:o:n:h")) != -1) {
		switch (ch) {
			case 'f':
				input = optarg;
				break;
            case 'g':
				gate_lib_name = optarg;
				break;
            case 's':
				subsys_lib_name = optarg;
				break;
            case 'o':
				output = optarg;
				break;
            case 'n':
				single_bit_fa_name = optarg;
				break;
			default:
                usage(argv[0]);
                exit(0);
		}
	}

    /***************************************************************************************\
     *                                                                                      *
     * The input format as presented in the project specification declares a VHDL entity.   *
     *                                                                                      *
     * In order to achieve that functionality we need to keep track of the input/output     *
     * signals that the entity has, as well as any variables that may come in useful when   *
     * creating the subsystem.                                                              *
     *                                                                                      *
     * The process is thus the following:                                                   *
     *  - the file is parsed, and information needed is stored in arrays (input_signals,    *
     *    output_signals)                                                                   *
     *  - the subsystem is created                                                          *
     *                                                                                      *
    \***************************************************************************************/

    int n;
    char **input_signals = NULL;
    int input_signals_c = 0;
    char **output_signals = NULL;
    int output_signals_c = 0;

    // we also need some temporary variables and (1 and 2 dimensional) buffers
    char **buf = NULL;
    int buf_len = 0;
    char *name = NULL;
    char *tmp;

    // loop through the lines of the file and get the contents
    char *line = NULL;
    int nread = 0;
    int offset = 0;
    size_t len = 0;
    while((nread = read_line_from_file(&line, input, &len, offset)) != -1) {

        char *_line = line;

        if (starts_with(_line, ENTITY_START)) {
            _line += strlen(ENTITY_START)+1;        // skip the declaration and the first space
            tmp = split(&_line, PORT_MAP_DELIM);    // keep the part until the second space
            
            // allocate memory for the name and save it
            name = malloc(strlen(tmp)+1);   
            strncpy(name, tmp, strlen(tmp)+1);

        } else if (starts_with(line, PORT_START)) {

            /* this is necessary because there might be something useful (an input declaration for example)
               in the same line as the port map declaration.
            */

            _line += strlen(PORT_START)+1;  // skip the port map declaration and the following space

        } else if (starts_with(line, VAR_DECLARATION)) {

            _line += strlen(VAR_DECLARATION)+1;     // skip the variable declaration keyword and the following space
            
            tmp = split(&_line, PORT_MAP_DELIM);    // skip the name of the variable
            tmp = split(&_line, PORT_MAP_DELIM);    // skip the space before the equal sign
            tmp += strlen(VAR_ASSIGNMENT);          // skip the equal sign
            n = atoi(tmp);  // this should be implemented differently, with a vars-vals structure... (TODO)

        }

        if (starts_with(_line, PORT_MAP_INPUT)) {
            _line += strlen(PORT_MAP_INPUT)+1;  // skip the declaration and the following space
            
            // get the input name/prefix (for example A)
            tmp = split(&_line, PORT_MAP_DELIM);

            // get the input signals (in a list)
            buf=NULL;
            buf_len = str_to_list(_line+strlen(PORT_MAP_COLON), &buf, PORT_MAP_SIGNAL_DELIM);

            // append the buffer list to the input list
            input_signals_c += list_concat(&input_signals, input_signals_c, buf, buf_len);

            free_str_list(buf, buf_len);

        } else if (starts_with(_line, PORT_MAP_OUTPUT)) {
            
            _line += strlen(PORT_MAP_OUTPUT)+1;  // skip the declaration and the following space
            
            // get the output name/prefix (for example A)
            tmp = split(&_line, PORT_MAP_DELIM);

            // get the output signals (in a list)
            buf=NULL;
            buf_len = str_to_list(_line+strlen(PORT_MAP_COLON), &buf, PORT_MAP_SIGNAL_DELIM);

            // append the buffer list to the output list
            output_signals_c += list_concat(&output_signals, output_signals_c, buf, buf_len);

            free_str_list(buf, buf_len);

        }

        // move the cursor
        offset += nread;
    }
    free(line);

    /***************************************************************************************\
     *                                                                                      *
     * Now that the parsing is over and we have all the information that we need, in the    *
     * format that we need it, the fun part begins:                                         *
     *  - First of all we parse the gate library, since gates are the basic building blocks *
     *    of everything                                                                     *
     *  - Then we parse the subsystem library to find the subsystems that are needed to     *
     *    create the requested subsystem                                                    *
     *  - We then create the requested full adder                                           *
     *  - Finally we print the netlist of that full adder to an output file                 *
     *                                                                                      *
    \***************************************************************************************/

    // read the component library where the gates are defined
    Netlist *gate_lib = malloc(sizeof(Netlist));
    if (gate_lib_from_file(gate_lib_name, gate_lib)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }
    
    // read the subsystem library where the single-bit full adder is defined
    Netlist *lib = malloc(sizeof(Netlist));
    if (subsys_lib_from_file(subsys_lib_name, lib, gate_lib)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }

    // find the full adder standard in the subsystem library
    Standard *single_bit_std = NULL;
    Node *nod = lib->contents;
    while(nod!=NULL) {

        if (strncmp(nod->std->subsys->name, single_bit_fa_name, strlen(single_bit_fa_name)) == 0) {
            single_bit_std = nod->std;
            break;
        }
        nod = nod->next;
    }

    if (single_bit_std == NULL) {
        printf("Error! Could not find a subsystem with the expected name (%s) in the subsystem library (%s)\n", single_bit_fa_name, subsys_lib_name);
    }

    // create the requested full adder
    Subsystem *nbit_fa = create_full_adder(name, input_signals, input_signals_c, output_signals, output_signals_c, n, single_bit_std);

    // write the full adder's netlist to a file
    netlist_to_file(nbit_fa, output, "w");
    
    // cleanup
    free_str_list(input_signals, input_signals_c);
    free_str_list(output_signals, output_signals_c);
    free(name);

    free_subsystem(nbit_fa, 1);

    free_lib(lib);
    free_lib(gate_lib);

    return 0;
}

/**
 * Create an n-bit full addder standard subsystem, with the given
 * input/output names and with single bit full adders that are
 * created according to single_bit_std as components.
 * 
 * Creates (and allocates memory for) all components and input/output
 * lists as well as the subsystem itself.
*/
Subsystem* create_full_adder(char *name, char **inputs, int inputc, char **outputs, int outputc, int nbits, Standard *single_bit_std) {

    Subsystem *new = malloc(sizeof(Subsystem));

    // copy the name over
    new->name = malloc(strlen(name)+1);
    strncpy(new->name, name, strlen(name)+1);

    // copy the input and output names over
    new->_inputc = deepcopy_str_list(&(new->inputs), inputs, inputc);
    new->_outputc = deepcopy_str_list(&(new->outputs), outputs, outputc);
    
    // set the new subsystem as non-standard
    new->is_standard = 0;

    // set the components (specific to full adder)
    new->components = NULL;
    for(int i=0; i<nbits; i++) {

        Component *comp = malloc(sizeof(Component));
        comp->id = i+1;
        comp->is_standard = 0;
        comp->prototype = single_bit_std;   // specific to full adder
        comp->i_maps = NULL;

        // set the component inputs
        comp->inputs = malloc(sizeof(char*) * comp->prototype->subsys->_inputc);
        comp->_inputc = comp->prototype->subsys->_inputc;

        // inputs A, B are simple
        comp->inputs[0] = malloc(strlen(inputs[nbits-i-1])+2);
        strncpy(comp->inputs[0], inputs[nbits-i-1], strlen(inputs[nbits-i-1])+1);
        
        comp->inputs[1] = malloc(strlen(inputs[2*nbits - 1 - i])+2);
        strncpy(comp->inputs[1], inputs[2*nbits - 1 - i], strlen(inputs[2*nbits - 1 - i])+1);

        // input C needs some handling
        if (i==0) {
            comp->inputs[2] = malloc(strlen(inputs[2*nbits])+1);
            strncpy(comp->inputs[2], inputs[2*nbits], strlen(inputs[2*nbits])+1);

        } else {
            // previous component Cout
            comp->inputs[2] = malloc(1+digits(i)+1+strlen(COUT)+2);
            sprintf(comp->inputs[2], "U%d_%s", i, COUT);
        }
        
        subsys_add_comp(new, comp);

    }

    // set the output mappings (specific to full adder)
    new->output_mappings = malloc(sizeof(char*) * outputc);
    for(int i=0; i<outputc; i++) {
        if (i==outputc-1) {
            
            // last component cout
            new->output_mappings[i] = malloc(1+digits(i-1)+1+strlen(COUT)+1);        
            sprintf(new->output_mappings[i], "U%d_%s", nbits, COUT);
        
        } else {

            // comp nbits-1-i s
            new->output_mappings[i] = malloc(1+digits(nbits-i)+1+strlen(S)+1);        
            sprintf(new->output_mappings[i], "U%d_%s", nbits-i, S);
        }
    }

    // initialize the rest of the fields to null to avoid segfaults
    // (if they are not explicitly set to null, null checks will not work
    // and garbage will be interpreted as pointers, leading to segfault)
    new->o_maps = NULL;

    return new;

}

void usage(char *name) {
    printf("Usage: %s [option optarg]\n", name);
    printf("Available options:\n");
    printf("\t-f <filename>: specify the file in which the entity is defined (default: %s)\n", FILENAME);
    printf("\t-g <filename>: specify the file in which the component/gates library is defined (default: %s)\n", GATE_LIB_NAME);
    printf("\t-s <filename>: specify the file in which the subsystem library is defined (default: %s)\n", SUBSYS_LIB_NAME);
    printf("\t-o <filename>: specify the file in which the output netlist will be stored (default: %s)\n", OUTPUT_FILE);
    printf("\t-n <FA name> : specify the name that the single bit has in the subsystem library (default: %s)\n", SINGLE_BIT_FA_NAME);
}
