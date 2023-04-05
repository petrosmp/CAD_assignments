#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "netlist.h"
#include "str_util.h"

#define FILENAME "component.lib"
#define SIZE 100

int main(int argc, char *argv[]) {

    Netlist *lib = malloc(sizeof(Netlist));
    gate_lib_from_file(FILENAME, lib);

    Standard *a = search_in_lib(lib, argv[1]);

    if (a == NULL) {
        printf("Not found\n");
    } else {
        char *buf = malloc(SIZE);
        printf("Found standard defined in %s:\n", a->defined_in->file);
        if (a->type == GATE) {
            gate_to_str(a->gate, buf, SIZE);
        } else {
            printf("Something is wrong\n");
        }
        printf("%s\n", buf);
        free(buf);

    }

    free_lib(lib);

    return 0;
}

