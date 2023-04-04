#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "netlist.h"
#include "str_util.h"

#define GATE_LIB_NAME "component.lib"
#define SUBSYS_LIB_NAME "subsystem.lib"
#define NETLIST_NAME "netlist.txt"
#define OUTPUT_FILE "only_gates.txt"
#define SIZE 100


int main() {


    Library *gate_lib = malloc(sizeof(Library));
    gate_lib_from_file(GATE_LIB_NAME, gate_lib);

    Library *lib = malloc(sizeof(Library));
    subsys_lib_from_file(SUBSYS_LIB_NAME, lib, gate_lib);

    Library *netlist = malloc(sizeof(Library));
    subsys_lib_from_file(NETLIST_NAME, netlist, lib);

    // node ptr is the node of FULL_ADDER5
    // create a subsystem that is just like the one that we are reading
    Library *only_gates_lib = malloc(sizeof(Library));
    netlist_to_gate_only(only_gates_lib, netlist, 1);

    lib_to_file(only_gates_lib, OUTPUT_FILE, "w");

    printf("Program executed successfully\n");

    free_lib(netlist);
    free_lib(lib);
    free_lib(gate_lib);

    return 0;
}
