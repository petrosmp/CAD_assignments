#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "netlist.h"
#include "str_util.h"

#define FILENAME "sample_input.txt"
#define SIZE 100
#define GATE_LIB_NAME   "component.lib"
#define SUBSYS_LIB_NAME "subsystem.lib"
#define SINGLE_BIT_FA_NAME  "FULL_ADDER"

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

Subsystem* create_full_adder_standard(char *name, char **inputs, int inputc,\
                                      char **outputs, int outputc, int nbits,
                                      Standard *single_bit_std);

int main() {

    
    char *filename = FILENAME;

    char *line = NULL;  // no need to malloc it, getline() does that for us (and also reallocs if needed) 
    int nread = 0;
    int offset = 0;
    size_t len = 0;

    char **input_names = NULL;
    int input_names_c = 0;
    char **output_names = NULL;
    int output_names_c = 0;

    int n;
    char **input_signals = NULL;
    int input_signals_c = 0;
    char **output_signals = NULL;
    int output_signals_c = 0;

    
    char **buf = NULL;  // buffer to store the "useful" part of a line (the one with no declarations)
    int buf_len = 0;
    char *name = NULL;
    char *tmp;

    // loop through the lines of the file and get the contents
    while((nread = read_line_from_file(&line, filename, &len, offset)) != -1) {

        char *_line = line;

        if (starts_with(_line, ENTITY_START)) {
            _line += strlen(ENTITY_START)+1;        // skip the declaration and the following space
            tmp = split(&_line, PORT_MAP_DELIM);    // keep the part until the next space
            
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

    // we got it all
    // check input names equals input signals
    // check output names equals output signals


    /************************************************************************
    *                                                                       *
    *                                                                       *
    *                From here on down the real main starts                 *
    *                                                                       *
    *                                                                       *
    ************************************************************************/

   
    // read the component library where the gates are defined
    Netlist *gate_lib = malloc(sizeof(Netlist));
    if (gate_lib_from_file(GATE_LIB_NAME, gate_lib)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }
    // read the subsystem library where the single-bit full adder is defined
    Netlist *lib = malloc(sizeof(Netlist));
    if (subsys_lib_from_file(SUBSYS_LIB_NAME, lib, gate_lib)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }

    // find the full adder standard
    Standard *single_bit_std = NULL;
    Node *nod = lib->contents;
    while(nod!=NULL) {

        if (strncmp(nod->std->subsys->name, SINGLE_BIT_FA_NAME, strlen(SINGLE_BIT_FA_NAME)) == 0) {
            single_bit_std = nod->std;
            break;
        }
        nod = nod->next;
    }

    if (single_bit_std == NULL) {
        printf("error! Single bit full adder standard from library is null\n");
    }


    // call the standard creating function
    Subsystem *nbit_std = create_full_adder_standard(
        name,
        input_names, input_names_c,
        output_names, output_names_c,
        n,
        single_bit_std
    );

    // print the standard's info
    printf("Successfully created standard:\n");
    printf("Name: %s\n", nbit_std->name);
    for(int i=0; i<nbit_std->_inputc; i++) {
        printf("Input #%d: %s\n", i, nbit_std->inputs[i]);
    }
    for(Node *n=nbit_std->components; n!=NULL; n=n->next) {
        printf("Component U%d (%s):\n",n->comp->id, n->comp->prototype->subsys->name);
        printf("\tInputs:\n");
        
        for(int i=0; i<n->comp->_inputc; i++){
            printf("\t\t%s, mapped to %s ",n->comp->prototype->subsys->inputs[i], n->comp->i_maps[i]->type==SUBSYS_INPUT?"input":"component");
            if (n->comp->i_maps[i]->type==SUBSYS_INPUT) {
                printf("%d (%s)\n", n->comp->i_maps[i]->index, nbit_std->inputs[n->comp->i_maps[i]->index]);
            } else {
                Node *rtcn = move_in_list(n->comp->i_maps[i]->index, nbit_std->components); // referred_to_comp_node
                printf("%d, output %d (U%d_%s)\n", n->comp->i_maps[i]->index, n->comp->i_maps[i]->out_index, rtcn->comp->id, rtcn->comp->prototype->subsys->outputs[n->comp->i_maps[i]->out_index]);
            }
        }

        printf("\tOutputs:\n");
        for(int i=0; i<n->comp->prototype->subsys->_outputc; i++) {
            printf("\t\t%s\n", n->comp->prototype->subsys->outputs[i]);
        }

        printf("\n");
    }
    for(int i=0; i<nbit_std->_outputc; i++) {
        Node *rtcn = move_in_list(nbit_std->o_maps[i]->index, nbit_std->components); // referred_to_comp_node
        printf("Output #%d: %s, mapped to component %d, output %d (U%d_%s)\n", i, nbit_std->outputs[i], nbit_std->o_maps[i]->index, nbit_std->o_maps[i]->out_index, rtcn->comp->id, rtcn->comp->prototype->subsys->outputs[nbit_std->o_maps[i]->out_index]);
    }



    free_str_list(input_names, input_names_c);
    free_str_list(input_signals, input_signals_c);
    free_str_list(output_names, output_names_c);
    free_str_list(output_signals, output_signals_c);
    free(name);

    free_subsystem(nbit_std, 1);

    free_lib(lib);
    free_lib(gate_lib);
    return 0;

}

Subsystem* create_full_adder_standard(char *name, char **inputs, int inputc,\
                                      char **outputs, int outputc, int nbits,
                                      Standard *single_bit_std) {

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

    return new;

}
