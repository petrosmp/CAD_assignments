#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "str_util.h"

#define FILENAME "sample_input.txt"
#define SIZE 100

#define ENTITY_START "ENTITY"       /* The string that indicates that an entity declaration starts in this line */
#define ENTITY_END "END"            /* The string that indicates that an entity declaration ends in this line */
#define VAR_DECLARATION "VAR"       /* The string that indicates that a line contains a variable declaration */
#define VAR_ASSIGNMENT "= "         /* The string that lies between a variables name and its value */
#define PORT_START "PORT ("         /* The string that indicates that a port map begins in this line */
#define PORT_END ");"               /* The string that indicates that a port map begins in this line */
#define PORT_MAP_INPUT "IN"         /* The string that indicates that a port map line contains an input signal */
#define PORT_MAP_OUTPUT "OUT"       /* The string that indicates that a port map line contains an output signal */
#define PORT_MAP_DELIM " "          /* The delimiter that separates fields in a port map line */
#define PORT_MAP_SIGNAL_DELIM " , " /* The delimiter that separates (input/output) signals in a port map */
#define PORT_MAP_COLON ": "         /* The delimiter between the input/output declarations and the signal names */

int main() {

    
    char *filename = FILENAME;

    char *line = NULL;  // no need to malloc it, getline() does that for us (and also reallocs if needed) 
    int nread = 0;
    int offset = 0;
    size_t len = 0;

    char **input_names = NULL;
    int input_names_c = 0;
    char **output_names = NULL;
    int output_names_c = 0;

    int n;
    char **input_signals = NULL;
    int input_signals_c = 0;
    char **output_signals = NULL;
    int output_signals_c = 0;

    
    char **buf = NULL;  // buffer to store the "useful" part of a line (the one with no declarations)
    int buf_len = 0;
    char *name = NULL;
    char *tmp;

    // loop through the lines of the file and get the contents
    while((nread = read_line_from_file(&line, filename, &len, offset)) != -1) {

        char *_line = line;

        if (starts_with(_line, ENTITY_START)) {
            _line += strlen(ENTITY_START)+1;        // skip the declaration and the following space
            tmp = split(&_line, PORT_MAP_DELIM);    // keep the part until the next space
            
            // allocate memory for the name and save it
            name = malloc(strlen(tmp)+1);   
            strncpy(name, tmp, strlen(tmp)+1);

        } else if (starts_with(line, PORT_START)) {

            /* this is necessary because there might be something useful (an input declaration for example)
               in the same line as the port map declaration.
            */

            _line += strlen(PORT_START)+1;  // skip the port map declaration and the following space

        } else if (starts_with(line, VAR_DECLARATION)) {

            _line += strlen(VAR_DECLARATION)+1;     // skip the variable declaration keyword and the following space
            
            tmp = split(&_line, PORT_MAP_DELIM);    // skip the name of the variable
            tmp = split(&_line, PORT_MAP_DELIM);    // skip the space before the equal sign
            tmp += strlen(VAR_ASSIGNMENT);          // skip the equal sign
            n = atoi(tmp);  // this should be implemented differently, with a vars-vals structure... (TODO)

        }

        if (starts_with(_line, PORT_MAP_INPUT)) {
            _line += strlen(PORT_MAP_INPUT)+1;  // skip the declaration and the following space
            
            // get the input name/prefix (for example A)
            tmp = split(&_line, PORT_MAP_DELIM);

            // get the input signals (in a list)
            buf=NULL;
            buf_len = str_to_list(_line+strlen(PORT_MAP_COLON), &buf, PORT_MAP_SIGNAL_DELIM);

            // allocate memory for the input names
            input_names = realloc(input_names, sizeof(char*) * (input_names_c+buf_len));

            // if the signals are more than one it means the input is a bit vector which we need to enumerate
            if (buf_len > 1) {

                for (int i=buf_len-1; i>=0; i--) {              // we go from n-1 to 0 inclusive
                    char *a = malloc(strlen(tmp)+digits(i)+2);  // allocate memory for the prefix, the number and a null byte

                    // "create" the enumerated name (for example A3, A2, A1, etc.)
                    sprintf(a, "%s%d", tmp, i);

                    // add it to the list
                    input_names[input_names_c] = malloc(strlen(a)+1);
                    strncpy(input_names[input_names_c++], a, strlen(a)+1);

                    free(a);
                }

            } else {    // otherwise it is just a single bit, just add it to the list
                input_names[input_names_c] = malloc(strlen(tmp)+1);
                strncpy(input_names[input_names_c++], tmp, strlen(tmp)+1);
            }


            // append the buffer list to the input list
            input_signals_c += list_concat(&input_signals, input_signals_c, buf, buf_len);

            free_str_list(buf, buf_len);

        } else if (starts_with(_line, PORT_MAP_OUTPUT)) {
            
            _line += strlen(PORT_MAP_OUTPUT)+1;  // skip the declaration and the following space
            
            // get the output name/prefix (for example A)
            tmp = split(&_line, PORT_MAP_DELIM);

            // get the output signals (in a list)
            buf=NULL;
            buf_len = str_to_list(_line+strlen(PORT_MAP_COLON), &buf, PORT_MAP_SIGNAL_DELIM);

            // allocate memory for the output names
            output_names = realloc(output_names, sizeof(char*) * (output_names_c+buf_len));

            // if the signals are more than one it means the output is a bit vector which we need to enumerate
            if (buf_len > 1) {

                for (int i=buf_len-1; i>=0; i--) {              // we go from n-1 to 0 inclusive
                    char *a = malloc(strlen(tmp)+digits(i)+2);  // allocate memory for the prefix, the number and a null byte

                    // "create" the enumerated name (for example A3, A2, A1, etc.)
                    sprintf(a, "%s%d", tmp, i);

                    // add it to the list
                    output_names[output_names_c] = malloc(strlen(a)+1);
                    strncpy(output_names[output_names_c++], a, strlen(a)+1);

                    free(a);
                }

            } else {    // otherwise it is just a single bit, just add it to the list
                output_names[output_names_c] = malloc(strlen(tmp)+1);
                strncpy(output_names[output_names_c++], tmp, strlen(tmp)+1);
            }


            // append the buffer list to the output list
            output_signals_c += list_concat(&output_signals, output_signals_c, buf, buf_len);

            free_str_list(buf, buf_len);

        }

        // move the cursor
        offset += nread;
    }


    free(line);

    // we got it all
    printf("Name is %s, n is %d.\n", name, n);

    // check input names equals input signals

    for(int i=0; i<input_names_c; i++) {
        printf("Input #%d: name:{%s}, mapping: {%s}\n", i+1, input_names[i], input_signals[i]);
    }

    // check output names equals output signals

    for(int i=0; i<output_names_c; i++) {
        printf("Output #%d: name:{%s}, mapping: {%s}\n", i, output_names[i], output_signals[i]);
    }

    free_str_list(input_names, input_names_c);
    free_str_list(input_signals, input_signals_c);
    free_str_list(output_names, output_names_c);
    free_str_list(output_signals, output_signals_c);
    free(name);

    return 0;

}
