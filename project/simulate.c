/**
 * @file    simulate.c
 * 
 * @author  Petros Bimpiris (pbimpiris@tuc.gr)
 * 
 * @brief   A tool to simulate the behavior of a circuit using testbenches.
 * 
 * @version 1.0
 * 
 * @date 11-06-2023
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "str_util.h"
#include "netlist.h"
#include <time.h>

#define GATE_LIB_NAME       "component.lib"
#define INPUT_FILE          "subsystem.lib"
#define OUTPUT_FILE         "testbench_out.txt"
#define SUBSYSTEM_NAME      "FULL_ADDER5"
#define TESTBENCH_FILE      "testbench.txt"

void usage();

int main(int argc, char *argv[]) {

    char ch, *gate_lib_name = GATE_LIB_NAME, *input_file = INPUT_FILE, *output_file = OUTPUT_FILE, *subsys_name = SUBSYSTEM_NAME, *tb_file = TESTBENCH_FILE;

    // parse any (optional) arguments
    while ((ch = getopt(argc, argv, "g:i:o:t:s:h")) != -1) {
		switch (ch) {		
			case 'g':
				gate_lib_name = optarg;
				break;
			case 'i':
				input_file = optarg;
				break;
            case 'o':
				output_file = optarg;
				break;
			case 't':
				tb_file = optarg;
				break;
            case 's':
                subsys_name = optarg;
                break;
            case 'h':
            default:
				usage();
                exit(0);
		}
	}

    // parse the component library where the gates that may be used are defined
    Netlist *gate_lib = malloc(sizeof(Netlist));
    if (gate_lib_from_file(gate_lib_name, gate_lib)) {
        fprintf(stderr, "There was an error, the program terminated abruptly!\n");
        return -1;
    }
    
    // read the netlist where the input circuit is described
    Netlist *input = malloc(sizeof(Netlist));
    if (subsys_lib_from_file(input_file, input, gate_lib)) {
        fprintf(stderr, "There was an error, the program terminated abruptly!\n");
        return -1;
    }

    // find the subsystem that will be simulated
    Subsystem *s = find_in_lib(input, subsys_name)->subsys;

    // initialize the testbench structure
    Testbench *tb = malloc(sizeof(Testbench));
    tb->uut = s;

    // start a clock
    clock_t start = clock();  // measure the total time - include the parsing of the file

    // parse the testbench data from the file
    if ( parse_tb_from_file(tb, tb_file, "r") ) {
        fprintf(stderr, "There was an error, the program terminated abruptly!\n");
        return -1;
    };

    // execute the testbench
    if ( execute_tb(tb, output_file, "w") ) {
        fprintf(stderr, "There was an error while executing the testbench, the program terminated abruptly!\n");
        return -1;
    };

    // stop the clock and measure the time
    clock_t end = clock();
    double time_taken = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Total testbench execution time (including parsing): %.3f msec\n", time_taken*1000);
    

    // cleanup
    free_tb(tb);
    free_lib(gate_lib);
    free_lib(input);
    printf("Program executed successfully\n");

    return 0;
}

void usage() {
    printf("Usage: ./simulate [<option> <argument>]\n");
    printf("Available options:\n");
    printf("\t-g <filename>:\tuse the file with the given name as the component (gate) library (default %s)\n", GATE_LIB_NAME);
    printf("\t-i <filename>:\tuse the file with the given name as the input netlist (default %s)\n", INPUT_FILE);
    printf("\t-o <filename>:\twrite the output to a file with the given name (will be overwritten if it already exists) (default %s)\n", OUTPUT_FILE);
    printf("\t-t <filename>:\tuse the file with the given name as the testbench file (default %s)\n", TESTBENCH_FILE);
    printf("\t-s <name>:\tfind and simulate the subsystem with the given name (must be contained in the specified netlist) (default %s)\n", SUBSYSTEM_NAME);
}
