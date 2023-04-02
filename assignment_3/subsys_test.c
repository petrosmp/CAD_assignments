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
    char *filename = SUBSYS_LIB_NAME;
    char *buf = malloc(SIZE);




    char *line = NULL;  // no need to malloc it, getline() does that for us (and also reallocs if needed) 
    int line_no = 0;
    int pending = 0;


    int nread = 0;
    int offset = 0;
    size_t len = 0;
    int _en;

    // save the filename
    lib->file = malloc(strlen(filename)+1);
    strncpy(lib->file, filename, strlen(filename)+1);

    // initialize the contents list pointer to null
    lib->contents = NULL;

    // loop through the lines of the file and get the contents
    while((nread = read_line_from_file(&line, filename, &len, offset)) != -1) {
        
        line_no++;
        offset += nread;

        if (strlen(line) != 0) {

            // if a line contains a gate declaration...
            if((starts_with(line, DECL_DESIGNATION))) {

                // raise the pending flag so we know if we exit too early
                pending = 1;

                // parse the line into a subsystem header
                Subsystem *s = malloc(sizeof(Subsystem));
                s->components = NULL;
                if ( (_en=str_to_subsys_hdr(line, s, strlen(line))) ) {
                    printf("error reading\n");
                    return _en;
                }

                // also allocate memory for the output mappings of the subsystem, now that we know how
                // many there should be
                s->output_mappings = malloc(sizeof(char*) * s->_outputc);

                // read the next line
                if ( (nread = read_line_from_file(&line, filename, &len, offset)) == -1 ) {
                    printf("%s:%d: Error! File ended right after %s declaration\n",filename, line_no, DECL_DESIGNATION);
                    return UNEXPECTED_EOF;
                }
                line_no++;
                offset += nread;

                // check that the next line starts with BEGIN 
                if (!starts_with(line, NETLIST_START)) {
                    printf("%s:%d: Syntax error! Expected %s, got %s instead\n",filename, line_no, NETLIST_START, line);
                    return SYNTAX_ERROR;
                }

                // check that the netlist is of the expected subsystem
                if  (!starts_with(line+strlen(NETLIST_START), s->name)) {
                    printf("%s:%d: Syntax error! Expected netlist for subsystem %s, got %s instead\n",filename, line_no, s->name, line);
                    return SYNTAX_ERROR;
                }


                // keep reading lines and parsing them until END is found
                while (1) {

                    // read the next line
                    if ( (nread = read_line_from_file(&line, filename, &len, offset)) == -1 ) {
                        printf("%s:%d: Error! File ended while %s netlist was pending\n",filename, line_no, s->name);
                        return UNEXPECTED_EOF;
                    }
                    line_no++;
                    offset += nread;

                    // if it starts with something that is an output of the subsystem, it's an output mapping
                    int index = index_starts_with(line, s->outputs, s->_outputc);
                    if (index != -1) {

                        char *_line = line;
                        split(&_line, OUTPUT_MAP_DELIM);

                        /*
                            _line now contains only the part after the delimiter, which is the signal
                            that is mapped to the (index)'th output of the subsystem
                        */
                        s->output_mappings[index] = malloc(strlen(_line)+1);
                        strncpy(s->output_mappings[index], _line, strlen(_line)+1);
                    } else 

                    // if it starts with a component declaration, create a component from it and add it to the subsystem
                    if (starts_with(line, COMP_ID_PREFIX)) {
                        Component *c = malloc(sizeof(Component));
                        if ( (_en=str_to_comp(line, c, strlen(line), gate_lib)) ) {
                            fprintf(stderr, "%s:%d: str_to_comp failed with _en %d on line [%s]\n", filename, line_no, _en, line);
                            return _en;
                        }
                        if ( (_en=subsys_add_comp(s, c)) ) return _en;
                    } else 

                    // else it should be an END ... NETLIST line
                    if (starts_with(line, NETLIST_END)) {
                        
                        // check that the ending netlist is of the current subsystem
                        if  (!starts_with(line+strlen(NETLIST_END), s->name)) {
                            printf("%s:%d: Syntax error! Expected end of netlist for subsystem %s, got %s instead\n",filename, line_no, s->name, line);
                            return SYNTAX_ERROR;
                        }
                        break;
                        pending = 0;
                    }

                }

                // the above loop only breaks if the netlist ended (otherwise it returns)
                // so reaching here means a whole subsystem has been parsed

                // turn it into a standard
                Standard *std = malloc(sizeof(Standard));
                std->type = SUBSYSTEM;
                std->subsys = s;
                std->defined_in = lib;
                if ( (_en=add_to_lib(lib, std)) ) return _en;
            }
        }
    }


    // print the contents of the library (assuming that its made of gates only)
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
            for (int i=0; i<comp->_inputc; i++)  {
                printf("%s ", comp->inputs[i]);
            }
            printf("respectively\n");

            _comp = _comp->next;

        }

        // print the output mappings
        printf("and the following output mappings:\n");

        for(int i=0; i<c2->_outputc; i++) {
            printf("\t%s <- %s\n", c2->outputs[i], c2->output_mappings[i]);
        }


        printf("\n");
        c = c->next;

    }


    free(line);
    free_lib(lib);
    free_lib(gate_lib);
    free(buf);
    return 0;
}

