#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "str_util.h"
#include "netlist.h"

#define INPUTS 10
#define GATE_LIB_NAME       "component.lib"
#define SUBSYS_LIB_NAME     "subsystem.lib"
#define INPUT_FILE          "full_adder_5.txt"
#define OUTPUT_FILE         "output1.txt"
#define SINGLE_BIT_FA_NAME  "FULL_ADDER"
#define SINGLE_BIT_FAS_NAME "FULL_ADDER_SUBTRACTOR"
#define INPUT_DELIM         ", "
#define TESTBENCH_IN        "IN"
#define TESTBENCH_OUT       "OUT"
#define TB_GENERAL_DELIM    " "
#define TB_IN_VAL_DELIM     ", "


void usage();
int simulate(Subsystem* s, char *inputs);
int parse_tb_from_file(Testbench *tb, char *filename, char *mode);

int main(int argc, char *argv[]) {

    char ch, *gate_lib_name = GATE_LIB_NAME, *input_name = INPUT_FILE;

    // parse any (optional) arguments
    while ((ch = getopt(argc, argv, "g:s:i:o:")) != -1) {
		switch (ch) {		
			case 'g':
				gate_lib_name = optarg;
				break;
			case 's':
				//subsys_lib_name = optarg;
				break;
			case 'i':
				input_name = optarg;
				break;
            case 'o':
				//output_file = optarg;
				break;
			default:
				usage();
                exit(0);
		}
	}

    // parse the component library where the gates that may be used are defined
    Netlist *gate_lib = malloc(sizeof(Netlist));
    if (gate_lib_from_file(gate_lib_name, gate_lib)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }
    
    // read the netlist where the n-bit full adder is described
    Netlist *only = malloc(sizeof(Netlist));
    if (subsys_lib_from_file(input_name, only, gate_lib)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }

    lib_to_file_debug(only, "only.txt", "w");

    Subsystem *s = find_in_lib(only, "FULL_ADDER5")->subsys;

    char *inpts = "0, 0, 1, 0, 1, "     // A4 to A0 (5)
                   "1, 1, 0, 1, 0, "    // B4 to B0 (26)
                   "0";                 // Cin (0)

    char *inputs = malloc(strlen(inpts)+1);
    strncpy(inputs, inpts, strlen(inpts)+1);

    if ( simulate(s, inputs) ) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }

    free(inputs);

    Testbench *tb = malloc(sizeof(Testbench));
    tb->uut = s;

    parse_tb_from_file(tb, "testbench.txt", "r");


    // cleanup
    free(tb);
    free_lib(gate_lib);
    free_lib(only);
    printf("Program executed successfully\n");

    return 0;
}


int simulate(Subsystem* s, char *inputs) {


    if (s == NULL || inputs==NULL) {
        return NARG;
    }

    // parse the inputs into a list
    char **l = NULL;
    int ic = str_to_list(inputs, &l, INPUT_DELIM);

    // check that the number of inputs is the expected one
    if (ic != s->_inputc) {
        fprintf(stderr, "simulation error: got %d inputs, expected %d\n", ic, s->_inputc);
        return GENERIC_ERROR;
    }

    int *int_inputs = malloc(sizeof(int)*s->_inputc);

    // make the inputs integers (so that they are usable in the simulation)
    for (int i=0; i<s->_inputc; i++) {
        
        // only check the first byte (suffices if everything is done right)
        char c = l[i][0];

        // check that it is a bit
        if (c!='1' && c !='0') {
            fprintf(stderr, "unexpected (non-bit) value found for input %d: '%c'\n", i+1, c);
            return GENERIC_ERROR;
        }

        // convert to int
        int_inputs[i] = c - '0';
    }


    // find out how large the buffers need to be (#components + #outputs)
    int max_component_index = 0;
    for(Node *n=s->components->head; n!=NULL; n=n->next) {
        if (n->comp->buffer_index > max_component_index) {
            max_component_index = n->comp->buffer_index;
        }
    }

    // initialize the buffers
    int *buffer1 = malloc(sizeof(int) * ((max_component_index+1) + s->_outputc));
    int *buffer2 = malloc(sizeof(int) * ((max_component_index+1) + s->_outputc));

    memset(buffer1, 0, sizeof(int)*((max_component_index+1) + s->_outputc));
    memset(buffer2, 0, sizeof(int)*((max_component_index+1) + s->_outputc));

    int *old = buffer1;
    int *new = buffer2;

    // dirty flag (whether something changed this iteration or not - this is how we know when to break)
    int dirty = 1;

    int iterations = 0;

    while(dirty) {

        // unset the dirty flag so that it is only set if something changes
        dirty = 0;

        // increment the iteration counter
        iterations++;

        // iterate through the components
        for(Node *n=s->components->head; n!=NULL; n=n->next) {

            // cache a reference to the component for easy access
            Component *comp = n->comp;

            // if it is not a gate something is wrong
            if (comp->prototype->type != GATE) {
                fprintf(stderr, "unexpected component type! we can only simulate if all components are gates\n");
                return GENERIC_ERROR;
            }

            // find out the input values

            // clear the space where they will be stored
            char comp_ins[(comp->prototype->gate->_inputc) + 1];
            memset(comp_ins, 0, (comp->prototype->gate->_inputc) + 1);

            // get them from the old buffer according to the component's mappings
            for (int i=0; i<comp->prototype->gate->_inputc; i++) {

                // get the mapping for each input
                Mapping *m = comp->i_maps[i];

                if (m->type == SUBSYS_INPUT) {
                    
                    // this is the easy case, inputs dont change so just get the value from there
                    comp_ins[i] = int_inputs[m->index] + '0';

                } else if (m->type == SUBSYS_COMP) {

                    // find the component
                    Component *rtc = move_in_list(m->index, s->components)->comp; // referred-to component node
                    
                    // get its index in the buffer
                    int rtc_buf_index = rtc->buffer_index;
                    
                    // get its old value
                    comp_ins[i] = old[rtc_buf_index] + '0';
                } else {

                    // the mapping is neither to an input or a component - should never happen
                    fprintf(stderr, "unexpected error! found mapping that does not map to an input or component\n");
                    return GENERIC_ERROR;
                }

            }

            // find the truth value of the component with the retrieved inputs
            int new_val = eval_at(comp->prototype->gate->truth_table, comp_ins);

            // only replace the value if it differs from the old one, and also set the dirty flag
            if (new_val != new[comp->buffer_index]) {
                new[comp->buffer_index] = new_val;
                dirty = 1;
            }


        }


            // also iterate through the outputs
            for (int i=0; i<s->_outputc; i++) {
                
                int new_val;

                // get the mapping
                Mapping *m = s->o_maps[i];

                if (m->type == SUBSYS_INPUT) {
                    
                    // this is the easy case, inputs dont change so just get the value from there
                    new_val = int_inputs[m->index] + '0';

                } else if (m->type == SUBSYS_COMP) {

                    // find the component
                    Component *rtc = move_in_list(m->index, s->components)->comp; // referred-to component node
                    
                    // get its index in the buffer
                    int rtc_buf_index = rtc->buffer_index;
                    
                    // get its old value
                    new_val = old[rtc_buf_index];

                } else {

                    // the mapping is neither to an input or a component - should never happen
                    fprintf(stderr, "unexpected error! found mapping that does not map to an input or component\n");
                    return GENERIC_ERROR;
                }

                // only replace the value if it differs from the old one, and also set the dirty flag
                if (new_val != new[max_component_index+1+i]) {
                    new[max_component_index+1+i] = new_val;
                    dirty = 1;
                }


            }
        

        // swap the buffers
        int *tmp = old;
        old = new;
        new = tmp;


    }

    fprintf(stderr, "Simulation over after %d iterations. outputs shown below:\n", iterations);

    for(int i=0; i<s->_outputc; i++) {
        fprintf(stderr, "%s = %d\n", s->outputs[i], old[max_component_index+1+i]);
    }


    // cleanup
    free(int_inputs);
    free_str_list(l, ic);
    free(buffer1);
    free(buffer2);

    return 0;
}

void usage() {
    printf("Usage: ./simulate [<option> <argument>]\n");
    printf("Available options:\n");
    printf("\t-g <filename>:\tuse the file with the given name as the component (gate) library (default %s)\n", GATE_LIB_NAME);
    printf("\t-s <filename>:\tuse the file with the given name as the subsystem library (default %s)\n", SUBSYS_LIB_NAME);
    printf("\t-i <filename>:\tuse the file with the given name as the input netlist (default %s)\n", INPUT_FILE);
    printf("\t-o <filename>:\twrite the output to a file with the given name (will be overwritten if it already exists) (default %s)\n", OUTPUT_FILE);
}

/**
 * @brief   Parse the information that describes a testbench from the given file into the
 *          given structure.
 * 
 * @note    This does not allocate memory for the testbench struct itself, and it requires
 *          that the uut has already been set. If not, an error value will be returned.
 *          
 *          It does however allocate memory for the values array, which will need to be
 *          properly freed.
 *  
 * 
 * @param tb        The structure where the data will be saved
 * @param filename  The file that will be parsed
 * @param mode      The mode with which fopen() will be called
 * @return 0 on success, 1 on failure.
 */
int parse_tb_from_file(Testbench *tb, char *filename, char *mode) {

    FILE *fp = fopen(filename, mode);

    char *line = NULL;  // no need to malloc it, getline() does that for us (and also reallocs if needed) 
    int line_no = 0;
    int nread = 0;
    int offset = 0;
    size_t len = 0;
    int _en;

    // initialize the values field of the testbench struct
    tb->values = malloc(sizeof(char**) * tb->uut->_inputc);
    tb->outs_display = malloc(sizeof(int)*tb->uut->_outputc);
    memset(tb->outs_display, 0, sizeof(int)*tb->uut->_outputc);

    tb->v_c = -1;

    // loop through the lines of the file and get the contents
    while((nread = read_line_from_file(&line, filename, &len, offset)) != -1) {
        
        line_no++;
        offset += nread;

        if (strlen(line) != 0) {
            
            if (starts_with(line, TESTBENCH_IN)) {

                while (1) {

                    // read the next line
                    if ( (nread = read_line_from_file(&line, filename, &len, offset)) == -1 ) {
                        printf("%s:%d: Error! File ended unexpectedly\n",filename, line_no);
                        return UNEXPECTED_EOF;
                    }
                    line_no++;
                    offset += nread;

                    // check if we need to stop
                    if (starts_with(line, TESTBENCH_OUT)) break;

                    // keep a reference to be able to split
                    char *_line = line;

                    // split the line into its contents
                    char *name = split(&_line, TB_GENERAL_DELIM);
                    char *vals = _line;


                    int in_index = -1;
                    if ( (in_index = contains(tb->uut->_inputc, tb->uut->inputs, name)) == -1 ) {
                        fprintf(stderr, "unknown input in testbench! uut of type %s has no input named %s\n", tb->uut->name, name);
                        return GENERIC_ERROR;
                    }


                    // put the given values into a list
                    tb->values[in_index] = NULL;
                    int v_c = str_to_list(vals, &(tb->values[in_index]), TB_IN_VAL_DELIM);

                    if (tb->v_c == -1 || v_c < tb->v_c) {
                        tb->v_c = v_c;
                    }

                    printf("found line that gives input named %s (index %d) the following values %s (%d in total)\n", name, in_index, vals, v_c);

                }
                
                // we here when tb out has been found
                // read the next lines that contain the outputs we wanna see
            
                fprintf(stderr, "broke\n");
            } 
            
            if (starts_with(line, TESTBENCH_OUT)) {

                while ((nread = read_line_from_file(&line, filename, &len, offset)) != -1) {
                    
                    line_no++;
                    offset += nread;


                    int out_index = -1;
                    if ( (out_index = contains(tb->uut->_outputc, tb->uut->outputs, line)) == -1 ) {
                        fprintf(stderr, "unknown input in testbench! uut of type %s has no input named '%s'\n", tb->uut->name, line);
                        return GENERIC_ERROR;
                    }

                    fprintf(stderr, "found line that indicates we should display output %d\n", out_index);

                    tb->outs_display[out_index] = 1;

                }
            }

        }
    }


    free(line);
    fclose(fp);

    for (int i=0; i<tb->uut->_inputc; i++) {
        fprintf(stderr, "values for %s: [ ", tb->uut->inputs[i]);
    
        for (int j=0; j<tb->v_c; j++) {
            fprintf(stderr, "%s, ", tb->values[i][j]);
        }

        fprintf(stderr, " ]\n");
    }

    for(int i=0; i<tb->uut->_outputc; i++) {
        fprintf(stderr, "output %s will be displayed: [%d]\n", tb->uut->outputs[i], tb->outs_display[i]);
    }

    return 0;
}
