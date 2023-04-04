#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "netlist.h"
#include "str_util.h"

#define GATE_LIB_NAME "component.lib"
#define SUBSYS_LIB_NAME "subsystem.lib"
#define NETLIST_NAME "netlist5.txt"
#define OUTPUT_FILE "gates_only.txt"
#define SIZE 100


int main() {

    // read the component library where the gates are defined
    Library *gate_lib = malloc(sizeof(Library));
    if (gate_lib_from_file(GATE_LIB_NAME, gate_lib)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }
    // read the subsystem library where the single-bit full adder is defined
    Library *lib = malloc(sizeof(Library));
    if (subsys_lib_from_file(SUBSYS_LIB_NAME, lib, gate_lib)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }
    // read the netlist where the n-bit full adder is described
    Library *netlist = malloc(sizeof(Library));
    if (subsys_lib_from_file(NETLIST_NAME, netlist, lib)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }
    // create a netlist that contains everything the one above does, but implemented only with gates
    Library *only_gates_lib = malloc(sizeof(Library));
    if (netlist_to_gate_only(only_gates_lib, netlist, 1)) {
        printf("There was an error, the program terminated abruptly!\n");
        return -1;
    }

    // print the gates-only netlist to the file
    lib_to_file(only_gates_lib, OUTPUT_FILE, "w");

    // cleanup
    free_lib(only_gates_lib);
    free_lib(netlist);
    free_lib(lib);
    free_lib(gate_lib);

    printf("Program executed successfully\n");
    return 0;
}
