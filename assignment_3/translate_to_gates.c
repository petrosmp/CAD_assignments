#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "netlist.h"
#include "str_util.h"
#include <getopt.h>

#define GATE_LIB_NAME   "component.lib"
#define SUBSYS_LIB_NAME "subsystem.lib"
#define NETLIST_NAME    "netlist5.txt"
#define OUTPUT_FILE     "gates_only.txt"

void usage();

int main(int argc, char *argv[]) {

    char ch;
    char* gate_lib_name = GATE_LIB_NAME;
    char* subsys_lib_name = SUBSYS_LIB_NAME;
    char* netlist_name = NETLIST_NAME;
    char* output_file = OUTPUT_FILE;

    // parse any (optional) arguments
    while ((ch = getopt(argc, argv, "g:s:n:o:")) != -1) {
		switch (ch) {		
			case 'g':
				gate_lib_name = optarg;
				break;
			case 's':
				subsys_lib_name = optarg;
				break;
			case 'n':
				netlist_name = optarg;
				break;
            case 'o':
				output_file = optarg;
				break;
			default:
				usage();
                exit(0);
		}
	}

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
    // read the netlist where the n-bit full adder is described
    Netlist *netlist = malloc(sizeof(Netlist));
    if (subsys_lib_from_file(netlist_name, netlist, lib)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }
    // create a netlist that contains everything the one above does, but implemented only with gates
    Netlist *only_gates_lib = malloc(sizeof(Netlist));
    if (netlist_to_gate_only(only_gates_lib, netlist, 1)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }

    // print the gates-only netlist to the file
    lib_to_file(only_gates_lib, output_file, "w");

    // cleanup
    free_lib(only_gates_lib);
    free_lib(netlist);
    free_lib(lib);
    free_lib(gate_lib);

    printf("Program executed successfully\n");
    return 0;
}

void usage() {
    printf("Usage: ./translate_to_gates <option1> <argument1> ... <optionX> <argument<X>\n");
    printf("Available options:\n");
    printf("\t-g <filename>:\tuse the file with the given name as the component (gate) library (default %s)\n", GATE_LIB_NAME);
    printf("\t-s <filename>:\tuse the file with the given name as the subsystem library (default %s)\n", SUBSYS_LIB_NAME);
    printf("\t-n <filename>:\tuse the file with the given name as the input netlist (default %s)\n", NETLIST_NAME);
    printf("\t-o <filename>:\twrite the output to a file with the given name (will be overwritten if it already exists) (default %s)\n", OUTPUT_FILE);
}
