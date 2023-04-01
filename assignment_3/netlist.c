#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "netlist.h"
#include "str_util.h"

void free_subsystem(Subsystem *s) {

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

    // free source
    if (s->source != NULL) {
        free(s->source);
    }

    // free component list
    if (s->components != NULL) {
        for (int i=0; i<s->_componentc; i++) {
            if (s->components[i] != NULL) {
                free_component(s->components[i]);
            }
        }
        free(s->components);
    }

    // free output mapping list
    if (s->output_mappings != NULL) {
        for (int i=0; i<s->_outputc; i++) {
            if (s->output_mappings[i] != NULL) {
                free(s->output_mappings[i]);
            }
        }
        free(s->output_mappings);
    }

    // free s itself
    free(s);
}

int subsys_to_ref_str(Subsystem* s, char* str, int n) {

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

int str_to_subsys_ref(char *str, Subsystem *s, int n) {

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

int read_ref_subsystem_from_file(char *filename, Subsystem **s) {
    
    if (filename == NULL || (*s)==NULL) {
        return NARG;
    }
    
    char *line = NULL;  // no need to malloc it, getline() does that for us (and also reallocs if needed) 

    int nread = 0;
    int offset = 0;
    size_t len = 0;
    int _en;

    // there is actually no need for a loop for the time being but it doesn't hurt
    while((nread = read_line_from_file(&line, filename, &len, offset)) != -1) {

        // if a line contains a subsystem declaration...
        if((starts_with(line, DECL_DESIGNATION))) {

            // ...find the newline character, replace it with a null byte...
            char *nl = strpbrk(line, "\n");
            *nl = '\0';

            // ...and parse the line into s
            if ( (_en=str_to_subsys_ref(line, *s, nread)) ) return _en;
        }

        // move the cursor
        offset += nread;
    }

    free(line);

    // allocate and set s->source
    (*s)->source = malloc(strlen(filename)+1);
    strncpy((*s)->source, filename, strlen(filename)+1);

    // set s->type and functional subsystem fields
    (*s)->type = REFERENCE;
    (*s)->components = NULL;
    (*s)->output_mappings = NULL;

    return 0;
}

void free_component(Component *c) {

    // free inputs
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
    if ( (_en=write_at(str, c->standard->name, offset, strlen(c->standard->name))) ) return _en;
    offset += strlen(c->standard->name);

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
    if ( (_en=subsys_to_ref_str(s, line, MAX_LINE_LEN)) ) {
        free(line);
        return _en;
    }
    fprintf(fp, "%s\n", line);

    // write the "BEGIN NETLIST" line
    fprintf(fp, "BEGIN %s NETLIST\n", s->name);

    // write the components
    for (int i=0; i<s->_componentc; i++) {
        if ( (_en=comp_to_str(s->components[i], line, MAX_LINE_LEN)) ) {
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
            free_subsystem(s->subsys);
        }

        free(s);
    }
}

void free_node(Node *n) {

    if (n != NULL) {
        free_standard(n->std);
        free(n);
    }
}

int add_to_lib(Library *lib, Standard* s) {

    if (lib==NULL || s==NULL) {
        return NARG;
    }

    // make the standard into a node
    Node *n = malloc(sizeof(Node));
    n->type = STANDARD;
    n->std = s;
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

int gate_lib_from_file(char *filename, Library* lib) {
    
    if (filename == NULL || lib==NULL) {
        return NARG;
    }
    
    char *line = NULL;  // no need to malloc it, getline() does that for us (and also reallocs if needed) 

    int nread = 0;
    int offset = 0;
    size_t len = 0;
    int _en;

    // save the filename
    lib->file = malloc(strlen(filename));
    strncpy(lib->file, filename, strlen(filename)+1);

    // loop through the lines of the file and get the contents
    while((nread = read_line_from_file(&line, filename, &len, offset)) != -1) {

        // if a line contains a gate declaration...
        if((starts_with(line, DECL_DESIGNATION))) {

            // ...find the newline character, replace it with a null byte...
            char *nl = strpbrk(line, "\n");
            *nl = '\0';

            // ...parse the line into a new gate...
            Gate *g = malloc(sizeof(Gate));
            if ( (_en=str_to_gate(line, g, nread)) ) return _en;

            // ...create a standard from that new gate...
            Standard *s = malloc(sizeof(Standard));
            s->type = GATE;
            s->gate = g;
            s->defined_in = lib;

            // ...and add the standard to the library
            if ( (_en=add_to_lib(lib, s)) ) return _en;
        }

        // move the cursor
        offset += nread;
    }

    free(line);

    return 0;
}

void free_lib(Library *lib) {

    if (lib != NULL) {

        // free the contents
        if (lib->contents != NULL) {
            Node *cur = lib->contents;
            while (cur != NULL) {
                lib->contents = cur->next;
                free_node(cur);
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

