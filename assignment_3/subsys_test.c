#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "netlist.h"
#include "str_util.h"

#define GATE_LIB_NAME "component.lib"
#define SUBSYS_LIB_NAME "subsystem.lib"
#define SIZE 100

#define A "A0"
#define B "B0"
#define C "Cin"

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


    char *a[] = {A,B,C};
    for(int i=0; i<3; i++) printf("%s\n", a[i]);

    Subsystem *custom_FA = malloc(sizeof(Subsystem));
    int next = create_custom(custom_FA, lib->contents->std, 3, a, 7);

    c = custom_FA->components;

    while (c != NULL) {

        Component *comp = c->comp;
    
        printf("\tU%d: %s,", comp->id, comp->prototype->gate->name);
        printf(" defined in %s", comp->prototype->defined_in->file);
        printf(", with %d inputs ", comp->prototype->gate->_inputc);

        for (int i=0; i<comp->prototype->gate->_inputc; i++)  {
            printf("%s ", comp->prototype->gate->inputs[i]);
        }

        printf("mapped to ");
        for (int i=0; i<comp->_inputc; i++) {
            printf("%s ", comp->inputs[i]);
        }
        printf("respectively\n");

        c = c->next;

    }

    // print the output mappings
    printf("and the following output mappings:\n");

    for(int i=0; i<custom_FA->_outputc; i++) {
        printf("\t%s <- %s\n", custom_FA->outputs[i], custom_FA->output_mappings[i]);
    }

    printf("next component will be U%d\n", next);
    printf("\n");



    free_subsystem(custom_FA);
    free_lib(lib);
    free_lib(gate_lib);
    free(buf);
    return 0;
}

