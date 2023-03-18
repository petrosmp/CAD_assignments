#include "netlist.h"

void free_subsystem(Subsystem *s) {

    // free inputs
    for(int i=0; i<s->_inputc; i++) {
        free(s->inputs[i]);
    }
    free(s->inputs);

    // free outputs
    for(int i=0; i<s->_outputc; i++) {
        free(s->outputs[i]);
    }
    free(s->outputs);

    // free name?

    // free source?

    // free s
    free(s);
}

int subsys_to_str(Subsystem* s, char* str, int n) {

    if (s==NULL || str==NULL) {
        return NARG;
    }

    int offset = 0;
    int _en;

    // write the first word, indicating that this is a component
    if(_en = write_at(str, COMP_DESIGNATION, offset, strlen(COMP_DESIGNATION))) return _en;
    offset += strlen(COMP_DESIGNATION);

    // write the name of the component
    if(_en = write_at(str, s->name, offset, strlen(s->name))) return _en;
    offset += strlen(s->name);

    // write the default delimiter
    if(_en = write_at(str, DEFAULT_DELIM, offset, strlen(DEFAULT_DELIM))) return _en;
    offset += strlen(DEFAULT_DELIM);

    // write the input designation
    if(_en = write_at(str, INPUT_DESIGNATION, offset, strlen(INPUT_DESIGNATION))) return _en;
    offset += strlen(INPUT_DESIGNATION);

    // write the inputs, separated by delimiter
    int list_bytes = 0;
    _en = write_list_at(s->_inputc, s->inputs, IN_OUT_DELIM, str, offset, n-1-offset, &list_bytes);
    if (_en) return _en;
    offset += list_bytes;

    // write the default delimiter
    if(_en = write_at(str, DEFAULT_DELIM, offset, strlen(DEFAULT_DELIM))) return _en;
    offset += strlen(DEFAULT_DELIM);

    // write the output designation
    if(_en = write_at(str, OUTPUT_DESIGNATION, offset, strlen(OUTPUT_DESIGNATION))) return _en;
    offset += strlen(OUTPUT_DESIGNATION);

    // write the outputs, separated by delimiter
    list_bytes = 0;
    _en = write_list_at(s->_outputc, s->outputs, IN_OUT_DELIM, str, offset, n-1-offset, &list_bytes);
    if (_en) return _en;
    offset += list_bytes;

    // manually null terminate the string
    str[offset] = '\0';

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
    int _inputc = 0;
    int _outputc = 0;
    char *cur;

    while(_inputs != NULL) {

        // get the name of the input
        cur = split(&_inputs, IN_OUT_DELIM);
        
        // allocate memory for the entry in the input table
        s->inputs = realloc(s->inputs, sizeof(char*)*(_inputc+1));
        printf("allocated %d bytes for inputs\n",  sizeof(char*)*(_inputc+1));
        if (s->inputs == NULL) {
            fprintf(stderr, "malloc() error! not enough memory!\n");
            exit(-1);
        }

        // allocate memory for the input's name (plus a null byte)
        s->inputs[_inputc] = malloc(strlen(cur)+1);
        if (s->inputs[_inputc] == NULL) {
            fprintf(stderr, "malloc() error! not enough memory!\n");
            exit(-1);
        }

        // copy the name of the input into the table
        strncpy(s->inputs[_inputc], cur, strlen(cur)+1);  // strcpy will insert null bytes in empty space
        
        _inputc++;
    }

    s->_inputc = _inputc;

    while(_outputs != NULL) {

        // get the name of the output
        cur = split(&_outputs, IN_OUT_DELIM);
        
        // allocate memory for the entry in the output table
        s->outputs = realloc(s->outputs, sizeof(char*)*(_outputc+1));
        printf("allocated %d bytes for outputs\n",  sizeof(char*)*(_outputc+1));

        if (s->outputs == NULL) {
            fprintf(stderr, "malloc() error! not enough memory!\n");
            exit(-1);
        }

        // allocate memory for the output's name (plus a null byte)
        s->outputs[_outputc] = malloc(strlen(cur)+1);
        if (s->outputs[_outputc] == NULL) {
            fprintf(stderr, "malloc() error! not enough memory!\n");
            exit(-1);
        }

        // copy the name of the output into the table
        strncpy(s->outputs[_outputc], cur, strlen(cur)+1);  // strcpy will insert null bytes in empty space
        
        _outputc++;
    }

    s->_outputc = _outputc;


    // printfs for testing

    printf("The name is: [%s]\n", name);
    printf("Read %d inputs:", s->_inputc);
    for(int i=0; i<s->_inputc; i++) {
        printf(" [%s]", s->inputs[i]);
    }
    printf("\n");
    printf("Read %d outputs:", s->_outputc);
    for(int i=0; i<s->_outputc; i++) {
        printf(" [%s]", s->outputs[i]);
    }
    printf("\n");


    return 0;
}
