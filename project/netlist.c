/**
 * @file    netlist.c
 * 
 * @author  Petros Bimpiris (pbimpiris@tuc.gr)
 * 
 * @brief   Implementations of the functions declared in the netlist.h header file.
 * 
 * @version 0.1
 * 
 * @date 11-06-2023
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "netlist.h"
#include "str_util.h"

LList* ll_init() {
    LList *ll = malloc(sizeof(LList));
    ll->head = NULL;
    ll->tail = NULL;

    return ll;
}

int ll_add(LList *ll, Node* new_element) {

    if (new_element==NULL) {
        return NARG;
    }

    // connect the new node to the list
    if (ll->head == NULL) {
        ll->head = new_element;
    } else {
        ll->tail->next = new_element;
    }

    ll->tail = new_element;

    return 0;
}

void ll_free(LList *l, int complete) {

    if (l != NULL) {

        if (l->head != NULL) {

            Node *c=l->head, *tmp=c->next;
            while ( c!=NULL) {
                tmp = c->next;
                free_node(c, complete);
                c=tmp;
            }
        }

        free(l);
    }
}

void ll_print(LList *l) {
    fprintf(stderr, "[ ");
    for(Node *n = l->head; n != NULL; n=n->next) {
        if (n->type == COMPONENT) {
            fprintf(stderr, "component {id: %d}, ", n->comp->id);
        } else if (n->type == SUBSYSTEM_N) {
            fprintf(stderr, "subsystem {%s}, ", n->subsys->name);
        } else if (n->type == STANDARD) {
            if (n->std->type == GATE) {
                fprintf(stderr, "gate {%s}, ", n->std->gate->name);
            } else if (n->std->type == SUBSYSTEM) {
                fprintf(stderr, "subsystem {%s}, ", n->std->subsys->name);
            } else {
                fprintf(stderr, "unknown standard, ");
            }
        } else if (n->type == ALIAS) {
            char *b = malloc(BUFSIZ);
            alias_to_str(n->alias, b, BUFSIZ);
            fprintf(stderr, "alias {%s}, ", b);
            free(b);
        } else {
            fprintf(stderr, "unknown node, ");
        }
    }
    fprintf(stderr, " ]\n");
}

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
        if (s->components != NULL) {
            ll_free(s->components, free_comp);
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

        // free aliases
        if (s->aliases != NULL) {
            ll_free(s->aliases, 1);
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

int str_to_gate(char *str, Gate *g, int n, int parse_tt) {

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
    char *truth_table = _str;

    // get the input list (get rid of the designation)
    char *_inputs = raw_inputs+strlen(INPUT_DESIGNATION);

    // parse the name
    g->name = malloc(strlen(name)+1);
    strncpy(g->name, name, strlen(name)+1);

    // parse the input list
    g->inputs = NULL;   // initialize to NULL so initial call to realloc is like malloc
    g->_inputc = str_to_list(_inputs, &(g->inputs), IN_OUT_DELIM);

    // parse the truth table
    g->truth_table = parse_truth_table(truth_table);

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

int str_to_alias(char *str, Alias* a, Subsystem *s, int n){

    // D = PIPIS
    // to " = " einai MAP_DELIM

    // we need to keep the str pointer intact
    char *_str = str;

    // split on the mapping delimeter
    char *name     = split(&_str, MAP_DELIM);
    char *map_info = _str;

    a->name = malloc(sizeof(name)+1);
    strncpy(a->name, name, strlen(name)+1);
    a->mapping = malloc(sizeof(Mapping));
    
    int res = str_to_mapping(map_info, s, a->mapping, strlen(map_info));

    return res;

}

int alias_to_str(Alias *a, char *str, int n) {

    int offset = 0;
    int _en = 0;

    // write the name of the alias
    if ( (_en = write_at(str, a->name, offset, n-offset)) != 0) return _en;
    offset += strlen(a->name);

    // write a delimeter
    if ( (_en = write_at(str, MAP_DELIM, offset, n-offset)) != 0) return _en;
    offset += strlen(MAP_DELIM);

    // write the interpretation of the mapping
    mapping_to_str(a->mapping, str+offset, n-offset);

    // mapping_to_str() is 'best effort' (aka badly written) so we dont
    // actually know the offset we should continue writing at and
    // we can only hope everything went well :) 
    // however this is only used for debugging so its fine

    return 0;
}

void free_alias(Alias *a) {

    if (a != NULL) {

        // free the name
        if (a->name != NULL) {
            free(a->name);
        }

        // free the mapping
        if (a->mapping != NULL) {
            free(a->mapping);
        }

        // free the alias itself
        free(a);

    }

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
            } else if (n->type == ALIAS) {
                free_alias(n->alias);
            }
        }

        free(n);
    }
}

void free_tb(Testbench *tb) {

    if (tb != NULL) {

        // free the values
        if (tb->values != NULL) {

            for(int i=0; i<tb->uut->_inputc; i++) {
                free_str_list(tb->values[i], tb->v_c);
            }

            free(tb->values);
        }

        // free the output display list
        if (tb->outs_display != NULL) {
            free(tb->outs_display);
        }

        // free the tb itself
        free(tb);

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

    // add the new node to the library
    return ll_add(lib->contents, n);

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

    // set the lib type
    lib->type = GATE;

    // initialize the contents list pointer to null
    lib->contents = ll_init();

    // loop through the lines of the file and get the contents
    while((nread = read_line_from_file(&line, filename, &len, offset)) != -1) {

        if (strlen(line) != 0) {

            // if a line contains a gate declaration...
            if((starts_with(line, DECL_DESIGNATION))) {

                // ...parse the line into a new gate...
                Gate *g = malloc(sizeof(Gate));
                if ( (_en=str_to_gate(line, g, strlen(line), 1)) ) return _en; // strlen(line) and not nread, because they might differ (see read_line())

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

    lib->type = SUBSYSTEM;

    // initialize the contents list pointer to null
    lib->contents = ll_init();

    // loop through the lines of the file and get the contents
    while((nread = read_line_from_file(&line, filename, &len, offset)) != -1) {
        
        line_no++;
        offset += nread;
        int index = -1;

        int *comp_buffer_index = malloc(sizeof(int));  // the index of each parsed component in the simulation buffers
        *comp_buffer_index = 0;

        if (strlen(line) != 0) {    // if the line is empty, skip it

            // if a line contains a gate declaration a new subsystem must be parsed
            if((starts_with(line, DECL_DESIGNATION))) {

                // parse the first line into a subsystem header
                Subsystem *s = malloc(sizeof(Subsystem));
                s->is_standard = 1;
                s->components = ll_init();
                s->aliases = ll_init();
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
                        split(&_line, MAP_DELIM);

                        /*
                            _line now contains only the part after the delimiter, which is the signal
                            that is mapped to the (index)'th output of the subsystem
                        */
                        s->output_mappings[index] = malloc(strlen(_line)+1);
                        strncpy(s->output_mappings[index], _line, strlen(_line)+1);
                    
                        // also create the actual mapping
                        s->o_maps[index] = malloc(sizeof(Mapping));

                        // create the corresponding mapping
                        str_to_mapping(s->output_mappings[index], s, s->o_maps[index], strlen(s->output_mappings[index]));

                    }

                    // if it starts with a component declaration, create a component from it and add it to the subsystem
                    else if (starts_with(line, COMP_ID_PREFIX)) {
                        Component *c = malloc(sizeof(Component));
                        if ( (_en=str_to_comp(line, c, strlen(line), lookup_lib, s, 1, comp_buffer_index)) ) {
                            fprintf(stderr, "%s:%d: component parsing failed\n", filename, line_no);
                            return _en;
                        }
                        if ( (_en=subsys_add_comp(s, c)) ) return _en;
                    }

                    // if it is an alias (contains the delimiter while not being an output)
                    else if (strstr(line, MAP_DELIM)) {
                        
                        // store a copy of the line to be free to modify it and pass it to split
                        char *_line = line;

                        // create an alias
                        Alias *a = malloc(sizeof(Alias));
                        str_to_alias(_line, a, s, strlen(_line));
                        // add that alias to the subsystems aliases list
                        Node *n = malloc(sizeof(Node));
                        n->type = ALIAS;
                        n->alias = a;
                        n->next = NULL;
                        ll_add(s->aliases, n);
                    }

                    // else it should be an END ... NETLIST line
                    else if (starts_with(line, NETLIST_END)) {

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

                // the above loop only breaks if the netlist ended (otherwise - on error -  it returns)
                // so reaching here means a whole subsystem has been parsed

                // turn it into a standard
                Standard *std = malloc(sizeof(Standard));
                std->type = SUBSYSTEM;
                std->subsys = s;
                std->defined_in = lib;
                if ( (_en=add_to_lib(lib, std, 1, SUBSYSTEM)) ) return _en;
            }

        }

        free(comp_buffer_index);

    }

    free(line);

    return 0;
}

void free_lib(Netlist *lib) {

    if (lib != NULL) {

        // free the contents
        if (lib->contents != NULL) {
            ll_free(lib->contents, 1);
        }


        // free the filename
        if (lib->file != NULL) {
            free(lib->file);
        }

        // free the lib itself
        free(lib);
    }
}

int str_to_comp(char *str, Component *c, int n, Netlist *lib, Subsystem *s, int is_standard, int *buffer_index) {

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
    Standard *std = find_in_lib(lib, _name);
    if (std == NULL) {
        printf("Could not find component '%s' in library %s\n", _name, lib->file);
        return UNKNOWN_COMP;
    }
    c->prototype = std;

    // parse the inputs into the component
    c->inputs = NULL;   // initialize to NULL so initial call to realloc is like malloc
    c->_inputc = str_to_list(_raw_inputs, &(c->inputs), IN_OUT_DELIM);

    c->is_standard=is_standard;

    // set the buffer index and increment it
    c->buffer_index = (*buffer_index);
    (*buffer_index)++;

    // if needed, take care of the input mappings
    if ( (s!=NULL) && is_standard ) {
        
        // allocate space for the list
        c->i_maps = malloc(sizeof(Mapping*) * c->_inputc);

        // create each individual mapping
        for (int i=0; i<c->_inputc; i++) {

            // allocate space for each individual mapping
            c->i_maps[i] = malloc(sizeof(Mapping));

            // find it
            int _en;
            if ( (_en=str_to_mapping(c->inputs[i], s, c->i_maps[i], strlen(c->inputs[i]))) ) return _en;

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

    // add the new node to the subsystem
    return ll_add(s->components, n);

}

Node *move_in_list(int x, LList* list) {

    // move x positions in the list as long as they are valid
    Node *tmp = list->head;
    while(x > 0) {
        if (tmp == NULL) return NULL;
        tmp = tmp->next;
        x--;
    }

    return tmp;
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
    ns->components = ll_init();
    ns->aliases = NULL;
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
    Node *cur = std->subsys->components->head;
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
        if ( (_en=subsys_add_comp(ns, comp)) ) {
            return _en;
        }
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

void lib_to_file(Netlist *lib, char *filename, char *mode) {

    FILE *fp = fopen(filename, mode);

    // iterate through the contents of the library and print them
    for (Node *n = lib->contents->head; n!=NULL; n=n->next) {

        // handle each content according to its type
        if (n->std->type == GATE) {

            // allocate memory for a string (and clear it)
            char *buf = malloc(MAX_LINE_LEN);
            memset(buf, 0, MAX_LINE_LEN);

            // write the contents of the gate into that string
            gate_to_str(n->std->gate, buf, MAX_LINE_LEN);

            // write that string to the file and free it
            fprintf(fp, "%s\n", buf);
            free(buf);

        } else {

            // print the header line
            char *buf = malloc(MAX_LINE_LEN);
            subsys_hdr_to_str(n->std->subsys, buf, MAX_LINE_LEN);
            fprintf(fp, "%s\n", buf);
            free(buf);

            // print the BEGIN ... NETLIST line
            fprintf(fp, "BEGIN %s NETLIST\n", n->std->subsys->name);

            // print the components
            for (Node *n_n=n->std->subsys->components->head; n_n!=NULL; n_n=n_n->next) {

                Component *comp = n_n->comp;

                // print the component's ID and name
                fprintf(fp, "U%d %s ", comp->id, comp->prototype->gate->name);

                // print the component's inputs (as resolved mappings)
                for (int i=0; i<comp->_inputc; i++) {

                    // get the mapping
                    Mapping *m = comp->i_maps[i];

                    // resolve it
                    char *b = resolve_mapping(m, n->std->subsys);

                    // write the result of the resolution in the file
                    fprintf(fp, "%s", b);

                    // leave some space before the next one if needed
                    if ( !(i==comp->_inputc-1) ) {
                        fprintf(fp, " ");
                    }

                    // free any allocated memory
                    free(b);
                }

                fprintf(fp, "\n");
            }

            // print the output mappings
            for (int i=0; i<n->std->subsys->_outputc; i++) {
                
                // get the mapping
                Mapping *m = n->std->subsys->o_maps[i];

                // resolve it
                char *b = resolve_mapping(m, n->std->subsys);

                // write the result of the resolution in the file
                fprintf(fp, "%s = %s\n", n->std->subsys->outputs[i], b);

                // free any allocated memory
                free(b);
            }

            // print the END ... NETLIST line
            fprintf(fp, "END %s NETLIST\n", n->std->subsys->name);

        }
        
        // if this is not the last node, leave some lines blank
        if ( !(n->next==NULL) ) {
            fprintf(fp, "\n\n");
        }
    }

    fclose(fp);

}

void subsystem_to_file(Subsystem *s, char *filename, char *mode) {

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
    subsys_hdr_to_str(s, line, MAX_LINE_LEN);
    fprintf(fp, "%s\n", line);

    // write the "BEGIN NETLIST" line
    fprintf(fp, "BEGIN %s NETLIST\n", s->name);

    // write the components
    for (Node *c=s->components->head; c!=NULL; c=c->next) {
        comp_to_str(c->comp, line, MAX_LINE_LEN);
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
}

void s2f(Subsystem *s, FILE *fp) {

    // write the declaration line
    char *line = malloc(MAX_LINE_LEN);
    subsys_hdr_to_str(s, line, MAX_LINE_LEN);
    fprintf(fp, "%s\n", line);

    // write the "BEGIN NETLIST" line
    fprintf(fp, "BEGIN %s NETLIST\n", s->name);

    // write the components
    for (Node *c=s->components->head; c!=NULL; c=c->next) {
        comp_to_str(c->comp, line, MAX_LINE_LEN);
        fprintf(fp, "%s\n", line);
    }

    // write the output mappings
    for (int i=0; i<s->_outputc; i++) {
        fprintf(fp, "%s = %s\n", s->outputs[i], s->output_mappings[i]);
    }

    // write the "END NETLIST" line
    fprintf(fp, "END %s NETLIST\n", s->name);

    // cleanup
    free(line);
}

void netlist_to_file(Netlist *netlist, char *filename, char *mode) {

    // open the file
    FILE *fp;
    fp = fopen(filename, mode);
    if (fp == NULL) {
        perror("fopen");
        exit(-1);
    }

    // prepend a newline if needed
    if (starts_with(mode, "a")) {
        fprintf(fp, "\n");
        fclose(fp);
    }

    // iterate through the subsystems and write them to the file
    for(Node *n=netlist->contents->head; n!=NULL; n=n->next) {
        s2f(n->subsys, fp);

        // if this is not the last subsystem, leave a line blank
        if (n->next != NULL) {
            fprintf(fp, "\n");
        }
    }

    fclose(fp);
}

void mapping_to_str(Mapping *m, char *str, int n) {

    // print the mapping's info for debugging (comp x output y), not the corresponding connection
    if (m->type == SUBSYS_INPUT) {
        sprintf(str, "input %d", m->index+1);
    } else if (m->type == SUBSYS_COMP) {

        // write prelude
        int offset = sprintf(str, "component %d's ouput", m->index+1);

        // write output no if the component is not a gate (assuming out_index = -1 if refering to a gate)
        if ( !(m->out_index == -1) ) {
            sprintf(str+offset, " %d", (m->out_index)+1);
        }
    }
}

int str_to_mapping(char *str, Subsystem *subsys, Mapping *m, int n) {

    if (str == NULL || subsys == NULL || m == NULL) {
        return NARG;
    }

    // init variables that will be used in the resolution
    int input_index = -1, comp_index = -1, output_index = -1;
    Node *mapping_node;
    char *last = NULL;

    // keep the name of the referenced signal (the part after the delim)        
    char *referenced_signal = str;

    // find if it is an input, an alias or a component('s output) and proceed accordingly
    if ( (input_index=contains(subsys->_inputc, subsys->inputs, referenced_signal)) != -1 ) {

        // then the mapping refers to one of the subsystem's inputs
        m->type = SUBSYS_INPUT;
        m->index = input_index;
        m->out_index = -1;

    } else if ( (mapping_node = search_in_llist(subsys->aliases, ALIAS, referenced_signal, strlen(referenced_signal), -1, NULL)) != NULL) {

        // then the mapping refers to another mapping (an alias)
        Mapping *other_mapping = mapping_node->alias->mapping;

        // copy everything over (note that this allows us to have an arbitrary number of aliases referring to other aliases without any fancy recursion but uses more memory. this is a possible TODO)
        m->type = other_mapping->type;
        m->index = other_mapping->index;
        m->out_index = other_mapping->out_index;

    } else if (starts_with(referenced_signal, COMP_ID_PREFIX)) {

        // then the mapping refers to a component of the containing subsystem (or an ouput of one)        

        // the type is component either way
        m->type = SUBSYS_COMP;
        
        // get the id of the referenced subsystem as an integer (e.g. 35 in U35_COUT)
        int id = strtol(referenced_signal+1, &last, 10);  // strtol is amazing - https://man7.org/linux/man-pages/man3/strtol.3.html

        // find the component with that id
        if ( (mapping_node=search_in_llist(subsys->components, COMPONENT, NULL, -1, id, &comp_index)) == NULL) {
            printf("could not find component '%s%d' in subsystem '%s' (referenced as a mapping in '%s'\n)", COMP_ID_PREFIX, id, subsys->name, referenced_signal);
            return UNKNOWN_COMP;
        }

        // check that strtol worked properly and read some number
        if (last == referenced_signal) {
            fprintf(stderr, "error on resolving mapping '%s'\n", referenced_signal);
            return GENERIC_ERROR;
        }
        
        // set the index of the mapping
        m->index = comp_index;
        Component *comp = mapping_node->comp;

        // check if str continues (with the expected delimeter for an output) after the component ID
        if (!starts_with(last, MAP_COMP_OUT_SEP)) {

            // check that there is nothing after the component number (for example its not U35FOO)
            if (last[0] != 0) {
                fprintf(stderr, "invalid reference to '%s'\n", referenced_signal);
                return GENERIC_ERROR;
            }

            // if this is not a gate, there should be an output specified
            if (comp->prototype->type != GATE) {
                fprintf(stderr, "reference to '%s' which is a subsystem, but no output specified\n", referenced_signal);
                return GENERIC_ERROR;
            }

            // then it is a gate so index = -1
            m->out_index = -1;

        } else {

            // check that the referenced subsystem is not a gate (gates dont have outputs)            
            if (comp->prototype->type == GATE) {
                fprintf(stderr, "the input refers to a specific output of component '%s%d' which is a gate (%s)\n", COMP_ID_PREFIX, id, comp->prototype->gate->name);
                return GENERIC_ERROR;
            }

            // then it is a subsystem and also has output
            char *output_name = last+1;    // skip the underscore and go right to the output name ('_COUT' -> 'COUT')

            // look for the of the output in the outputs of the component
            if ( (output_index = contains(comp->prototype->subsys->_outputc, comp->prototype->subsys->outputs, output_name)) == -1) {
                fprintf(stderr, "the input refers to output '%s' of component '%s%d' but type '%s' has no such output\n", output_name, COMP_ID_PREFIX, id, comp->prototype->subsys->name);
                return GENERIC_ERROR;
            }

            // set the out_index of the mapping to the index of the referenced output
            m->out_index = output_index;

        }

    } else {    // error
        fprintf(stderr, "input refers to '%s' but it couldn't be resolved into an input, other component or alias!\n", referenced_signal);
        return GENERIC_ERROR;
    }

    return 0;

}

/**
 * @brief   Resolve m to the name of what it points to and write it to str.
 * 
 * @details The returned string is always null terminated, and just as long as
 *          it has to be.
 *
 *          The subsystem where the mapping was found (and thus withing which
 *          it must be resolved) should be passed to this function in s.
 * 
 * @example Let m = {type=SUBSYS_COMP, index=5, out_index=1}. If the 6'th component
 *          of s has ID 89 and its second output is named "BAR", str will be set to
 *          'U89_BAR'.
 * 
 * @param m     The mapping to be resolved
 * @param s     The subsystem within which the mapping will be resolved
 * 
 * @note    Note that the string returned from this will need freeing.
 * 
 * @retval  a pointer to the string on success (null terminated)
 * @retval  NULL on any error
 */
char* resolve_mapping(Mapping *m, Subsystem *s) {

    if (m == NULL || s == NULL) {
        return NULL;
    }

    // initialize variables that will be needed for the resolution
    char *b = NULL;

    // resolve the mapping according to its type
    if (m->type == SUBSYS_INPUT) {

        // if the mapping points to an input, just set b to point there
        b = malloc(strlen(s->inputs[m->index])+1);
        strncpy(b, s->inputs[m->index], strlen(s->inputs[m->index])+1);

    } else if (m->type == SUBSYS_COMP) {

        // find the node of the component that the mapping refers to
        Node *rtcn = move_in_list(m->index, s->components);    // referred-to component node
        
        // resolve the mapping according to its type
        if (rtcn->comp->prototype->type == GATE) {

            // allocate as much memory as we need
            b = malloc(strlen(COMP_ID_PREFIX)+digits(rtcn->comp->id)+1);

            // write the component id in the string (Uxx is enough if Uxx is a gate)
            sprintf(b, "%s%d", COMP_ID_PREFIX, rtcn->comp->id);

        } else if (rtcn->comp->prototype->type == SUBSYSTEM) {

            // allocate as much memory as we need
            b = malloc(
                strlen(COMP_ID_PREFIX)
                +digits(rtcn->comp->id)
                +strlen(MAP_COMP_OUT_SEP)
                +strlen(rtcn->comp->prototype->subsys->outputs[m->out_index])
                +1  // for the null byte
            );
            
            // wrtie the component id and the output name in the string (Uxx_yy)
            sprintf(b, "%s%d_%s", 
                COMP_ID_PREFIX,
                rtcn->comp->id,
                rtcn->comp->prototype->subsys->outputs[m->out_index]
            );

        }

    } else {

        // should never reach here, since all mappings map either to inputs or other component('s outputs)

        fprintf(stderr, "tried to resolve mapping  (within subsystem '%s') that does not map to input or component.\n", s->name);
        return NULL;
    }

    return b;
}

void lib_to_file_debug(Netlist *lib, char *filename, char *mode) {

    FILE *fp = fopen(filename, mode);

    // iterate through the contents of the library and print them
    for (Node *n = lib->contents->head; n!=NULL && n->std!=NULL; n=n->next) {

        // handle each content according to its type
        if (n->std->type == GATE) {

            // allocate memory for a string (and clear it)
            char *buf = malloc(MAX_LINE_LEN);
            memset(buf, 0, MAX_LINE_LEN);

            // write the contents of the gate into that string
            gate_to_str(n->std->gate, buf, MAX_LINE_LEN);

            // write that string to the file and free it
            fprintf(fp, "%s\n", buf);
            free(buf);

        } else {

            // print the header line
            char *buf = malloc(MAX_LINE_LEN);

            subsys_hdr_to_str(n->std->subsys, buf, MAX_LINE_LEN);
            fprintf(fp, "%s\n", buf);
            free(buf);

            // print the BEGIN ... NETLIST line
            fprintf(fp, "BEGIN %s NETLIST\n", n->std->subsys->name);

            // print the components
            for (Node *n_n=n->std->subsys->components->head; n_n!=NULL; n_n=n_n->next) {

                Component *comp = n_n->comp;
                fprintf(fp, "U%d %s ", comp->id, comp->prototype->gate->name);

                // all things in a library are standards so all components have i_maps
                for (int i=0; i<comp->_inputc; i++) {

                    // get the mapping
                    Mapping *m = comp->i_maps[i];

                    // get the mapping's info in a string
                    char *b = malloc(MAX_LINE_LEN);
                    mapping_to_str(m, b, MAX_LINE_LEN);

                    // write that string to the file
                    fprintf(fp, "%s", b);
                    free(b);

                    // leave a space before the next one (if there is a next one)
                    if ( !(i==comp->_inputc-1) ) {
                        fprintf(fp, ", ");
                    }
                }

                fprintf(fp, "(index: %d)\n", comp->buffer_index);
            }

            // print the output mappings
            for (int i=0; i<n->std->subsys->_outputc; i++) {
                
                // get the mapping
                Mapping *m = n->std->subsys->o_maps[i];

                // get the mapping's info in a string
                char *b = malloc(MAX_LINE_LEN);
                mapping_to_str(m, b, MAX_LINE_LEN);

                // write that string to the file
                fprintf(fp, "%s = %s\n", n->std->subsys->outputs[i], b);
                free(b);
            }

            // print the END ... NETLIST line
            fprintf(fp, "END %s NETLIST\n", n->std->subsys->name);

        }

        // if this is not the last node, leave some lines blank
        if ( !(n->next==NULL) ) {
            fprintf(fp, "\n\n");
        }
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
    Node *nd = std->components->head;
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

Component *instantiate_component(Standard *std, int id, char **inputs, int inputc) {

    Component *comp = malloc(sizeof(Component));

    // set the ID and the prototype of the new component
    comp->id = id;
    comp->prototype = std;

    // set the new component as non-standard and set the dynamic mappings to NULL
    comp->is_standard = 0;
    comp->i_maps = NULL;

    // copy the inputs
    comp->_inputc = inputc;
    deepcopy_str_list(&(comp->inputs), inputs, inputc);

    return comp;

}

Standard *find_in_lib(Netlist *lib, char *name) {
    
    Standard *ret = NULL;
    Node *nod = lib->contents->head;
    while(nod!=NULL) {
        if (strncmp(nod->std->subsys->name, name, strlen(name)) == 0) {
            ret = nod->std;
            return ret;
        }
        nod = nod->next;
    }

    printf("Error! Could not find a subsystem with the expected name (%s) in the subsystem library (%s)\n", name, lib->file);
    return NULL;

}

Node* search_in_llist(LList *list, enum NODE_TYPE t, char *str, int n, int id, int *index) {

    if (t != COMPONENT && str == NULL) {
        return NULL;
    }

    Node *cur = list->head;
    char *name = NULL;     // a pointer to the thing that will be compared to str

    // reset the index counter if needed
    if (index != NULL) *index = -1;

    // iterate through the list
    while (cur != NULL) {

        // increment the index counter if needed
        if (index != NULL) (*index)++;

        // keep a pointer to the next node
        Node *tmp = cur->next;

        // if the type matches
        if (cur->type == t) {

            // handle the case of a component (where we compare to the ID)
            if (t == COMPONENT) {
                if (id == cur->comp->id) {
                    return cur;
                }

                // skip the rest of the comparisons
                cur = tmp;
                continue;
            }

            // in any other case, set the name to what we want to compare str to
            if (t == STANDARD) {
                if (cur->std->type==GATE) {
                    name = cur->std->gate->name;
                } else {
                    name = cur->std->subsys->name;
                }
            } else if (t == SUBSYSTEM_N) {
                name = cur->subsys->name;
            } else if (t == ALIAS) {
                name = cur->alias->name;
            } else {
                return NULL;
            }

            // do the actual comparison and return if needed
            if (strncmp(name, str, n) == 0) {
                return cur;
            }

        }

        // proceed to the next node
        cur = tmp;

    }

    // we reach here only if the search didn't find anything
    return NULL;

}

int parse_truth_table(char *_tt) {

    int truth_table_bits = 0;
    for (int i=0; i<strlen(_tt); i++) {
        char c = _tt[i];
        
        // skip any non-bit characters
        if (c!='1' && c!='0') continue;

        truth_table_bits <<= 1;
        truth_table_bits += (c-'0');
    }

    return truth_table_bits;
}

int eval_at(int tt, char *inputs) {

    int index = 0;
    int n = strlen(inputs);
    for(int i=0; i<n; i++) {


        if (inputs[i]!='0' && inputs[i]!='1') {
            fprintf(stderr, "invalid bit '%c' passed to eval() function\n", inputs[i]);
            return -1; //TODO GENERIC ERROR
        }
        index <<= 1;
        index += (inputs[i]-'0');
    }

    int filter = one_at_index(2<<(n-1), index);

    int res = tt & filter;

    return res>>((int)(2<<(n-1))-1-index);
}

void print_as_truth_table(int tt, int inputs) {

    int max_n = 2<<(inputs-1);

    for(int i=0; i<inputs; i++) {
        fprintf(stderr, "%c | ", 'A'+i);
    }

    fprintf(stderr, " O\n");

    for(int i=0; i<(inputs*4)+2; i++) {
        fprintf(stderr, "_");
    }

    fprintf(stderr, "\n");

    for(int i=0; i<max_n; i++) {

        char *inps = malloc(inputs+1);

        for(int j=inputs-1; j>=0; j--) {
            fprintf(stderr, "%d | ", (i>>j)&1);
            inps[inputs-1-j] = ((i>>j)&1) + '0';
        }

        inps[inputs] = '\0';

        fprintf(stderr, " %d\t\n", eval_at(tt, inps));

        free(inps);
    }

}

int simulate(Subsystem* s, char *inputs, int *display_outs, FILE* fp) {


    clock_t _start = clock();    // measure time of execution - initial timestamp

    if (s == NULL || inputs==NULL) {
        return NARG;
    }

    // parse the inputs into a list
    char **l = NULL;
    int ic = str_to_list(inputs, &l, SIM_INPUT_DELIM);

    // check that the number of inputs is the expected one
    if (ic != s->_inputc) {
        fprintf(stderr, "simulation error: got %d inputs, expected %d\n", ic, s->_inputc);
        return GENERIC_ERROR;
    }

    int *int_inputs = malloc(sizeof(int)*s->_inputc);

    // make the inputs integers (so that they are usable in the simulation)
    for (int i=0; i<s->_inputc; i++) {
        
        // only check the first byte (suffices if everything is done right)
        char c = l[i][0];

        // check that it is a bit
        if (c!='1' && c !='0') {
            fprintf(stderr, "unexpected (non-bit) value found for input %s of subsystem %s: '%c'\n", s->inputs[i], s->name, c);
            return GENERIC_ERROR;
        }

        // convert to int
        int_inputs[i] = c - '0';
    }


    // find out how large the buffers need to be (#components + #outputs)
    int max_component_index = 0;
    for(Node *n=s->components->head; n!=NULL; n=n->next) {
        if (n->comp->buffer_index > max_component_index) {
            max_component_index = n->comp->buffer_index;
        }
    }

    // initialize the buffers
    int *buffer1 = malloc(sizeof(int) * ((max_component_index+1) + s->_outputc));
    int *buffer2 = malloc(sizeof(int) * ((max_component_index+1) + s->_outputc));

    memset(buffer1, 0, sizeof(int)*((max_component_index+1) + s->_outputc));
    memset(buffer2, 0, sizeof(int)*((max_component_index+1) + s->_outputc));

    int *old = buffer1;
    int *new = buffer2;

    // dirty flag (whether something changed this iteration or not - this is how we know when to break)
    int dirty = 1;

    int iterations = 0;

    clock_t start = clock();    // measure time of execution - initial timestamp

    while(dirty) {

        // unset the dirty flag so that it is only set if something changes
        dirty = 0;

        // increment the iteration counter
        iterations++;

        // iterate through the components
        for(Node *n=s->components->head; n!=NULL; n=n->next) {

            // cache a reference to the component for easy access
            Component *comp = n->comp;

            // if it is not a gate something is wrong
            if (comp->prototype->type != GATE) {
                fprintf(stderr, "unexpected component type! we can only simulate if all components are gates\n");
                return GENERIC_ERROR;
            }

            // find out the input values

            // clear the space where they will be stored
            char comp_ins[(comp->prototype->gate->_inputc) + 1];
            memset(comp_ins, 0, (comp->prototype->gate->_inputc) + 1);

            // get them from the old buffer according to the component's mappings
            for (int i=0; i<comp->prototype->gate->_inputc; i++) {

                // get the mapping for each input
                Mapping *m = comp->i_maps[i];

                if (m->type == SUBSYS_INPUT) {
                    
                    // this is the easy case, inputs dont change so just get the value from there
                    comp_ins[i] = int_inputs[m->index] + '0';

                } else if (m->type == SUBSYS_COMP) {

                    // find the component
                    Component *rtc = move_in_list(m->index, s->components)->comp; // referred-to component node
                    
                    // get its index in the buffer
                    int rtc_buf_index = rtc->buffer_index;
                    
                    // get its old value
                    comp_ins[i] = old[rtc_buf_index] + '0';
                } else {

                    // the mapping is neither to an input or a component - should never happen
                    fprintf(stderr, "unexpected error! found mapping that does not map to an input or component\n");
                    return GENERIC_ERROR;
                }

            }

            // find the truth value of the component with the retrieved inputs
            int new_val = eval_at(comp->prototype->gate->truth_table, comp_ins);

            // only replace the value if it differs from the old one, and also set the dirty flag
            if (new_val != new[comp->buffer_index]) {
                new[comp->buffer_index] = new_val;
                dirty = 1;
            }


        }


            // also iterate through the outputs
            for (int i=0; i<s->_outputc; i++) {
                
                int new_val;

                // get the mapping
                Mapping *m = s->o_maps[i];

                if (m->type == SUBSYS_INPUT) {
                    
                    // this is the easy case, inputs dont change so just get the value from there
                    new_val = int_inputs[m->index] + '0';

                } else if (m->type == SUBSYS_COMP) {

                    // find the component
                    Component *rtc = move_in_list(m->index, s->components)->comp; // referred-to component node
                    
                    // get its index in the buffer
                    int rtc_buf_index = rtc->buffer_index;
                    
                    // get its old value
                    new_val = old[rtc_buf_index];

                } else {

                    // the mapping is neither to an input or a component - should never happen
                    fprintf(stderr, "unexpected error! found mapping that does not map to an input or component\n");
                    return GENERIC_ERROR;
                }

                // only replace the value if it differs from the old one, and also set the dirty flag
                if (new_val != new[max_component_index+1+i]) {
                    new[max_component_index+1+i] = new_val;
                    dirty = 1;
                }


            }
        

        // swap the buffers
        int *tmp = old;
        old = new;
        new = tmp;


    }

    clock_t end = clock();  // final timestamp

    double actual_time = ((double) (end - start)) / CLOCKS_PER_SEC;
    double total_time = ((double) (end - _start)) / CLOCKS_PER_SEC;

    // print the results
    for(int i=0; i<s->_inputc; i++) {
        fprintf(fp!=NULL?fp:stderr, "%-5d", int_inputs[i]);
    }

    // print the delimiter
    fprintf(fp!=NULL?fp:stderr, "%-5c", '|');

    for(int i=0; i<s->_outputc; i++) {
        if (display_outs[i]) {
            fprintf(fp!=NULL?fp:stderr, "%-5d", old[max_component_index+1+i]);
        }
    }

    fprintf(fp!=NULL?fp:stderr, "\t [%d iterations, %.3f msec of iterating, %.3f msec in total]\n", iterations, actual_time*1000, total_time*1000);

    // cleanup
    free(int_inputs);
    free_str_list(l, ic);
    free(buffer1);
    free(buffer2);

    return 0;
}

int parse_tb_from_file(Testbench *tb, char *filename, char *mode) {

    FILE *fp = fopen(filename, mode);

    char *line = NULL;  // no need to malloc it, getline() does that for us (and also reallocs if needed) 
    int line_no = 0;
    int nread = 0;
    int offset = 0;
    size_t len = 0;

    // initialize the values field of the testbench struct
    tb->values = malloc(sizeof(char**) * tb->uut->_inputc);
    tb->outs_display = malloc(sizeof(int)*tb->uut->_outputc);
    memset(tb->outs_display, 0, sizeof(int)*tb->uut->_outputc);

    tb->v_c = -1;

    // loop through the lines of the file and get the contents
    while((nread = read_line_from_file(&line, filename, &len, offset)) != -1) {
        
        line_no++;
        offset += nread;

        if (strlen(line) != 0) {
            
            if (starts_with(line, TESTBENCH_IN)) {

                while (1) {

                    // read the next line
                    if ( (nread = read_line_from_file(&line, filename, &len, offset)) == -1 ) {
                        printf("%s:%d: Error! File ended unexpectedly\n",filename, line_no);
                        return UNEXPECTED_EOF;
                    }
                    line_no++;
                    offset += nread;

                    // check if we need to stop
                    if (starts_with(line, TESTBENCH_OUT)) break;

                    // keep a reference to be able to split
                    char *_line = line;

                    // split the line into its contents
                    char *name = split(&_line, TB_GENERAL_DELIM);
                    char *vals = _line;


                    int in_index = -1;
                    if ( (in_index = contains(tb->uut->_inputc, tb->uut->inputs, name)) == -1 ) {
                        fprintf(stderr, "unknown input in testbench! uut of type %s has no input named %s\n", tb->uut->name, name);
                        return GENERIC_ERROR;
                    }


                    // put the given values into a list
                    tb->values[in_index] = NULL;
                    int v_c = str_to_list(vals, &(tb->values[in_index]), TB_IN_VAL_DELIM);

                    if (tb->v_c == -1 || v_c < tb->v_c) {
                        tb->v_c = v_c;
                    }

                }
    
            } 
            
            if (starts_with(line, TESTBENCH_OUT)) {

                while ((nread = read_line_from_file(&line, filename, &len, offset)) != -1) {
                    
                    line_no++;
                    offset += nread;


                    int out_index = -1;
                    if ( (out_index = contains(tb->uut->_outputc, tb->uut->outputs, line)) == -1 ) {
                        fprintf(stderr, "unknown input in testbench! uut of type %s has no input named '%s'\n", tb->uut->name, line);
                        return GENERIC_ERROR;
                    }

                    tb->outs_display[out_index] = 1;

                }
            }

        }
    }

    free(line);
    fclose(fp);

    return 0;
}

int execute_tb(Testbench *tb, char *output_file, char *mode) {

    if (tb == NULL || output_file == NULL || mode == NULL) {
        return NARG;
    }

    // open the file
    FILE *fp = fopen(output_file, mode);
    if (fp == NULL) {
        fprintf(stderr, "execute_tb() encountered an error opening the file\n");
        return GENERIC_ERROR;
    }

    // print the header

    // print the inputs
    for (int i=0; i<tb->uut->_inputc; i++) {
        fprintf(fp, "%-5s", tb->uut->inputs[i]);
    }

    // print the delimiter
    fprintf(fp, "%-5c", '|');

    // print the outputs
    for(int i=0; i<tb->uut->_outputc; i++) {
        if (tb->outs_display[i]) {
            fprintf(fp, "%-5s", tb->uut->outputs[i]);
        }
    }

    // print a newline
    fprintf(fp, "\n");



    // iterate over the tests
    for (int test_no=0; test_no<tb->v_c; test_no++) {

        // get the inputs for each test in a format that simulate() understands
        char *inputs = malloc((tb->uut->_inputc)*3+1);    // <= 3 bytes per input: the value, a comma and a space

        // get the value of each input for the particular test number (assuming that the bit we want is the first in each value - the values are only strs because the function to parse a str into a list of strs was already written, technically they should be chars)
        for(int i=0; i<tb->uut->_inputc; i++) {
            sprintf(inputs+(i*3), "%c, ", tb->values[i][test_no][0]);
        }

        // hack to remove the last comma
        inputs[tb->uut->_inputc*3-2] = '\0';

        // call simulate
        simulate(tb->uut, inputs, tb->outs_display, fp);

        // test is over, free the inputs
        free(inputs);
    }

    fclose(fp);

    return 0;
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

    /*******************************************************************************************\
    *                                                                                           *
    * The process of translating a system to gates is basically the core of a CAD tool and thus *
    * deserves a text explanation to help in understanding the code.                            *
    *                                                                                           *
    * The following terms are used throughout this explanation:                                 *
    *      - the "target subsystem" is the subsystem that we seek to translate to gates         *
    *      - the "final subsystem" is the subsystem that we are creating, i.e. the one that     *
    *        only contains gates.                                                               *
    *      - "translated" refers to something (a subsystem mainly) that has been compiled down  *
    *        to gates.                                                                          *
    *                                                                                           *
    * The steps are roughly the following:                                                      *
    *      1) We initialize the final subsystem and copy into it the trivial details of         *
    *         the target (name, input and output names and counts)                              *
    *      2) We iterate through the components of the target subsystem. For each one:          *
    *          2.1) we resolve its input mappings. This process differs for each type of        *
    *               mapping:                                                                    *
    *                 2.1.1) if an input maps to an input of the (target) subsystem,            *
    *                        resolving it is trivial: we just fetch the name of that            *
    *                        particular input.                                                  *
    *                 2.1.2) if an input maps to an output of another component of the          *
    *                        (target) subsystem, we want to find the mapping for this output    *
    *                        in the final one. In order to do that we have to keep some         *
    *                        information relating the components of the target subsystem to     *
    *                        the gates that implement them in the final one. (We have to        *
    *                        somehow know that U35 has become {U3897, U3898, U3899, U3900}      *
    *                        in the final subsystem, and also that U35_COUT is mapped to        *
    *                        what is U3900 in the final subsystem).                             *
    *                        For details of how this is implemented see below (step 2.4).       *
    *          2.2) once we have its inputs (now resolved to an array of strings), we can       *
    *               create (an instance of) a subsystem of the same type, with those exact      *
    *               inputs. We want the components of this instance to be numbered according    *
    *               to the ID sequence of the final subsystem (see next steps to see why).      *
    *          2.3) we now add every component of the subsystem created in step 2.2) to the     *
    *               final subsystem.                                                            *
    *          2.4) we mentioned before that we have to somehow relate the subsystems of        *
    *               the target to the gates that implement them in the final. We achieve this   *
    *               by keeping every translated component in an intermediate list, so we can    *
    *               reference it through its position in the target (which is also its          *
    *               position in the list). That way we can access its mappings, which in turn   *
    *               contain the names of to its components, which are identical to the ones     * <- notice the "contains the name of" - we are looking at strings, not mappings
    *               in the final subsystem.                                                     *
    *               That way if we see a mapping to output x of component y of the target, we   *
    *               can find the x'th element of the intermediate list and resolve the mapping  *
    *               of its y'th output, effectively finding out the (final) ID of the gate      *
    *               that the mapping actually points to.                                        *
    *      3) We iterate through the output mappings of the target, resolving them              *
    *         following the same process as above. We set the corresponding output of each      *
    *         mapping to the result of that resolution.                                         *
    *                                                                                           *
    * A limitation of the above process (and a possible TODO) is that it assumes at most 1      *
    * level of depth: we can only translate subsystems that contain subsystems that are made    *
    * of gates, not other subsystems. For example, if we had the following setup:               *
    *  - a half adder made from gates                                                           *
    *  - a single bit full adder made from half adders                                          *
    *  - an n-bit full adder made from single bit full adders                                   *
    * we would not be able to translate the n-bit full adder to gates.                          *
    *                                                                                           *
    \*******************************************************************************************/


    // set the destination library info
    dest->contents = ll_init();
    dest->file = NULL;
    dest->type = SUBSYSTEM;

    // iterate over the contents of the netlist
    Node *node_ptr = netlist->contents->head;
    while (node_ptr!=NULL) {

        // this is the subsystem whose netlist we want to boil down to just gates
        Subsystem *target = node_ptr->std->subsys;

        // count the compponents in the target subsystem
        int subsys_count = 0;
        Node *_comp = target->components->head;
        while (_comp != NULL) {
            _comp = _comp->next;
            subsys_count++;
        }

        // allocate space for the new, gate only subsystem
        Subsystem *only_gates_sub = malloc(sizeof(Subsystem));
        only_gates_sub->aliases = NULL;

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
        only_gates_sub->components = ll_init();


        // every translated component will be added to this intermediate list so
        // that any references to the output of a component can be resolved
        LList *intermediate_components = ll_init();

        // iterate over the components of the old subsystem, resolving input mappings
        // like UXX_S0 to the gate that is mapped to output S0 of component XX
        _comp = target->components->head;

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
                    Node *_n = move_in_list(map->index, intermediate_components);

                    if (_n->type == SUBSYSTEM_N) {
                        
                            // take advantage of the hack we did when putting the subsystems in the intermediate components (search for COMP_HACK)
                            Subsystem *_s = (Subsystem *)(_n->subsys);

                            // check that the out_index is valid
                            if (map->out_index < 0) {
                                char *b = malloc(BUFSIZ);
                                mapping_to_str(map, b, BUFSIZ);
                                fprintf(stderr, "invalid output index %d in mapping '%s' to subsystem of type '%s'\n", map->out_index, b, _s->name);
                                free(b);
                                return GENERIC_ERROR;
                            }
                            
                            // find the corresponding output mapping
                            char *m = _s->output_mappings[map->out_index];

                            // put it in the input list
                            inputs[i] = malloc(strlen(m)+1); // +1 for the null byte
                            strncpy(inputs[i], m, strlen(m)+1);

                        
                    } else {

                        Component *c = _n->comp;

                        inputs[i] = malloc(strlen(COMP_ID_PREFIX)+digits(c->id)+2);
                        sprintf(inputs[i], "%s%d", COMP_ID_PREFIX, c->id);
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
            */
            
            fprintf(stderr, "inputs for U%d, %s are the following: ", comp->id, comp->prototype->subsys->name);
            for (int i=0; i<comp->_inputc; i++) {
                fprintf(stderr, "%s ", inputs[i]);
            }
            fprintf(stderr, "\n");
            
            /*
                we now have a complete list of inputs for each component
                we need to create the netlist of that component with the inputs we now have
                and add it to the subsystems array, unless it is a gate, in which case we just
                have to add it to the new subsystem (the gate-only one). but then how will we
                resolve mappings that need that gate's output? add an else in line 1319
            */
            if (comp->prototype->type == GATE) {

                // create a gate component just like the one in the target subsystem
                Component *nc = malloc(sizeof(Component));
                nc->i_maps = NULL;
                nc->id = component_id++;
                nc->_inputc = deepcopy_str_list(&(nc->inputs), inputs, comp->_inputc);
                nc->is_standard = 0;
                nc->prototype = comp->prototype;

                // add it to the gates-only subsystem
                subsys_add_comp(only_gates_sub, nc);

                // also add it to the list of components of the target subsystems, so
                // that any references to it can be resolved
                Node *nn = malloc(sizeof(Node));
                nn->type = COMPONENT;
                nn->comp = nc;
                nn->next = NULL;
                ll_add(intermediate_components, nn);

            } else {


                Subsystem *just_translated = malloc(sizeof(Subsystem));

                component_id = create_custom(just_translated, comp->prototype, comp->_inputc, inputs, component_id);
                

                // free the input list
                for(int i=0; i<comp->_inputc; i++) free(inputs[i]);

                // add every gate to the gate-only subsystem, freeing the used nodes in the process
                Node *c_c = just_translated->components->head;
                while (c_c != NULL) {
                    subsys_add_comp(only_gates_sub, c_c->comp);
                    Node *tmp = c_c->next;
                    char *b = malloc(BUFSIZ);
                    comp_to_str(c_c->comp, b, BUFSIZ);
                    free(b);
                    c_c = tmp;
                }

                // also add it to the list of components of the target subsystems, so
                // that any references to it can be resolved
                Node *nn = malloc(sizeof(Node));
                nn->type = SUBSYSTEM_N;
                nn->subsys = just_translated;    // -----: components cannot represent subsystems and putting the subsystem in the prototype field would be a violation of the field's logic, so we just use C's very static typing instead
                nn->next = NULL;
                ll_add(intermediate_components, nn);

            }

            FILE *fp = fopen("koka", "w");
            Node *c = only_gates_sub->components->head;
            while(c!=NULL) {
                
                char *b = malloc(BUFSIZ);
                comp_to_str(c->comp, b, BUFSIZ);
                fprintf(fp, "%s\n", b);
                free(b);
                c = c->next;
            }
            fclose(fp);

            // move on to the next subsystem
            _comp = _comp->next;
            for(int i=0; i<comp->_inputc; i++){
                //free(inputs[i]);
            }
            free(inputs);

        }
        
        // we have now filled the subsystems array with subsystems whose components are gates and numbered consistently

        // now we need to map the outputs of target to gate only things
        only_gates_sub->output_mappings = malloc(sizeof(char*) * only_gates_sub->_outputc); // +1 for the null byte

        for(int i=0; i<target->_outputc; i++) {
            
            Mapping *om = target->o_maps[i];

            // resolve the mapping according to its type
            if (om->type == SUBSYS_INPUT) {

                // then all we need to do is put the name of the input in the output mapping list
                only_gates_sub->output_mappings[i] = malloc(strlen(target->inputs[om->index])+1); // +1 for the null byte
                strncpy(only_gates_sub->output_mappings[i], target->inputs[om->index], strlen(target->inputs[om->index])+1);

            } else if (om->type == SUBSYS_COMP) {

                // find out the component that the mapping maps to, which will now be in intermediate_components
                Node *rtcn = move_in_list(om->index, intermediate_components); // referred-to component node

                if (rtcn->type == COMPONENT) {  // then, according to the hack (search COMP_HACK) it is a gate
                    fprintf(stderr, "tha doume ti tha kanoume\n");
                    
                    // then all we need to do is put the ID of the gate in the output mapping list (it is consistent with the numbering of the final)
                    only_gates_sub->output_mappings[i] = malloc(strlen(COMP_ID_PREFIX)+digits(rtcn->comp->id)+1); // +1 for the null byte
                    sprintf(only_gates_sub->output_mappings[i], "%s%d", COMP_ID_PREFIX, rtcn->comp->id);

                } else if (rtcn->type == SUBSYSTEM_N) {

                    // use the hack (search COMP_HACK)
                    Subsystem *mapped = (Subsystem*)(rtcn->comp);

                    // check that the out_index is valid
                    if (om->out_index < 0) {
                        char *b = malloc(BUFSIZ);
                        mapping_to_str(om, b, BUFSIZ);
                        fprintf(stderr, "invalid output index %d in mapping '%s' to subsystem of type '%s'\n", om->out_index, b, mapped->name);
                        free(b);
                        return GENERIC_ERROR;
                    }

                    // find the corresponding output mapping
                    char *m = mapped->output_mappings[om->out_index];

                    // put it in the output list
                    only_gates_sub->output_mappings[i] = malloc(strlen(m)+1); // +1 for the null byte
                    strncpy(only_gates_sub->output_mappings[i], m, strlen(m)+1);
                }
            } else {

                // should never reach here, since all mappings map either to inputs or other component('s outputs)
                char *b = malloc(BUFSIZ);
                mapping_to_str(om, b, BUFSIZ);
                fprintf(stderr, "invalid mapping type: %s\n", b);
                free(b);
                return GENERIC_ERROR;
            }

        }

        // add the gate-only subsystem to the destination library
        int _en=0;
        if ( (_en=add_to_lib(dest, only_gates_sub, 0, SUBSYSTEM)) ) return _en;

        // cleanup the array that was used for the components of this subsystem
        Node *n = intermediate_components->head;
        while (n!=NULL) {
            Node *t = n->next;
            if (n->type == SUBSYSTEM_N) {
                free_subsystem(n->subsys, 0);
            } else if (n->type == COMPONENT) {
                // dont free
            } else {
                printf("nothing...?\n");
            }
            free(n);
            n = t;
        }

        free(intermediate_components);
        // advance to the next subsystem to be translated
        node_ptr = node_ptr->next;


    }
    return 0;
}

void old_lib_to_file(Netlist *lib, char *filename, char *mode, int mod) {

    FILE *fp = fopen(filename, mode);

    // print the stuff
    for (Node *n = lib->contents->head; n!=NULL; n=n->next) {

        // print the header line
        char *buf = malloc(MAX_LINE_LEN);
        subsys_hdr_to_str(n->subsys, buf, MAX_LINE_LEN);
        fprintf(fp, "%s\n", buf);
        free(buf);

        // print the BEGIN ... NETLIST line
        fprintf(fp, "BEGIN %s NETLIST\n", n->subsys->name);

        // print the components
        for (Node *n_n=n->subsys->components->head; n_n!=NULL; n_n=n_n->next) {


            Component *comp = n_n->comp;
            fprintf(fp, "U%d %s ", comp->id, comp->prototype->gate->name);

            for (int i=0; i<comp->_inputc; i++) {
                fprintf(fp, "%s ", comp->inputs[i]);
            }
            fprintf(fp, "\n");
            if(comp->id % mod == 0) {
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

