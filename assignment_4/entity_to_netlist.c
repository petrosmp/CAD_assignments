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

void usage(char *name);
Standard* create_full_adder_standard(char *name, char **inputs, int inputc, char **outputs, int outputc, int nbits,Standard *single_bit_std);

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
     * The input format as presented in the project specification contains two (different)  *
     * stages of writing HDL code:                                                          *
     *  1) declaring an entity                                                              *
     *  2) instantiating an entity - mapping its inputs/outputs to (external) signals       *
     *                                                                                      *
     * Since the above are distinct from one another (and 2 actually requires 1), they are  *
     * also done separately, each with its respective function (see the functions declared  *
     * at the top of this file).                                                            *
     *                                                                                      *
     * The process is thus the following:                                                   *
     *  - the file is parsed, and information needed for each stage is stored in arrays     *
     *    (input_names, output_names are needed for declaration, input_signals,             *
     *    output_signals are needed for instantiation)                                      *
     *  - the component is "declared", meaning a standard on which future instantiations    *
     *    will be based is created.                                                         *
     *  - a subsystem following that standard is instantiated with the input/output signals *
     *    that the input file specified mapped to it's inputs/outputs respectively.         *
     *                                                                                      *
    \***************************************************************************************/

    char **input_names = NULL;
    int input_names_c = 0;
    char **output_names = NULL;
    int output_names_c = 0;

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

            // allocate memory for the input names
            input_names = realloc(input_names, sizeof(char*) * (input_names_c+buf_len));

            // if the signals are more than one it means the input is a bit vector which we need to enumerate
            if (buf_len > 1) {

                for (int i=buf_len-1; i>=0; i--) {              // we go from n-1 to 0 inclusive
                    char *a = malloc(strlen(tmp)+digits(i)+2);  // allocate memory for the prefix, the number and a null byte

                    // "create" the enumerated name (for example A3, A2, A1, etc.)
                    sprintf(a, "%s%d", tmp, i);

                    // add it to the list
                    input_names[input_names_c] = malloc(strlen(a)+1);
                    strncpy(input_names[input_names_c++], a, strlen(a)+1);

                    free(a);
                }

            } else {    // otherwise it is just a single bit, just add it to the list
                input_names[input_names_c] = malloc(strlen(tmp)+1);
                strncpy(input_names[input_names_c++], tmp, strlen(tmp)+1);
            }


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

            // allocate memory for the output names
            output_names = realloc(output_names, sizeof(char*) * (output_names_c+buf_len));

            // if the signals are more than one it means the output is a bit vector which we need to enumerate
            if (buf_len > 1) {

                for (int i=buf_len-1; i>=0; i--) {              // we go from n-1 to 0 inclusive
                    char *a = malloc(strlen(tmp)+digits(i)+2);  // allocate memory for the prefix, the number and a null byte

                    // "create" the enumerated name (for example A3, A2, A1, etc.)
                    sprintf(a, "%s%d", tmp, i);

                    // add it to the list
                    output_names[output_names_c] = malloc(strlen(a)+1);
                    strncpy(output_names[output_names_c++], a, strlen(a)+1);

                    free(a);
                }

            } else {    // otherwise it is just a single bit, just add it to the list
                output_names[output_names_c] = malloc(strlen(tmp)+1);
                strncpy(output_names[output_names_c++], tmp, strlen(tmp)+1);
            }


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
     *  - We then create a standard for the subsystem, describing its internals             *
     *    (components, output mappings etc.) (this is the only part where the full adder    *
     *    stuff is "hard-coded", everything else is generic)                                *
     *  - We create an instance of the requested subsystem, according to the standard       *
     *    defined above, and map its inputs/outputs to the ones described in the input      *
     *    file.                                                                             *
     *  - Finally we print the netlist of that instance to an output file                   *
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

    // create the standard for the requested full adder
    Standard *nbit_std = create_full_adder_standard(
        name,
        input_names, input_names_c,
        output_names, output_names_c,
        n,
        single_bit_std
    );

    // instantiate the requested full adder
    Subsystem *instance = instantiate_subsys(nbit_std->subsys, input_signals, input_signals_c, output_signals, output_signals_c);

    // write the instance's netlist to a file
    netlist_to_file(instance, output, "w");
    
    // display a success message
    printf("Success! The output netlist was written to %s\n", output);

    // cleanup
    free_str_list(input_names, input_names_c);
    free_str_list(input_signals, input_signals_c);
    free_str_list(output_names, output_names_c);
    free_str_list(output_signals, output_signals_c);
    free(name);

    free_subsystem(instance, 1);
    free_standard(nbit_std);

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
Standard* create_full_adder_standard(char *name, char **inputs, int inputc, char **outputs, int outputc, int nbits, Standard *single_bit_std) {

    Subsystem *new = malloc(sizeof(Subsystem));

    // copy the name over
    new->name = malloc(strlen(name)+1);
    strncpy(new->name, name, strlen(name)+1);

    // copy the input and output names over
    new->_inputc = inputc;
    new->inputs = malloc(sizeof(char*) * inputc);
    for (int i=0; i<inputc; i++) {
        new->inputs[i] = malloc(strlen(inputs[i])+1);
        strncpy(new->inputs[i], inputs[i], strlen(inputs[i])+1);
    }

    new->_outputc = outputc;
    new->outputs = malloc(sizeof(char*) * outputc);
    for (int i=0; i<outputc; i++) {
        new->outputs[i] = malloc(strlen(outputs[i])+1);
        strncpy(new->outputs[i], outputs[i], strlen(outputs[i])+1);
    }

    // set the new subsystem as standard
    new->is_standard = 1;

    // set the components (specific to full adder)
    new->components = NULL;
    for(int i=0; i<nbits; i++) {

        Component *comp = malloc(sizeof(Component));
        comp->id = i+1;
        comp->is_standard = 1;
        comp->prototype = single_bit_std;   // specific to full adder
        
             
        if (comp->prototype->subsys->inputs == NULL) {
            printf("is null\n");
        }
        // set the component input names
        comp->inputs = NULL;
        
        
        if (&(comp->inputs) == NULL) {
            printf("normal\n");
        }
        
        deepcopy_str_list(&(comp->inputs), comp->prototype->subsys->inputs, comp->prototype->subsys->_inputc);

        if (comp->prototype->subsys == NULL) {
            printf("is null\n");
        }
        comp->_inputc = comp->prototype->subsys->_inputc;

        // set the component input mappings (specific to full adder)
        comp->i_maps = malloc(sizeof(Mapping*) * comp->_inputc);
        
        // inputs A, B (we assume subsys input array is Ai..., Bi..., Cin)
        comp->i_maps[0] = malloc(sizeof(Mapping));
        comp->i_maps[0]->type = SUBSYS_INPUT;
        comp->i_maps[0]->index = nbits - 1 - i;
        
        comp->i_maps[1] = malloc(sizeof(Mapping));
        comp->i_maps[1]->type = SUBSYS_INPUT;
        comp->i_maps[1]->index = 2*nbits - 1 - i;

        // input C needs some handling
        comp->i_maps[2] = malloc(sizeof(Mapping));
        if (i==0) {
            // subsystem Cin
            comp->i_maps[2]->type = SUBSYS_INPUT;
            comp->i_maps[2]->index = 2*nbits;
        } else {
            // previous component Cout
            comp->i_maps[2]->type = SUBSYS_COMP;
            comp->i_maps[2]->index = i-1;
            comp->i_maps[2]->out_index = comp->prototype->subsys->_outputc-1;
        }
        
        subsys_add_comp(new, comp);

    }

    // set the output mappings (specific to full adder)
    new->o_maps = malloc(sizeof(Mapping*) * new->_outputc);

    // the first n are the S outputs
    for(int i=0; i<new->_outputc-1; i++) {
        new->o_maps[i] = malloc(sizeof(Mapping));
        new->o_maps[i]->type = SUBSYS_COMP;
        new->o_maps[i]->index = nbits-1-i;
        new->o_maps[i]->out_index = 0; // S output
    }

    // the last one is Cout
    new->o_maps[new->_outputc-1] = malloc(sizeof(Mapping));
    new->o_maps[new->_outputc-1]->type = SUBSYS_COMP;
    new->o_maps[new->_outputc-1]->index = nbits-1;
    new->o_maps[new->_outputc-1]->out_index = 1; // Cout output

    // initialize the rest of the fields to null to avoid segfaults
    // (if they are not explicitly set to null, null checks will not work
    // and garbage will be interpreted as pointers, leading to segfault)
    new->output_mappings = NULL;

    // create the standard that wraps the subsystem
    Standard *s = malloc(sizeof(Standard));
    s->defined_in = NULL;
    s->type = SUBSYSTEM;
    s->subsys = new;

    return s;

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
