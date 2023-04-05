#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "netlist.h"
#include "str_util.h"

#define FILENAME "component.lib"
#define SIZE 100

int main() {

    Netlist *lib = malloc(sizeof(Netlist));

    gate_lib_from_file(FILENAME, lib);

    char *buf = malloc(SIZE);

    Node *cur = lib->contents;
    while (cur != NULL) {
        printf("[%s] ", cur->std->gate->name);
        
        memset(buf, 0, SIZE);
        int b_w=0;
        write_list_at(cur->std->gate->_inputc, cur->std->gate->inputs, IN_OUT_DELIM, buf, 0, SIZE, &b_w);
        printf("[%s] ", buf);
        printf("(read from [%s])\n", cur->std->defined_in->file);
        cur=cur->next;
    }

    free(buf);
    free_lib(lib);

    return 0;
}
