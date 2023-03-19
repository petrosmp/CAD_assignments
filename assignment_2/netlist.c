#include "netlist.h"

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

    // free s itself
    free(s);
}

int subsys_to_str(Subsystem* s, char* str, int n) {

    if (s==NULL || str==NULL) {
        return NARG;
    }

    int offset = 0;
    int _en;

    // write the first word, indicating that this is a component
    if( (_en = write_at(str, COMP_DESIGNATION, offset, strlen(COMP_DESIGNATION))) ) return _en;
    offset += strlen(COMP_DESIGNATION);

    // write the name of the component
    if( (_en = write_at(str, s->name, offset, strlen(s->name))) ) return _en;
    offset += strlen(s->name);

    // write the default delimiter
    if( (_en = write_at(str, DEFAULT_DELIM, offset, strlen(DEFAULT_DELIM))) ) return _en;
    offset += strlen(DEFAULT_DELIM);

    // write the input designation
    if( (_en = write_at(str, INPUT_DESIGNATION, offset, strlen(INPUT_DESIGNATION))) ) return _en;
    offset += strlen(INPUT_DESIGNATION);

    // write the inputs, separated by delimiter
    int list_bytes = 0;
    _en = write_list_at(s->_inputc, s->inputs, IN_OUT_DELIM, str, offset, n-1-offset, &list_bytes);
    if (_en) return _en;
    offset += list_bytes;

    // write the default delimiter
    if( (_en = write_at(str, DEFAULT_DELIM, offset, strlen(DEFAULT_DELIM))) ) return _en;
    offset += strlen(DEFAULT_DELIM);

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

int str_to_subsys(char *str, Subsystem *s, int n) {

    // any line declaring a subsystem starts with COMP_DESIGNATION which we ignore
    int offset = strlen(COMP_DESIGNATION);

    /* because split alters its first argument, and we need to keep
       the original address of str so we can free it, we use an internal
       variable to pass to split
    */
    char *_str = str+offset;

    // strtok interprets delim as a collection of delimiters, so if even one of them is
    // found, it splits. we need to split if ALL OF THEM are found CONSECUTIVELY.
    // so we create what we need (see split()).

    // get the fields of the line
    char *name        = split(&_str, DEFAULT_DELIM);
    char *raw_inputs  = split(&_str, DEFAULT_DELIM);
    char *raw_outputs = split(&_str, DEFAULT_DELIM);

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

int read_subsystem_from_file(char *filename, Subsystem **s) {
    
    if (filename == NULL || (*s)==NULL) {
        return NARG;
    }
    
    char *line = NULL;  // no need to malloc it, getline() does that for us (and also reallocs if needed) 

    int nread = 0;
    int offset = 0;
    size_t len = 0;

    // there is actually no need for a loop for the time being but it doesn't hurt
    while((nread = read_line_from_file(&line, filename, &len, offset)) != -1) {

        // if a line contains a subsystem declaration
        if((starts_with(line, COMP_DESIGNATION))) {

            // remove the newline character
            char *nl = strpbrk(line, "\n");
            *nl = '\0';

            // parse the line into s
            str_to_subsys(line, *s, nread);
        }

        // move the cursor
        offset += nread;
    }

    free(line); // this should be done here tho

    // allocate and set s->source
    (*s)->source = malloc(strlen(filename)+1);
    strncpy((*s)->source, filename, strlen(filename)+1);

    return 0;
}
