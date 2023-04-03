#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "netlist.h"
#include "str_util.h"

#define GATE_LIB_NAME "component.lib"
#define SUBSYS_LIB_NAME "subsystem.lib"
#define SIZE 100

int main() {


    Library *gate_lib = malloc(sizeof(Library));
    gate_lib_from_file(GATE_LIB_NAME, gate_lib);

    Library *lib = malloc(sizeof(Library));
    subsys_lib_from_file(SUBSYS_LIB_NAME, lib, gate_lib);

    // print the contents of the library (assuming that its made of gates only)
    char *buf = malloc(SIZE);
    Node *c = lib->contents;
    while (c!=NULL) {
        Subsystem *c2 = c->std->subsys;

        // print the components
        printf("%s has the following components:\n", c2->name);

        Node *_comp = c2->components;
        while (_comp != NULL) {

            Component *comp = _comp->comp;
    
            printf("\t%s,", comp->prototype->gate->name);
            printf(" defined in %s", comp->prototype->defined_in->file);
            printf(", with %d inputs ", comp->prototype->gate->_inputc);

            for (int i=0; i<comp->prototype->gate->_inputc; i++)  {
                printf("%s ", comp->prototype->gate->inputs[i]);
            }

            printf("mapped to ");
            for (int i=0; i<comp->_inputc; i++) {
                printf("%s (%c, %d) ", comp->inputs[i], comp->i_maps[i]->type==SUBSYS_INPUT?'I':'C', comp->i_maps[i]->index);
            }
            printf("respectively\n");

            _comp = _comp->next;

        }

        // print the output mappings
        printf("and the following output mappings:\n");

        for(int i=0; i<c2->_outputc; i++) {
            printf("\t%s <- %s (%c, %d)\n", c2->outputs[i], c2->output_mappings[i], c2->o_maps[i]->type==SUBSYS_INPUT?'I':'C', c2->o_maps[i]->index);
        }


        printf("\n");
        c = c->next;

    }


    free_lib(lib);
    free_lib(gate_lib);
    free(buf);
    return 0;
}

