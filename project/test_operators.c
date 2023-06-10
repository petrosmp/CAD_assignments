#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "str_util.h"
#include "netlist.h"

#define INPUTS 10
#define GATE_LIB_NAME       "component.lib"
#define SUBSYS_LIB_NAME     "subsystem.lib"
#define INPUT_FILE          "input1.txt"
#define OUTPUT_FILE         "output1.txt"
#define SINGLE_BIT_FA_NAME  "FULL_ADDER"
#define SINGLE_BIT_FAS_NAME "FULL_ADDER_SUBTRACTOR"

int read_libs(char *gate_lib_name, Netlist *gate_lib, char *subsys_lib_name, Netlist *subsys_lib);
void usage();

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


    // print the libraries
    lib_to_file(gate_lib, "filename_gates.txt", "w");

    lib_to_file(lib, "filename_subsystems.txt", "w");

    // print the libraries
    lib_to_file_debug(gate_lib, "filename_gates_debug.txt", "w");

    lib_to_file_debug(lib, "filename_subsystems_debug.txt", "w");


    
    // read the netlist where the n-bit full adder is described
    Netlist *netlist = malloc(sizeof(Netlist));
    if (subsys_lib_from_file("netlist5.txt", netlist, lib)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }

    lib_to_file_debug(netlist, "preza", "w");



    // create a netlist that contains everything the one above does, but implemented only with gates
    Netlist *only_gates_lib = malloc(sizeof(Netlist));
    if (netlist_to_gate_only(only_gates_lib, netlist, 1)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }

    // print the gates-only netlist to the file
    netlist_to_file(only_gates_lib, output_file, "w");


    // parse the new lib
    // read the netlist where the n-bit full adder is described
    Netlist *only = malloc(sizeof(Netlist));
    if (subsys_lib_from_file(output_file, only, gate_lib)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }

    lib_to_file_debug(only, "only.txt", "w");



    // cleanup
    free_lib(lib);
    free_lib(gate_lib);
    free_lib(netlist);
    free_lib(only_gates_lib);
    free_lib(only);
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
