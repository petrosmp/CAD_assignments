#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "netlist.h"
#include "str_util.h"

#define GATE_LIB_NAME "component.lib"
#define SUBSYS_LIB_NAME "subsystem.lib"
#define NETLIST_NAME "netlist.txt"
#define SIZE 100

#define A "A0"
#define B "B0"
#define C "Cin"

int main() {


    Library *gate_lib = malloc(sizeof(Library));
    gate_lib_from_file(GATE_LIB_NAME, gate_lib);

    Library *lib = malloc(sizeof(Library));
    subsys_lib_from_file(SUBSYS_LIB_NAME, lib, gate_lib);

    Library *lib2 = malloc(sizeof(Library));
    subsys_lib_from_file(NETLIST_NAME, lib2, lib);

/*     // print the contents of the library
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

    // print the contents of the netlist
    c = lib2->contents;
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

 */
    char *a[] = {A,B,C};
    // for(int i=0; i<3; i++) printf("%s\n", a[i]);



    // node ptr is the node of FULL_ADDER5
    int component_id = 1;
    Node *node_ptr = lib2->contents;
    
    while (node_ptr!=NULL) {

        // C2 IS FULL_ADDER5
        Subsystem *c2 = node_ptr->std->subsys;

        // count the compponents in the subsystem
        int subsys_count = 0;
        Node *_comp = c2->components;
        while (_comp != NULL) {
            _comp = _comp->next;
            subsys_count++;
        }

        // create a subsystem that is just like the one that we are reading
        Subsystem *only_gates_sub = malloc(sizeof(Subsystem));

        only_gates_sub->is_standard = 0;
        only_gates_sub->name = c2->name;
        only_gates_sub->_inputc = c2->_inputc;
        only_gates_sub->inputs = malloc(sizeof(char*) * c2->_inputc);
        only_gates_sub->_outputc = c2->_outputc;
        only_gates_sub->outputs = malloc(sizeof(char*) * c2->_outputc);
        for(int i=0; i<c2->_inputc; i++){
            only_gates_sub->inputs[i] = malloc(strlen(c2->inputs[i])+1);
            strncpy(only_gates_sub->inputs[i], c2->inputs[i], strlen(c2->inputs[i])+1);
        }
        for(int i=0; i<c2->_outputc; i++){
            only_gates_sub->outputs[i] = malloc(strlen(c2->outputs[i])+1);
            strncpy(only_gates_sub->outputs[i], c2->outputs[i], strlen(c2->outputs[i])+1);
        }
        only_gates_sub->components = NULL;

        Subsystem **subsystems = malloc(sizeof(Subsystem*) * subsys_count);

        int s_i = 0;
        // _COMP IS THE NODE THAT CONTAINS FULL_ADDER and has inputs
        _comp = c2->components;

        // FOR EACH COMPONENT OF THE SUBSYSTEM WE WANT TO CALL A CREATE_CUSTOM
        while (_comp != NULL) {

            // COMP IS THE FULL_ADDER THAT WE KNOW FROM THE LIB
            Component *comp = _comp->comp;
            
            // store the inputs to pass them to create_custom()
            char **inputs = malloc(sizeof(char*) * comp->_inputc);

            // iterate over the inputs
            for (int i=0; i<comp->_inputc; i++)  {

                Mapping *map = comp->i_maps[i];

                // if an input is an output of another component
                if (map->type == SUBSYS_COMP) {

                    // find out the component that the mapping maps to
                    Component *cmap = move_in_list(map->index, c2->components)->comp;

                    // assume all components are subsystems (TODO: FIX)
                    if (cmap->prototype->type == SUBSYSTEM) {
                        
                        // then after the UXX_ in inputs[i] there is the name of the output we are interested in
                        // find the name of the output
                        char *o_name = comp->inputs[i]+strlen(COMP_ID_PREFIX)+digits(map->index+1)+1;

                        // find the index of that output in the mapping
                        int index = contains(cmap->prototype->subsys->_outputc, cmap->prototype->subsys->outputs, o_name);
                        if (index == -1) printf("Error!\n");

                        //  %s not found in %s->outputs (which are %d, %s and %s) o_name, cmap->prototype->subsys->name, cmap->prototype->subsys->_outputc, cmap->prototype->subsys->outputs[0], cmap->prototype->subsys->outputs[1]


                        // find the output mapping for that output
                        char *m = subsystems[map->index]->output_mappings[index];

                        // put it in the input list
                        inputs[i] = malloc(strlen(m)+1); // +1 for the null byte
                        strncpy(inputs[i], m, strlen(m)+1);
                    }

                } else if (map->type == SUBSYS_INPUT) {

                    // find the input of the subsystem that the mapping maps to
                    char *m = c2->inputs[map->index];
                    
                    // put it in the input list
                    inputs[i] = malloc(strlen(m)+1); // +1 for the null byte
                    strncpy(inputs[i], m, strlen(m)+1);

                }
                // edw stamataei h epeksergasia tou kathe input

            }
            
/*             fprintf(stderr, "inputs for U%d, %s are the following: ", comp->id, comp->prototype->subsys->name);
            for (int i=0; i<comp->_inputc; i++) {
                fprintf(stderr, "%s ", inputs[i]);
            }
            fprintf(stderr, "\n");
 */
            // we now have a complete list of inputs for each component
            // we need to create the netlist of that component with the inputs we now have
            // and add it to the subsystems array
            subsystems[s_i] = malloc(sizeof(Subsystem));
            component_id = create_custom(subsystems[s_i], comp->prototype, comp->_inputc, inputs, component_id);

            // we can free the inputs here

            for(Node *c_c = subsystems[s_i]->components; c_c!=NULL; c_c=c_c->next) {
                subsys_add_comp(only_gates_sub, c_c->comp);
            }

            s_i++;

            // move on to the next subsystem
            _comp = _comp->next;


        }
        
        // we have now filled the subsystems array with subsystems whose components are gates and numbered consistently

        // now we need to map the outputs of c2 to gate only things
        only_gates_sub->output_mappings = malloc(sizeof(char*) * only_gates_sub->_outputc); // +1 for the null byte

        for(int i=0; i<c2->_outputc; i++) {
            
            Mapping *om = c2->o_maps[i];

            // assuming all mappings are to gates and none is to input
            if (om->type != SUBSYS_COMP) printf("error mapping type!\n");
    
            // find out the component that the mapping maps to, which will now be a subsystem in subsystems
            Subsystem *mapped = subsystems[om->index];

            // assume everything in the subsystems[] array is a subsystem (and not a gate) (TODO: FIX)
                        
            // then after the UXX_ in output_mappings[i] there is the name of the output we are interested in
            // find the name of the output
            char *o_name = c2->output_mappings[i]+strlen(COMP_ID_PREFIX)+digits(om->index+1)+1;

            // find the index of that output in the mapping
            int index = contains(mapped->_outputc, mapped->outputs, o_name);
            if (index == -1) printf("Error!\n");

            // find the output mapping for that output
            char *m = mapped->output_mappings[index];

            // put it in the output list
            only_gates_sub->output_mappings[i] = malloc(strlen(m)+1); // +1 for the null byte
            strncpy(only_gates_sub->output_mappings[i], m, strlen(m)+1);

        }

        // print the stuff
        for (Node *n_n=only_gates_sub->components; n_n!=NULL; n_n=n_n->next) {

                Component *comp = n_n->comp;
            
                printf("U%d %s ", comp->id, comp->prototype->gate->name);

                for (int i=0; i<comp->_inputc; i++) {
                    printf("%s ", comp->inputs[i]);
                }
                printf("\n");
                if(comp->id % 5 == 0) {
                    printf("\n");
                }
        }

        node_ptr = node_ptr->next;

        for (int i=0; i<only_gates_sub->_outputc; i++) {
            printf("%s = %s\n", only_gates_sub->outputs[i], only_gates_sub->output_mappings[i]);
        }

    }



    Subsystem *custom_FA = malloc(sizeof(Subsystem));
/* 
    int next = create_custom(custom_FA, lib->contents->std, 3, a, 7);

    Node *c = custom_FA->components;

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
 */


    free_subsystem(custom_FA);
    free_lib(lib2);
    free_lib(lib);
    free_lib(gate_lib);
    //free(buf);
    return 0;
}

