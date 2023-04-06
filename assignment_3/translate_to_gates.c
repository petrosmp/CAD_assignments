#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "netlist.h"
#include "str_util.h"

#define GATE_LIB_NAME   "component.lib"
#define SUBSYS_LIB_NAME "subsystem.lib"
#define NETLIST_NAME    "netlist5.txt"
#define OUTPUT_FILE     "gates_only.txt"

int main() {

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
    // read the netlist where the n-bit full adder is described
    Netlist *netlist = malloc(sizeof(Netlist));
    if (subsys_lib_from_file(NETLIST_NAME, netlist, lib)) {
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
    lib_to_file(only_gates_lib, OUTPUT_FILE, "w");

    // cleanup
    free_lib(only_gates_lib);
    free_lib(netlist);
    free_lib(lib);
    free_lib(gate_lib);

    printf("Program executed successfully\n");
    return 0;
}
