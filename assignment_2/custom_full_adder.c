#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "netlist.h"

#define SUBSYSTEM_LIB_FILENAME "subsystem.lib"
#define OUTPUT_FILENAME "netlist.txt"
#define MAX_INPUT_LEN 10

int create_nbit_full_adder(int n, Subsystem **n_bit_fa, Subsystem *reference);

int main() {

    // read the number of bits that the user wants
    int nbits = 0;
    printf("Please enter number of bits (1-8): ");
    scanf("%d", &nbits);

    // check that the value is valid
    if (nbits<1 || nbits>8) {
        printf("The number of bits must be between 1 and 8!\n");
        exit(1);
    }

    // load the single bit full adder reference from the library
    Subsystem *FA_reference = malloc(sizeof(Subsystem)); 
    if ( read_ref_subsystem_from_file(SUBSYSTEM_LIB_FILENAME, &FA_reference) ) {
        fprintf(stderr, "There was an error while reading the library file!\n");
        free_subsystem(FA_reference);
        exit(1);
    }

    // create the n-bit full adder (functional) subsystem
    Subsystem *nbit_full_adder = malloc(sizeof(Subsystem));
    if ( create_nbit_full_adder(nbits, &nbit_full_adder, FA_reference) ) {
        fprintf(stderr, "There was an error while creating the %d-bit full adder!\n", nbits);
        free_subsystem(FA_reference);
        free_subsystem(nbit_full_adder);
        exit(1);
    }

    // write the netlist to the file
    if ( netlist_to_file(nbit_full_adder, OUTPUT_FILENAME, "w") ) {
        fprintf(stderr, "There was an error while writing the %d-bit full adder netlist to the file!\n", nbits);
        free_subsystem(FA_reference);
        free_subsystem(nbit_full_adder);
        exit(1);
    }

    printf("Netlist file created successfully: %s\n", OUTPUT_FILENAME);

    // cleanup
    free_subsystem(nbit_full_adder);
    free_subsystem(FA_reference);

    return 0;
}

/**
 * Create an n-bit full adder functional subsystem.
 * 
 * Takes as arguments the number of bits (n), a subsystem that will be
 * "turned into" the n-bit full adder and a single bit full adder for
 * reference.
 * 
 * The memory for the n-bit full adder is assumed to be allocated. Anything
 * else (the memory for each component and its members) will be dynamically
 * allocated as needed. It has to be freed by the caller.
 * 
*/
int create_nbit_full_adder(int n, Subsystem **n_bit_fa, Subsystem *reference) {
    
    /*
        Set the fields of the n-bit full adder
    */

    // set the name
    (*n_bit_fa)->name = malloc(20);
    snprintf((*n_bit_fa)->name, 20, "FULL_ADDER%d", n);
    
    // allocate memory for the inputs...
    (*n_bit_fa)->_inputc = 2*n+1;
    (*n_bit_fa)->inputs = malloc(sizeof(char*) * (*n_bit_fa)->_inputc);

    // ...and set the inputs
    for (int i=0; i<n; i++) {
        (*n_bit_fa)->inputs[i] = malloc(MAX_INPUT_LEN);
        snprintf((*n_bit_fa)->inputs[i], MAX_INPUT_LEN, "A%d", n-i-1);
    }
    for (int i=n; i<2*n; i++) {
        (*n_bit_fa)->inputs[i] = malloc(MAX_INPUT_LEN);
        snprintf((*n_bit_fa)->inputs[i], MAX_INPUT_LEN, "B%d", n-(i-n)-1);
    }
    (*n_bit_fa)->inputs[2*n] = malloc(MAX_INPUT_LEN);
    snprintf((*n_bit_fa)->inputs[2*n], strlen("CIN")+1, "CIN");

    // allocate memory for the outputs...
    (*n_bit_fa)->_outputc = n+1;
    (*n_bit_fa)->outputs = malloc(sizeof(char*) * (*n_bit_fa)->_outputc);

    // ...and set the outputs
    for (int i=0; i<n; i++) {
        (*n_bit_fa)->outputs[i] = malloc(MAX_INPUT_LEN);
        snprintf((*n_bit_fa)->outputs[i], MAX_INPUT_LEN, "S%d", n-i-1);
    }
    (*n_bit_fa)->outputs[n] = malloc(MAX_INPUT_LEN);
    snprintf((*n_bit_fa)->outputs[n], strlen("COUT")+1, "COUT");

    // set the type
    (*n_bit_fa)->type = FUNCTIONAL;

    // create and set the components
    (*n_bit_fa)->_componentc = n;
    (*n_bit_fa)->components = malloc(sizeof(Component*) * (*n_bit_fa)->_componentc);

    for (int i=0; i<(*n_bit_fa)->_componentc; i++) {

        // allocate memory for each component
        (*n_bit_fa)->components[i] = malloc(sizeof(Component));

        // set the standard for each component
        (*n_bit_fa)->components[i]->standard = reference;

        // set each component's id as a part of the subsystem
        (*n_bit_fa)->components[i]->id = i+1;

        // set the input mappings of each component:

        // 1) allocate memory for the list
        (*n_bit_fa)->components[i]->_inputc = (*n_bit_fa)->components[i]->standard->_inputc;
        (*n_bit_fa)->components[i]->inputs = malloc(sizeof(char*) * (*n_bit_fa)->components[i]->standard->_inputc);

        // 2) allocate memory for each input mapping
        for (int j=0; j<(*n_bit_fa)->components[i]->standard->_inputc; j++) {
            (*n_bit_fa)->components[i]->inputs[j] = malloc(MAX_INPUT_LEN);
        }

        // 3) set each input mapping
        snprintf((*n_bit_fa)->components[i]->inputs[0], MAX_INPUT_LEN, "A%d", i);
        snprintf((*n_bit_fa)->components[i]->inputs[1], MAX_INPUT_LEN, "B%d", i);
            
        if (i==0) {
            // for the first full adder, Cin is the circuit's Cin
            snprintf((*n_bit_fa)->components[i]->inputs[2], MAX_INPUT_LEN, "CIN");
        } else {
            // for the rest, Cin is the Cout of the previous full adder
            snprintf(
                (*n_bit_fa)->components[i]->inputs[2],
                MAX_INPUT_LEN, "%s%d_%s",
                COMP_ID_PREFIX, (*n_bit_fa)->components[i-1]->id, (*n_bit_fa)->components[i-1]->standard->outputs[1]
            );
        }
    }
    
    // allocate memory and set the output mappings of the subsystem
    (*n_bit_fa)->output_mappings = malloc(sizeof(char*) * (*n_bit_fa)->_outputc);

    for (int i=0; i<(*n_bit_fa)->_outputc-1; i++) {
        (*n_bit_fa)->output_mappings[i] = malloc(MAX_INPUT_LEN);
        snprintf((*n_bit_fa)->output_mappings[i], MAX_INPUT_LEN, "U%d_S", n-i);
    }
    (*n_bit_fa)->output_mappings[n] = malloc(MAX_INPUT_LEN);
    snprintf((*n_bit_fa)->output_mappings[n], MAX_INPUT_LEN, "U%d_COUT", (*n_bit_fa)->_componentc);

    return 0;
}
