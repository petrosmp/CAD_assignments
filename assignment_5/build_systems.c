#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "netlist.h"
#include "str_util.h"
#include <getopt.h>

#define GATE_LIB_NAME       "component.lib"
#define SUBSYS_LIB_NAME     "subsystem.lib"
#define INPUT_FILE          "input1.txt"
#define OUTPUT_FILE         "output1.txt"
#define SINGLE_BIT_FA_NAME  "FULL_ADDER"
#define SINGLE_BIT_FAS_NAME "FULL_ADDER_SUBTRACTOR"

void usage();
Standard* create_nbit_adder_subtractor(Standard *single_bit_FAS, char *name, int n, int inputc, char **inputs, int outputc, char **outputs);
Standard* create_nbit_full_adder(Standard *single_bit_std, char *name, int nbits, int inputc, char **inputs, int outputc, char **outputs);
int parse_input_file(char *filename, char **name, int *inputc, char ***inputs, int *outputc,  char ***outputs, int *n, char **requirement);
int read_libs(char *gate_lib_name, Netlist *gate_lib, char *subsys_lib_name, Netlist *subsys_lib);


int main(int argc, char *argv[]) {

    char ch, *gate_lib_name = GATE_LIB_NAME, *subsys_lib_name = SUBSYS_LIB_NAME, *input_name = INPUT_FILE, *output_file = OUTPUT_FILE;

    // parse any (optional) arguments
    while ((ch = getopt(argc, argv, "g:s:i:o:")) != -1) {
		switch (ch) {		
			case 'g':
				gate_lib_name = optarg;
				break;
			case 's':
				subsys_lib_name = optarg;
				break;
			case 'i':
				input_name = optarg;
				break;
            case 'o':
				output_file = optarg;
				break;
			default:
				usage();
                exit(0);
		}
	}

    // parse the libraries
    Netlist *gate_lib = malloc(sizeof(Netlist)), *lib = malloc(sizeof(Netlist));
    read_libs(gate_lib_name, gate_lib, subsys_lib_name, lib);

    // parse the input file
    int inputc = 0, outputc = 0, n = 0;
    char **inputs = NULL, **outputs = NULL, *name=NULL, *requirement = NULL;
    parse_input_file(input_name, &name, &inputc, &inputs, &outputc, &outputs, &n, &requirement);

    // check if the required subsystem is defined in the library
    Standard *required_std;
    if ( (required_std=find_in_lib(lib, requirement)) == NULL)  {
        return -1;
    }

    // call the appropriate function for the subsystem we want to create
    Standard *target = NULL; int sub=0;
    if (strncmp(requirement, SINGLE_BIT_FAS_NAME, strlen(SINGLE_BIT_FAS_NAME)) == 0) {                      // The full_adder_subtractor has to be before the full_adder of the n in str'n'cmp would have to be dropped
        target = create_nbit_adder_subtractor(required_std, name, n, inputc, inputs, outputc, outputs);
        sub = 1;
    } else if (strncmp(requirement, SINGLE_BIT_FA_NAME, strlen(SINGLE_BIT_FA_NAME)) == 0) {
        target = create_nbit_full_adder(required_std, name, n, inputc, inputs, outputc, outputs);
    } else {
        printf("Unknown system type '%s'\n", requirement);
    }

    // put the newly created subsystem in a library
    Netlist *net = malloc(sizeof(Netlist));
    net->contents = NULL; net->file=NULL; net->type=target->type;
    add_to_lib(net, target, 1, SUBSYSTEM);

    // translate that library to gates-only
    Netlist *only_gates_lib = malloc(sizeof(Netlist));
    if (netlist_to_gate_only(only_gates_lib, net, 1)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }

    // print the gates-only netlist to the file
    old_lib_to_file(only_gates_lib, output_file, "w", sub?6:5);


    // cleanup
    free_lib(only_gates_lib);
    free_lib(net);
    free_lib(lib);
    free_lib(gate_lib);
    free_str_list(inputs, inputc); free_str_list(outputs, outputc);
    free(requirement);
    free(name);

    printf("Program executed successfully\n");

    return 0;
}

void usage() {
    printf("Usage: build_systems [<option> <argument>]\n");
    printf("Available options:\n");
    printf("\t-g <filename>:\tuse the file with the given name as the component (gate) library (default %s)\n", GATE_LIB_NAME);
    printf("\t-s <filename>:\tuse the file with the given name as the subsystem library (default %s)\n", SUBSYS_LIB_NAME);
    printf("\t-i <filename>:\tuse the file with the given name as the input netlist (default %s)\n", INPUT_FILE);
    printf("\t-o <filename>:\twrite the output to a file with the given name (will be overwritten if it already exists) (default %s)\n", OUTPUT_FILE);
}

/**
 * @brief Creates and returns an n-bit adder_subtractor Standard.
 * 
 * @param name      The name that the created Standard will have
 * @param n         The number of bits that the Standard will be able to accomodate
 * @param inputc    The length of the list of input names
 * @param inputs    The list of input names (A7, A6, ..., A0, B7, B6, ..., B0, Cin, ADD'_SUB)
 * @param outputc   The length of the list of output names
 * @param outputs   The list of output names (S7, S6, ..., S0, Cout)
 * @return (a pointer to) the newly created Standard
 */
Standard* create_nbit_adder_subtractor(Standard *single_bit_FAS, char *name, int n, int inputc, char **inputs, int outputc, char **outputs) {

    Subsystem *s = malloc(sizeof(Subsystem));

    // set the name
    s->name = malloc(strlen(name)+1);
    strncpy(s->name, name, strlen(name)+1);

    // set the input names
    s->_inputc = inputc;
    deepcopy_str_list(&(s->inputs), inputs, inputc);

    // set the output names
    s->_outputc = outputc;
    deepcopy_str_list(&(s->outputs), outputs, outputc);

    // create the components
    s->components = NULL;
    s->_tail = NULL;

    // create n single-bit adder_subtractor components
    for (int i=0; i<n; i++) {

        Component *comp = malloc(sizeof(Component));

        // set the component's fields
        comp->id = i+1;
        comp->prototype = single_bit_FAS;
        comp->_inputc = single_bit_FAS->subsys->_inputc;
        comp->is_standard = 1;  // the component is part of a standard subsystem
        comp->inputs = NULL;    // since the component is part of a standard subsystem, it only has dynamic mappings
        comp->i_maps = malloc(sizeof(Mapping*) * comp->_inputc);
        
        // create the dynamic input mappings for the new component (circuit specific - hardcoded)

        // first input (A) is mapped to A_i
        comp->i_maps[0] = malloc(sizeof(Mapping));
        comp->i_maps[0]->type = SUBSYS_INPUT;
        comp->i_maps[0]->index = n-i-1;

        // second input (B) is mapped to B_i (which is n spots after A_i)
        comp->i_maps[1] = malloc(sizeof(Mapping));
        comp->i_maps[1]->type = SUBSYS_INPUT;
        comp->i_maps[1]->index = 2*n-i-1;

        // third input (Cin) is mapped to external Cin if i==0 or Cin of previous component otherwise
        comp->i_maps[2] = malloc(sizeof(Mapping));
        if (i==0) {
            comp->i_maps[2]->type = SUBSYS_INPUT;
            comp->i_maps[2]->index = (2*n);   // cin is at index 2n (after n A's and n B's)
        } else {
            comp->i_maps[2]->type = SUBSYS_COMP;
            comp->i_maps[2]->index = i-1;
            comp->i_maps[2]->out_index = 1;     // full_adder_subtractor.outputs[1] is COUT
        }
        
        // fourth input is mapped to (external) control signal
        comp->i_maps[3] = malloc(sizeof(Mapping));
        comp->i_maps[3]->type = SUBSYS_INPUT;
        comp->i_maps[3]->index = (2*n)+1;   // control singlas is at 2n+1 (affter n A's, n B's and 1 Cin)
        
        // add the component to the subsystem
        subsys_add_comp(s, comp);

    }

    // set output_mappings to null (this is a standard subsystem, it only has dynamic mappings)
    s->output_mappings = NULL;

    // set the dynamic output mappings (circuit specific - hardcoded)
    s->o_maps = malloc(sizeof(Mapping*) * s->_outputc);

    // the first n outputs are mapped to S's
    for (int i=0; i<n; i++) {
        s->o_maps[i] = malloc(sizeof(Mapping));
        s->o_maps[i]->type = SUBSYS_COMP;
        s->o_maps[i]->index = n-i-1;
        s->o_maps[i]->out_index = 0;    // full_adder_subtractor.outputs[0] is S
    }

    // the last output is mapped to the Cout of the last (index n-1) component
    s->o_maps[n] = malloc(sizeof(Mapping));
    s->o_maps[n]->type = SUBSYS_COMP;
    s->o_maps[n]->index = n-1;
    s->o_maps[n]->out_index = 1;    // full_adder_subtractor.outputs[1] is COUT


    // set the subsystem as standard and wrap it an a struct
    s->is_standard = 1;

    Standard *std = malloc(sizeof(Standard));
    std->type = SUBSYSTEM;
    std->subsys = s;
    std->defined_in = NULL;


    return std;
}

/**
 * Create an n-bit full addder standard subsystem, with the given
 * input/output names and with single bit full adders that are
 * created according to single_bit_std as components.
 * 
 * Creates (and allocates memory for) all components and input/output
 * lists as well as the subsystem itself.
*/
Standard* create_nbit_full_adder(Standard *single_bit_std, char *name, int nbits, int inputc, char **inputs, int outputc, char **outputs) {

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

int parse_input_file(char *filename, char **name, int *inputc, char ***inputs, int *outputc,  char ***outputs, int *n, char **requirement) {

    // we also need some temporary variables and (1 and 2 dimensional) buffers
    char **buf = NULL;
    int buf_len = 0;
    char *tmp;

    // loop through the lines of the file and get the contents
    char *line = NULL;
    int nread = 0;
    int offset = 0;
    size_t len = 0;
    while((nread = read_line_from_file(&line, filename, &len, offset)) != -1) {

        char *_line = line;

        if (starts_with(_line, ENTITY_START)) {
            _line += strlen(ENTITY_START)+1;        // skip the declaration and the first space
            tmp = split(&_line, PORT_MAP_DELIM);    // keep the part until the second space
            
            // allocate memory for the name and save it
            (*name) = malloc(strlen(tmp)+1);   
            strncpy((*name), tmp, strlen(tmp)+1);

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
            (*n) = atoi(tmp);  // this should be implemented differently, with a vars-vals structure... (TODO)

        } else if (starts_with(line, REQUIREMENT_DECL)) {
            _line += strlen(REQUIREMENT_DECL)+1;     // skip the requirement declaration keyword and the following space

            // save the name of the required subsystem
            (*requirement) = malloc(strlen(_line)+1);
            strncpy(*requirement, _line, strlen(_line)+1);
        }

        if (starts_with(_line, PORT_MAP_INPUT)) {
            _line += strlen(PORT_MAP_INPUT)+1;  // skip the declaration and the following space
            
            // get the input name/prefix (for example A)
            tmp = split(&_line, PORT_MAP_DELIM);

            // get the input signals (in a list)
            buf=NULL;
            buf_len = str_to_list(_line+strlen(PORT_MAP_COLON), &buf, PORT_MAP_SIGNAL_DELIM);

            // append the buffer list to the input list
            (*inputc) += list_concat(inputs, (*inputc), buf, buf_len);

            free_str_list(buf, buf_len);

        } else if (starts_with(_line, PORT_MAP_OUTPUT)) {
            
            _line += strlen(PORT_MAP_OUTPUT)+1;  // skip the declaration and the following space
            
            // get the output name/prefix (for example A)
            tmp = split(&_line, PORT_MAP_DELIM);

            // get the output signals (in a list)
            buf=NULL;
            buf_len = str_to_list(_line+strlen(PORT_MAP_COLON), &buf, PORT_MAP_SIGNAL_DELIM);

            // append the buffer list to the output list
            (*outputc) += list_concat(outputs, (*outputc), buf, buf_len);

            free_str_list(buf, buf_len);

        }

        // move the cursor
        offset += nread;
    }
    free(line);

    return 0;

}

int read_libs(char *gate_lib_name, Netlist *gate_lib, char *subsys_lib_name, Netlist *subsys_lib) {

    // read the component library where the gates that may be used are defined
    if (gate_lib_from_file(gate_lib_name, gate_lib)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }
    
    // read the subsystem library where the subsystems that may be used are defined
    if (subsys_lib_from_file(subsys_lib_name, subsys_lib, gate_lib)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }

    return 0;
}
