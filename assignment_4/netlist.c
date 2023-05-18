#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "netlist.h"
#include "str_util.h"

void free_subsystem(Subsystem *s, int free_comp) {

    if (s!=NULL) {

        // free inputs
        if (s->inputs != NULL) {
            for(int i=0; i<s->_inputc; i++) {
                if (s->inputs[i] != NULL) {
                    free(s->inputs[i]);
                }
            }
            free(s->inputs);
        }

        // free outputs
        if (s->outputs != NULL) {
            for(int i=0; i<s->_outputc; i++) {
                if (s->outputs[i] != NULL) {
                    free(s->outputs[i]);
                }
            }
            free(s->outputs);
        }

        // free name
        if (s->name != NULL) {
            free(s->name);
        }

        // free component list
        if (s->components != NULL && free_comp) {

            Node *c=s->components; 
            while ( c!=NULL) {
                s->components = c->next;
                free_node(c, 1);
                c=s->components;
            }
        }

        // free output mapping str list
        if (s->output_mappings != NULL) {
            for (int i=0; i<s->_outputc; i++) {
                if (s->output_mappings[i] != NULL) {
                    free(s->output_mappings[i]);
                }
            }
            free(s->output_mappings);
        }

        // free the actual output mappings (if needed)
        if (s->is_standard) {
            if (s->o_maps != NULL) {
                for (int i=0; i<s->_outputc; i++) {
                    if (s->o_maps[i] != NULL) {
                        free(s->o_maps[i]);
                    }
                }
                free(s->o_maps);
            }
        }

        // free s itself
        free(s);

    }

}

int subsys_hdr_to_str(Subsystem* s, char* str, int n) {

    /*
        strncpy could be used to do each part of this, but
        it does not have an error return value so even though we
        would be sure that no buffer overflows have occured, we
        would not know whether the copying was completed properly
        or not. This is why write_at() was created.
    */

    if (s==NULL || str==NULL) {
        return NARG;
    }

    int offset = 0;
    int _en;

    // write the first word, indicating that this is a subsystem
    if( (_en = write_at(str, DECL_DESIGNATION, offset, strlen(DECL_DESIGNATION))) ) return _en;
    offset += strlen(DECL_DESIGNATION);

    // write the name of the subsystem
    if( (_en = write_at(str, s->name, offset, strlen(s->name))) ) return _en;
    offset += strlen(s->name);

    // write the default delimiter
    if( (_en = write_at(str, GENERAL_DELIM, offset, strlen(GENERAL_DELIM))) ) return _en;
    offset += strlen(GENERAL_DELIM);

    // write the input designation
    if( (_en = write_at(str, INPUT_DESIGNATION, offset, strlen(INPUT_DESIGNATION))) ) return _en;
    offset += strlen(INPUT_DESIGNATION);

    // write the inputs, separated by delimiter
    int list_bytes = 0;
    _en = write_list_at(s->_inputc, s->inputs, IN_OUT_DELIM, str, offset, n-1-offset, &list_bytes);
    if (_en) return _en;
    offset += list_bytes;

    // write the default delimiter
    if( (_en = write_at(str, GENERAL_DELIM, offset, strlen(GENERAL_DELIM))) ) return _en;
    offset += strlen(GENERAL_DELIM);

    // write the output designation
    if( (_en = write_at(str, OUTPUT_DESIGNATION, offset, strlen(OUTPUT_DESIGNATION))) ) return _en;
    offset += strlen(OUTPUT_DESIGNATION);

    // write the outputs, separated by delimiter
    list_bytes = 0;
    _en = write_list_at(s->_outputc, s->outputs, IN_OUT_DELIM, str, offset, n-1-offset, &list_bytes);
    if (_en) return _en;
    offset += list_bytes;

    // manually null terminate the string
    str[offset] = '\0';

    return 0;
}

int str_to_subsys_hdr(char *str, Subsystem *s, int n) {

    // any line declaring a subsystem starts with DECL_DESIGNATION which we ignore
    int offset = strlen(DECL_DESIGNATION);

    /* 
       because split alters its first argument, and we need to keep
       the original address of str so we can free it, we use an internal
       variable to pass to split
    */
    char *_str = str+offset;

    // strtok interprets delim as a collection of delimiters, so if even one of them is
    // found, it splits. we need to split if ALL OF THEM are found CONSECUTIVELY.
    // so we create what we need (see split()).

    // get the fields of the line
    char *name        = split(&_str, GENERAL_DELIM);
    char *raw_inputs  = split(&_str, GENERAL_DELIM);
    char *raw_outputs = split(&_str, GENERAL_DELIM);

    // get the input and output lists (get rid of the designations)
    char *_inputs = raw_inputs+strlen(INPUT_DESIGNATION);
    char *_outputs = raw_outputs+strlen(OUTPUT_DESIGNATION);

    // parse the input and output lists
    s->inputs = NULL;   // initialize to NULL so initial call to realloc is like malloc
    s->outputs = NULL;

    s->_inputc = str_to_list(_inputs, &(s->inputs), IN_OUT_DELIM);
    s->_outputc = str_to_list(_outputs, &(s->outputs), IN_OUT_DELIM);
    s->name = malloc(strlen(name)+1);
    strncpy(s->name, name, strlen(name)+1);

    return 0;
}

void free_component(Component *c) {

    if (c!= NULL) {

        // check the type and free as needed
        if (c->is_standard) {

            // then it has output mappings
            if (c->i_maps != NULL) {
                for(int i=0; i<c->_inputc; i++) {
                    if (c->i_maps[i] != NULL) {
                        free(c->i_maps[i]);
                    }
                }
                free(c->i_maps);
            }
        }

        // in any case there will be inputs
        if (c->inputs != NULL) {
            for(int i=0; i<c->_inputc; i++) {
                if (c->inputs[i] != NULL) {
                    free(c->inputs[i]);
                }
            }
            free(c->inputs);
        }

        // free c itself
        free(c);
    }
}

int comp_to_str(Component *c, char *str, int n) {

    int offset = 0;
    int _en;

    if (c==NULL || str==NULL) {
        return NARG;
    }

    // write the ID prefix
    if ( (_en=write_at(str, COMP_ID_PREFIX, offset, strlen(COMP_ID_PREFIX))) ) return _en;
    offset += strlen(COMP_ID_PREFIX);

    // write the component's ID
    offset += snprintf(str+offset, n-offset, "%d ", c->id)-1;
    
    // write the delimiter
    if ( (_en=write_at(str, COMP_DELIM, offset, strlen(COMP_DELIM))) ) return _en;
    offset += strlen(COMP_DELIM);

    // write the type of the component
    char *name = c->prototype->type==GATE?c->prototype->gate->name : c->prototype->subsys->name;
    if ( (_en=write_at(str, name, offset, strlen(name))) ) return _en;
    offset += strlen(name);

    // write the delimiter
    if ( (_en=write_at(str, COMP_DELIM, offset, strlen(COMP_DELIM))) ) return _en;
    offset += strlen(COMP_DELIM);

    // write the names of the inputs, separated by a delimiter
    int list_bytes = 0;
    _en = write_list_at(c->_inputc, c->inputs, IN_OUT_DELIM, str, offset, n, &list_bytes);
    if (_en) return _en;
    offset += list_bytes;

    // manually null terminate the string
    str[offset] = '\0';

    return 0;
}

int netlist_to_file(Subsystem *s, char *filename, char *mode) {

    if (s==NULL || filename==NULL || mode==NULL) {
        return NARG;
    }

    // open the file
    FILE *fp;
    fp = fopen(filename, mode);
    if (fp == NULL) {
        perror("fopen");
        exit(-1);
    }

    // if mode = append, write an extra newline to separate the netlists
    if (starts_with(mode, "a")) fprintf(fp, "\n");

    // write the declaration line
    char *line = malloc(MAX_LINE_LEN);
    int _en;
    if ( (_en=subsys_hdr_to_str(s, line, MAX_LINE_LEN)) ) {
        free(line);
        return _en;
    }
    fprintf(fp, "%s\n", line);

    // write the "BEGIN NETLIST" line
    fprintf(fp, "BEGIN %s NETLIST\n", s->name);

    // write the components
    for (Node *c=s->components; c!=NULL; c=c->next) {
        if ( (_en=comp_to_str(c->comp, line, MAX_LINE_LEN)) ) {
            free(line);
            return _en;
        }
        fprintf(fp, "%s\n", line);
    }

    // write the output mappings
    for (int i=0; i<s->_outputc; i++) {
        fprintf(fp, "%s = %s\n", s->outputs[i], s->output_mappings[i]);
    }

    // write the "END NETLIST" line
    fprintf(fp, "END %s NETLIST\n", s->name);

    // cleanup
    fclose(fp);
    free(line);

    return 0;
}

int str_to_gate(char *str, Gate *g, int n) {

    if (str==NULL || g==NULL) {
        return NARG;
    }

    // any line declaring a gate starts with DECL_DESIGNATION which we ignore
    int offset = strlen(DECL_DESIGNATION);

    /* 
       because split alters its first argument, and we need to keep
       the original address of str so we can free it, we use an internal
       variable to pass to split
    */
    char *_str = str+offset;

    // get the fields of the line
    char *name        = split(&_str, GENERAL_DELIM);
    char *raw_inputs  = split(&_str, GENERAL_DELIM);

    // get the input list (get rid of the designation)
    char *_inputs = raw_inputs+strlen(INPUT_DESIGNATION);

    // parse the input and output lists
    g->inputs = NULL;   // initialize to NULL so initial call to realloc is like malloc

    g->_inputc = str_to_list(_inputs, &(g->inputs), IN_OUT_DELIM);
    g->name = malloc(strlen(name)+1);
    strncpy(g->name, name, strlen(name)+1);

    return 0;
}

int gate_to_str(Gate *g, char *str, int n) {
    
    if (g==NULL || str==NULL) {
        return NARG;
    }

    int offset = 0;
    int _en;

    // write the first word, indicating that this is a gate
    if( (_en = write_at(str, DECL_DESIGNATION, offset, strlen(DECL_DESIGNATION))) ) return _en;
    offset += strlen(DECL_DESIGNATION);

    // write the name of the gate
    if( (_en = write_at(str, g->name, offset, strlen(g->name))) ) return _en;
    offset += strlen(g->name);

    // write the default delimiter
    if( (_en = write_at(str, GENERAL_DELIM, offset, strlen(GENERAL_DELIM))) ) return _en;
    offset += strlen(GENERAL_DELIM);

    // write the input designation
    if( (_en = write_at(str, INPUT_DESIGNATION, offset, strlen(INPUT_DESIGNATION))) ) return _en;
    offset += strlen(INPUT_DESIGNATION);

    // write the inputs, separated by delimiter
    int list_bytes = 0;
    _en = write_list_at(g->_inputc, g->inputs, IN_OUT_DELIM, str, offset, n-1-offset, &list_bytes);
    if (_en) return _en;
    offset += list_bytes;

    // manually null terminate the string
    str[offset] = '\0';
    return 0;
}

void free_gate(Gate *g) {

    // free name
    if (g->name != NULL) {
        free(g->name);
    }

    // free inputs
    if (g->inputs != NULL) {
        for(int i=0; i<g->_inputc; i++) {
            if (g->inputs[i] != NULL) {
                free(g->inputs[i]);
            }
        }
        free(g->inputs);
    }

    // free g itself
    free(g);
}

void free_standard(Standard *s) {
    
    if (s != NULL) {

        // call the appropriate free() function 
        if (s->type == GATE) {
            free_gate(s->gate);
        } else if (s->type == SUBSYSTEM) {
            if (s->subsys != NULL){
                free_subsystem(s->subsys, 1);
            }
        }

        free(s);
    }
}

void free_node(Node *n, int complete) {

    if (n != NULL) {

        if (complete) {
            if (n->type == STANDARD) {
                free_standard(n->std);
            } else if (n->type == COMPONENT) {
                free_component(n->comp);
            } else if (n->type == SUBSYSTEM_N) {
                free_subsystem(n->subsys, 1);
            }
        }

        free(n);
    }
}

int add_to_lib(Netlist *lib, void* s, int is_standard, enum STANDARD_TYPE type) {

    if (lib==NULL || s==NULL) {
        return NARG;
    }

    // make the standard into a node
    Node *n = malloc(sizeof(Node));
    if (is_standard) {
        n->type = STANDARD;
        n->std = (Standard*) s;
    } else {
        if (type == SUBSYSTEM) {
            n->type = SUBSYSTEM_N;
            n->subsys = (Subsystem*) s;
        }
    }

    n->next = NULL;

    // connect the new node to the library
    if (lib->contents == NULL) {
        lib->contents = n;
    } else {
        lib->_tail->next = n;
    }

    lib->_tail = n;

    return 0;
}

int gate_lib_from_file(char *filename, Netlist* lib) {
    
    if (filename == NULL || lib==NULL) {
        return NARG;
    }
    
    char *line = NULL;  // no need to malloc it, getline() does that for us (and also reallocs if needed) 

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

        if (strlen(line) != 0) {

            // if a line contains a gate declaration...
            if((starts_with(line, DECL_DESIGNATION))) {

                // ...parse the line into a new gate...
                Gate *g = malloc(sizeof(Gate));
                if ( (_en=str_to_gate(line, g, strlen(line))) ) return _en; // strlen(line) and not nread, because they might differ (see read_line())

                // ...create a standard from that new gate...
                Standard *s = malloc(sizeof(Standard));
                s->type = GATE;
                s->gate = g;
                s->defined_in = lib;

                // ...and add the standard to the library
                if ( (_en=add_to_lib(lib, s, 1, GATE)) ) return _en;
            }
        }

        // move the cursor according to the bytes read from the file
        offset += nread;
    }

    free(line);

    return 0;
}

int subsys_lib_from_file(char *filename, Netlist *lib, Netlist *lookup_lib) {
    
    if (filename == NULL || lib==NULL) {
        return NARG;
    }
    
    char *line = NULL;  // no need to malloc it, getline() does that for us (and also reallocs if needed) 
    int line_no = 0;
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
        int index = -1;

        if (strlen(line) != 0) {    // if the line is empty, skip it

            // if a line contains a gate declaration a new subsystem must be parsed
            if((starts_with(line, DECL_DESIGNATION))) {

                // parse the first line into a subsystem header
                Subsystem *s = malloc(sizeof(Subsystem));
                s->is_standard = 1;
                s->components = NULL;
                if ( (_en=str_to_subsys_hdr(line, s, strlen(line))) ) {
                    printf("error reading\n");
                    return _en;
                }

                // also allocate memory for the output mappings of the subsystem, now that we know how many there should be
                s->output_mappings = malloc(sizeof(char*) * s->_outputc);
                s->o_maps = malloc(sizeof(Mapping*) * s->_outputc);

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


                // keep reading lines and parsing them until END ... NETLIST is found
                while (1) {

                    // read the next line
                    if ( (nread = read_line_from_file(&line, filename, &len, offset)) == -1 ) {
                        printf("%s:%d: Error! File ended while %s netlist was pending\n",filename, line_no, s->name);
                        return UNEXPECTED_EOF;
                    }
                    line_no++;
                    offset += nread;

                    // if it starts with something that is an output of the subsystem, it's an output mapping
                    if ( (index = index_starts_with(line, s->outputs, s->_outputc)) != -1) {

                        char *_line = line;
                        split(&_line, OUTPUT_MAP_DELIM);

                        /*
                            _line now contains only the part after the delimiter, which is the signal
                            that is mapped to the (index)'th output of the subsystem
                        */
                        s->output_mappings[index] = malloc(strlen(_line)+1);
                        strncpy(s->output_mappings[index], _line, strlen(_line)+1);
                    
                        // also create the actual mapping
                        s->o_maps[index] = malloc(sizeof(Mapping));

                        // find if it is an input or a component
                        int map_index = -1;
                        if ( (map_index=contains(s->_inputc, s->inputs, s->output_mappings[index])) != -1 ) {

                            s->o_maps[index]->type = SUBSYS_INPUT;
                            s->o_maps[index]->index = map_index;
                        } else {
                            
                            // if it is not an input of the subsystem, then it is the ID of another
                            // component. we need to remove the prefix and check against the IDs of
                            // the rest of the components.

                            // first remove the prefix (and the underscore) and cast to int
                            int outmap_comp_id = atoi(s->output_mappings[index]+strlen(COMP_ID_PREFIX));

                            map_index = 0;
                            Node *cur = s->components;
                            while (cur != NULL) {
                                if (cur->comp->id == outmap_comp_id) {
                                    break;
                                }
                                map_index++;
                                cur = cur->next;
                            }

                            // create the mapping
                            s->o_maps[index]->type = SUBSYS_COMP;
                            s->o_maps[index]->index = map_index;
                        }

                    }

                    // if it starts with a component declaration, create a component from it and add it to the subsystem
                    else if (starts_with(line, COMP_ID_PREFIX)) {
                        Component *c = malloc(sizeof(Component));
                        if ( (_en=str_to_comp(line, c, strlen(line), lookup_lib, s, 1)) ) {
                            fprintf(stderr, "%s:%d: component parsing failed\n", filename, line_no);
                            return _en;
                        }
                        if ( (_en=subsys_add_comp(s, c)) ) return _en;
                    }

                    // else it should be an END ... NETLIST line
                    else  if (starts_with(line, NETLIST_END)) {
                        
                        // check that the ending netlist is of the current subsystem
                        if  (!starts_with(line+strlen(NETLIST_END), s->name)) {
                            printf("%s:%d: Syntax error! Expected end of netlist for subsystem %s, got %s instead\n",filename, line_no, s->name, line);
                            return SYNTAX_ERROR;
                        }
                        break;
                    }

                    // if none of the above conditions hold, the line is assumed to be of no interest
                    // to us and is skipped

                }

                // the above loop only breaks if the netlist ended (otherwise it returns)
                // so reaching here means a whole subsystem has been parsed

                // turn it into a standard
                Standard *std = malloc(sizeof(Standard));
                std->type = SUBSYSTEM;
                std->subsys = s;
                std->defined_in = lib;
                if ( (_en=add_to_lib(lib, std, 1, SUBSYSTEM)) ) return _en;
            }

        }

    }

    free(line);

    return 0;
}

void free_lib(Netlist *lib) {

    if (lib != NULL) {

        // free the contents
        if (lib->contents != NULL) {
            Node *cur = lib->contents;
            while (cur != NULL) {
                lib->contents = cur->next;
                free_node(cur, 1);
                cur = lib->contents;
            }
        }

        // free the filename
        if (lib->file != NULL) {
            free(lib->file);
        }

        // free the lib itself
        free(lib);
    }
}

Standard* search_in_lib(Netlist *lib, char *name) {


    if (lib==NULL || name==NULL) {
        return NULL;
    }

    Standard *r_std = NULL;
    Node *n = lib->contents;

    while (n!= NULL) {

        if (n->type != STANDARD) return NULL;
        if (n->std == NULL) return NULL;

        // check the name of the standard in the current node appropriately        
        r_std = n->std;
        if (r_std->type == GATE) {
            if ((strlen(name) == strlen(r_std->gate->name)) && (strncmp(r_std->gate->name, name, strlen(name)) == 0)) {
                return r_std;
            }
        } else if (r_std->type == SUBSYSTEM) {
            if ((strlen(name) == strlen(r_std->subsys->name)) &&  (strncmp(r_std->subsys->name, name, strlen(name)) == 0)) {
                return r_std;
            }
        }
        
        // move on to the next node
        n=n->next;
    }

    return NULL;
}

int str_to_comp(char *str, Component *c, int n, Netlist *lib, Subsystem *s, int is_standard) {

    if (str==NULL || c==NULL) {
        return NARG;
    }

    // skip the ID prefix
    char *_str = str+strlen(COMP_ID_PREFIX);

    // read the fields
    char *_id         = split(&_str, COMP_DELIM);
    char *_name       = split(&_str, COMP_DELIM);
    char *_raw_inputs = _str;

    // add the id to the component
    c->id = atoi(_id);

    // check if the name of the component is a known one
    Standard *std = search_in_lib(lib, _name);
    if (std == NULL) {
        printf("Could not find component '%s' in library %s\n", _name, lib->file);
        return UNKNOWN_COMP;
    }
    c->prototype = std;

    // parse the inputs into the component
    c->inputs = NULL;   // initialize to NULL so initial call to realloc is like malloc
    c->_inputc = str_to_list(_raw_inputs, &(c->inputs), IN_OUT_DELIM);

    c->is_standard=is_standard;

    // if needed, take care of the input mappings
    if ( (s!=NULL) && is_standard ) {
        
        // allocate space for the list
        c->i_maps = malloc(sizeof(Mapping*) * c->_inputc);

        // create each individual mapping
        for (int i=0; i<c->_inputc; i++) {

            // allocate space for each individual mapping
            c->i_maps[i] = malloc(sizeof(Mapping));
            
            // find if it is an input or a component
            int index = -1;
            if ( (index=contains(s->_inputc, s->inputs, c->inputs[i])) != -1 ) {

                c->i_maps[i]->type = SUBSYS_INPUT;
                c->i_maps[i]->index = index;
            } else {
                
                // if it is not an input of the subsystem, then it is the ID of another
                // component. we need to remove the prefix and check against the IDs of
                // the rest of the components.

                // first remove the prefix and cast to int
                int input_id = atoi(c->inputs[i]+strlen(COMP_ID_PREFIX));

                index = 0;
                Node *cur = s->components;
                while (cur != NULL) {
                    if (cur->comp->id == input_id) {
                        break;
                    }
                    index++;
                    cur = cur->next;
                }

                // create the mapping
                c->i_maps[i]->type = SUBSYS_COMP;
                c->i_maps[i]->index = index;

            }
        }
    }

    return 0;
}

int subsys_add_comp(Subsystem *s, Component *c) {
    
    if (s==NULL || c==NULL) {
        return NARG;
    }

    // make the component into a node
    Node *n = malloc(sizeof(Node));
    n->type = COMPONENT;
    n->comp = c;
    n->next = NULL;

    // connect the new node to the library
    if (s->components == NULL) {
        s->components = n;
    } else {
        s->_tail->next = n;
    }

    s->_tail = n;

    return 0;
}

Node *move_in_list(int x, Node *list) {

    // move x positions in the list as long as they are valid
    while(x > 0) {
        if (list == NULL) return NULL;
        list = list->next;
        x--;
    }

    return list;
}

int create_custom(Subsystem *ns, Standard *std, int inputc, char **inputs, int starting_index) {

    if (std==NULL || inputs==NULL) {
        return NARG;
    }

    if (std->type != SUBSYSTEM) {
        return NARG;
    }

    int _en=0;

    // copy the name, inputs and outputs of the standard into ns
    ns->name = malloc(strlen(std->subsys->name)+1);
    strncpy(ns->name, std->subsys->name, strlen(std->subsys->name)+1);


    ns->_inputc = std->subsys->_inputc;
    ns->inputs = malloc(sizeof(char*) * std->subsys->_inputc);
    for(int i=0; i<std->subsys->_inputc; i++) {
        ns->inputs[i] = malloc(strlen(inputs[i])+1);
        strncpy(ns->inputs[i], inputs[i], strlen(inputs[i])+1);
    }

    ns->_outputc = std->subsys->_outputc;
    ns->outputs = malloc(sizeof(char*) * std->subsys->_outputc);
    for(int i=0; i<std->subsys->_outputc; i++) {
        ns->outputs[i] = malloc(strlen(std->subsys->outputs[i])+1);
        strncpy(ns->outputs[i], std->subsys->outputs[i], strlen(std->subsys->outputs[i])+1);
    }

    // mark the new subsystem as non-standard
    ns->is_standard = 0;

    // loop through the components of the prototype
    int comp_id = starting_index;
    Node *cur = std->subsys->components;
    ns->components = NULL;
    while (cur != NULL) {

        // allocate memory for the component
        Component *comp = malloc(sizeof(Component));
        comp->id = comp_id;
        comp->is_standard = 0;
        comp->prototype = cur->comp->prototype;

        // allocate memory for the inputs of the component
        comp->inputs = malloc(sizeof(char*) * cur->comp->_inputc);
        comp->_inputc = cur->comp->_inputc;

        // create the inputs according to the prototype's mappings
        for (int i=0; i<cur->comp->_inputc; i++) {
            
            // find the mapping
            Mapping *map = cur->comp->i_maps[i];

            // check the type of the mapping and act accordingly
            if (map->type == SUBSYS_INPUT) {

                // find the input that the mapping maps to and put its name in the corresponding slot
                comp->inputs[i] = malloc(strlen(ns->inputs[map->index])+1);
                strncpy(comp->inputs[i], ns->inputs[map->index], strlen(ns->inputs[map->index])+1);

            } else if (map->type == SUBSYS_COMP) {

                // find the component that the mapping maps to
                Component *cmap = move_in_list(map->index, ns->components)->comp;

                // allocate memory for the input in the component
                comp->inputs[i] = malloc(digits(cmap->id)+2); // +1 for the U, +1 for the null byte
                int offset=0;

                // write the ID prefix
                if ( (_en=write_at(comp->inputs[i], COMP_ID_PREFIX, offset, strlen(COMP_ID_PREFIX))) ) return _en;
                offset += strlen(COMP_ID_PREFIX);

                // write the mapping component's ID
                snprintf(comp->inputs[i]+offset, digits(cmap->id)+2-offset, "%d ", cmap->id);

            }

        }

        // add the node containing the component to the component list of the new subsystem
        if ( (_en=subsys_add_comp(ns, comp)) ) return _en;

        // advance the current node in the prototype's list and increment the ID
        cur = cur->next;
        comp_id++;
    }

    // also take care of the output mappings of the subsystem
    ns->output_mappings = malloc(sizeof(char*) * std->subsys->_outputc);
    for(int i=0; i<std->subsys->_outputc; i++) {

        // find the mapping
        Mapping *map = std->subsys->o_maps[i];

        // check the type of the mapping and act accordingly
        if (map->type == SUBSYS_INPUT) {

            // find the input that the mapping maps to and put its name in the corresponding slot
            ns->output_mappings[i] = malloc(strlen(ns->output_mappings[map->index])+1);
            strncpy(ns->output_mappings[i], ns->output_mappings[map->index], strlen(ns->output_mappings[map->index])+1);

        } else if (map->type == SUBSYS_COMP) {

            // find the component that the mapping maps to
            Component *cmap = move_in_list(map->index, ns->components)->comp;

            // allocate memory for the output mapping
            ns->output_mappings[i] = malloc(digits(cmap->id)+2); // +1 for the U, +1 for the null byte
            int offset=0;

            // write the ID prefix
            if ( (_en=write_at(ns->output_mappings[i], COMP_ID_PREFIX, offset, strlen(COMP_ID_PREFIX))) ) return _en;
            offset += strlen(COMP_ID_PREFIX);

            // write the mapping component's ID
            snprintf(ns->output_mappings[i]+offset, digits(cmap->id)+2-offset, "%d ", cmap->id);

        }
    }

    return comp_id;
}

/**
 * Given a netlist (in the form of a library in order to avoid creating another
 * struct), parse the subsystems in it, and create a netlist for each one using
 * only gates (translate each subsystem all the way down to the gates it is defined
 * as in the library it is defined in).
 * 
 * Stores the translated compponents in dest, which is assumed to be allocated.
 * 
 * Starts the component ID numbering from the given one.
 * 
 * Returns:
 *  - 0 on success
 *  - -1 on failure
*/
int netlist_to_gate_only(Netlist *dest, Netlist *netlist, int component_id) {

    // set the destination info
    dest->contents = NULL;
    dest->file = NULL;
    dest->type = SUBSYSTEM;
    dest->_tail = NULL;

    // iterate over the contents of the netlist
    Node *node_ptr = netlist->contents;
    while (node_ptr!=NULL) {

        // this is the subsystem whose netlist we want to boil down to just gates
        Subsystem *target = node_ptr->std->subsys;

        // count the compponents in the target subsystem
        int subsys_count = 0;
        Node *_comp = target->components;
        while (_comp != NULL) {
            _comp = _comp->next;
            subsys_count++;
        }

        // allocate space for the new, gate only subsystem
        Subsystem *only_gates_sub = malloc(sizeof(Subsystem));

        // initialize the fields of the new subsystem to match the old one
        only_gates_sub->is_standard = 0;
        only_gates_sub->name = malloc(strlen(target->name)+1);
        strncpy(only_gates_sub->name, target->name, strlen(target->name)+1);
        only_gates_sub->_inputc = target->_inputc;
        only_gates_sub->inputs = malloc(sizeof(char*) * target->_inputc);
        only_gates_sub->_outputc = target->_outputc;
        only_gates_sub->outputs = malloc(sizeof(char*) * target->_outputc);
        for(int i=0; i<target->_inputc; i++){
            only_gates_sub->inputs[i] = malloc(strlen(target->inputs[i])+1);
            strncpy(only_gates_sub->inputs[i], target->inputs[i], strlen(target->inputs[i])+1);
        }
        for(int i=0; i<target->_outputc; i++){
            only_gates_sub->outputs[i] = malloc(strlen(target->outputs[i])+1);
            strncpy(only_gates_sub->outputs[i], target->outputs[i], strlen(target->outputs[i])+1);
        }
        only_gates_sub->components = NULL;

        // the components that get translated to gates will be stored as subsystems
        // in this array, so that if any component refers to the output of another,
        // we can find both the referred one and the mapping of the output
        Subsystem **subsystems = malloc(sizeof(Subsystem*) * subsys_count);
        int s_i = 0; // the index we will use for the array above

        // iterate over the components of the old subsystem, resolving input mappings
        // like UXX_S0 to the gate that is mapped to output S0 of component XX
        _comp = target->components;
        while (_comp != NULL) {

            // the component (we use the nodes to iterate)
            Component *comp = _comp->comp;

            // the (resolved) inputs that will be passed to to create_custom()
            char **inputs = malloc(sizeof(char*) * comp->_inputc);

            // iterate over the inputs of the component
            for (int i=0; i<comp->_inputc; i++)  {

                // find the mapping for each input
                Mapping *map = comp->i_maps[i];

                // if it maps to another component
                if (map->type == SUBSYS_COMP) {

                    // find out the component that it maps to
                    Component *cmap = move_in_list(map->index, target->components)->comp;

                    // assume all components are subsystems, so no else
                    if (cmap->prototype->type == SUBSYSTEM) {
                        
                        // then after the UXX_ in inputs[i] there is the name of the output we are interested in
                        char *o_name = comp->inputs[i]+strlen(COMP_ID_PREFIX)+digits(map->index+1)+1;

                        // find the index of that output in the mapping
                        int index = contains(cmap->prototype->subsys->_outputc, cmap->prototype->subsys->outputs, o_name);
                        if (index == -1) {
                            char *b = malloc(sizeof(MAX_LINE_LEN));
                            int b_w=0;
                            write_list_at(cmap->prototype->subsys->_outputc, cmap->prototype->subsys->outputs, ", ", b, 0, MAX_LINE_LEN, &b_w);
                            printf("Could not find output %s in the outputs of subsystem %s (%s)!\n", o_name, cmap->prototype->subsys->name, b);
                            free(b);
                            return -1;
                        }
                        // find the output mapping for that output
                        char *m = subsystems[map->index]->output_mappings[index];

                        // put it in the input list
                        inputs[i] = malloc(strlen(m)+1); // +1 for the null byte
                        strncpy(inputs[i], m, strlen(m)+1);
                    }

                } else if (map->type == SUBSYS_INPUT) {

                    // find the input of the subsystem that the mapping maps to
                    char *m = target->inputs[map->index];
                    
                    // put it in the input list
                    inputs[i] = malloc(strlen(m)+1); // +1 for the null byte
                    strncpy(inputs[i], m, strlen(m)+1);

                }
            }
            
            /*
            Useful for debugging, can print the inputs of each subsystem and see if the gates are ok
            
            fprintf(stderr, "inputs for U%d, %s are the following: ", comp->id, comp->prototype->subsys->name);
            for (int i=0; i<comp->_inputc; i++) {
                fprintf(stderr, "%s ", inputs[i]);
            }
            fprintf(stderr, "\n");
            */
            
            /*
                we now have a complete list of inputs for each component
                we need to create the netlist of that component with the inputs we now have
                and add it to the subsystems array
            */
            subsystems[s_i] = malloc(sizeof(Subsystem));
            component_id = create_custom(subsystems[s_i], comp->prototype, comp->_inputc, inputs, component_id);

            // free the input list
            for(int i=0; i<comp->_inputc; i++) free(inputs[i]);

            // add every gate to the gate-only subsystem, freeing the used nodes in the process
            Node *c_c = subsystems[s_i]->components;
            while (c_c != NULL) {
                subsys_add_comp(only_gates_sub, c_c->comp);
                Node *tmp = c_c->next;
                free_node(c_c, 0);
                c_c = tmp;
            }

            // move on to the next subsystem
            _comp = _comp->next;
            s_i++;
            free(inputs);

        }
        
        // we have now filled the subsystems array with subsystems whose components are gates and numbered consistently

        // now we need to map the outputs of target to gate only things
        only_gates_sub->output_mappings = malloc(sizeof(char*) * only_gates_sub->_outputc); // +1 for the null byte

        for(int i=0; i<target->_outputc; i++) {
            
            Mapping *om = target->o_maps[i];

            // assuming all mappings are to gates and none is to input
            if (om->type != SUBSYS_COMP) return -1;

            // find out the component that the mapping maps to, which will now be a subsystem in subsystems
            Subsystem *mapped = subsystems[om->index];

            // assume everything in the subsystems[] array is a subsystem (and not a gate) (TODO: FIX)
                        
            // then after the UXX_ in output_mappings[i] there is the name of the output we are interested in
            // find the name of the output
            char *o_name = target->output_mappings[i]+strlen(COMP_ID_PREFIX)+digits(om->index+1)+1;

            // find the index of that output in the mapping
            int index = contains(mapped->_outputc, mapped->outputs, o_name);
            if (index == -1) {
                char *buf = malloc(sizeof(MAX_LINE_LEN));
                int b_w=0;
                write_list_at(mapped->_outputc, mapped->outputs, ", ", buf, 0, MAX_LINE_LEN, &b_w);
                printf("Could not find output '%s' in the outputs of subsystem '%s' (%s)!\n", o_name, mapped->name, buf);
                free(buf);
                return -1;
            }
            // find the output mapping for that output
            char *m = mapped->output_mappings[index];

            // put it in the output list
            only_gates_sub->output_mappings[i] = malloc(strlen(m)+1); // +1 for the null byte
            strncpy(only_gates_sub->output_mappings[i], m, strlen(m)+1);

        }

        // add the gate-only subsystem to the destination library
        int _en=0;
        if ( (_en=add_to_lib(dest, only_gates_sub, 0, SUBSYSTEM)) ) return _en;

        // cleanup the array that was used for the components of this subsystem
        for(int i=0; i<subsys_count; i++) {
            free_subsystem(subsystems[i], 0);
        }
        free(subsystems);

        // advance to the next subsystem to be translated
        node_ptr = node_ptr->next;


    }
    return 0;
}

void lib_to_file(Netlist *lib, char *filename, char *mode) {

    FILE *fp = fopen(filename, mode);

    // print the stuff
    for (Node *n = lib->contents; n!=NULL; n=n->next) {

        // print the header line
        char *buf = malloc(MAX_LINE_LEN);
        subsys_hdr_to_str(n->subsys, buf, MAX_LINE_LEN);
        fprintf(fp, "%s\n", buf);
        free(buf);

        // print the BEGIN ... NETLIST line
        fprintf(fp, "BEGIN %s NETLIST\n", n->subsys->name);

        // print the components
        for (Node *n_n=n->subsys->components; n_n!=NULL; n_n=n_n->next) {


            Component *comp = n_n->comp;
            fprintf(fp, "U%d %s ", comp->id, comp->prototype->gate->name);

            for (int i=0; i<comp->_inputc; i++) {
                fprintf(fp, "%s ", comp->inputs[i]);
            }
            fprintf(fp, "\n");
            if(comp->id % 5 == 0) {
                fprintf(fp, "\n");
            }
        }

        // print the output mappings
        for (int i=0; i<n->subsys->_outputc; i++) {
            fprintf(fp, "%s = %s\n", n->subsys->outputs[i], n->subsys->output_mappings[i]);
        }

        // print the END ... NETLIST line
        fprintf(fp, "END %s NETLIST\n", n->subsys->name);

    }

    fclose(fp);

}

Subsystem *instantiate_subsys(Subsystem *std, char **inputs, int inputc, char **outputs, int outputc) {

    Subsystem *instance = malloc(sizeof(Subsystem));

    // check if null, if number of inputs/outputs the same, etc TODO

    // set the name
    instance->name = malloc(strlen(std->name)+1);
    instance->is_standard = 0;
    strncpy(instance->name, std->name, strlen(std->name)+1);

    // set the inputs and outputs according to the given names
    deepcopy_str_list(&(instance->inputs), inputs, inputc);
    instance->_inputc = inputc;

    deepcopy_str_list(&(instance->outputs), outputs, outputc);
    instance->_outputc = outputc;

    // create the components of the instance according to the standard
    Node *nd = std->components;
    instance->components = NULL;
    while (nd != NULL) {

        Component *comp = malloc(sizeof(Component));

        comp->is_standard = 0;
        comp->id = nd->comp->id;
        comp->prototype = nd->comp->prototype;

        // resolve the input mappings
        comp->inputs = malloc(sizeof(char*) * comp->prototype->subsys->_inputc);
        comp->_inputc = comp->prototype->subsys->_inputc;
        for(int i=0; i<comp->prototype->subsys->_inputc; i++) {
            
            Mapping *m = nd->comp->i_maps[i];
            char *tmp;
            if (m->type == SUBSYS_INPUT) {

                // find the input that the mapping refers to
                tmp = instance->inputs[m->index];
                comp->inputs[i] = malloc(strlen(tmp)+1);
                strncpy(comp->inputs[i], tmp, strlen(tmp)+1);

            } else if (m->type == SUBSYS_COMP) {

                // find the component that the mapping refers to
                Node *rtcn = move_in_list(m->index, instance->components);  // referred_to_comp_node (the node that contains the component that the mapping refers to)
                
                // write the component id and the output name in a string
                tmp = malloc(1+digits(rtcn->comp->id)+1+strlen(rtcn->comp->prototype->subsys->outputs[m->out_index])+2);  // allocate memory for the 'U', the ID, the '_', the output name and a null byte
                sprintf(tmp, "U%d_%s", rtcn->comp->id, rtcn->comp->prototype->subsys->outputs[m->out_index]);
                
                // copy that string into the component's inputs (and free it)
                comp->inputs[i] = malloc(strlen(tmp)+1);
                strncpy(comp->inputs[i], tmp, strlen(tmp)+1);
                free(tmp);

            }

        }

        // add the newly created component to the instance
        subsys_add_comp(instance, comp);

        nd = nd->next;
    }

    // map the outputs according to the standard (resolve the mappings)
    instance->output_mappings = malloc(sizeof(char*) * instance->_outputc);
    for(int i=0; i<instance->_outputc; i++) {
        
        Mapping *m = std->o_maps[i];

        if (m->type == SUBSYS_INPUT) {

            // find the input that the mapping refers to
            instance->output_mappings[i] = malloc(strlen(instance->inputs[m->index])+1);
            strncpy(instance->output_mappings[i], instance->inputs[m->index], strlen(instance->inputs[m->index])+1);
        
        } else if (m->type == SUBSYS_COMP) {
        
            // find the component that the mapping refers to
            Node *rtcn = move_in_list(m->index, instance->components);  // referred_to_comp_node (the node that contains the component that the mapping refers to)
            
            // write the component id and the output name in a string
            char *tmp = malloc(1+digits(rtcn->comp->id)+1+strlen(rtcn->comp->prototype->subsys->outputs[m->out_index])+2);  // allocate memory for the 'U', the ID, the '_', the output name and a null byte
            sprintf(tmp, "U%d_%s", rtcn->comp->id, rtcn->comp->prototype->subsys->outputs[m->out_index]);
            
            // copy that string into the instance's outputs (and free it)
            instance->output_mappings[i] = malloc(strlen(tmp)+1);
            strncpy(instance->output_mappings[i], tmp, strlen(tmp)+1);
            free(tmp);
        }

    }

    return instance;

}
