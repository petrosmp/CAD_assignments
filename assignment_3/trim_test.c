#include <stdio.h>
#include <stdlib.h>
#include "str_util.h"

#define FILENAME "component.lib"
#define SIZE 100

int main() {

    
    char *filename = FILENAME;

    char *line = NULL;  // no need to malloc it, getline() does that for us (and also reallocs if needed) 
    int nread = 0;
    int offset = 0;
    size_t len = 0;

    // loop through the lines of the file and get the contents
    while((nread = read_line_from_file(&line, filename, &len, offset)) != -1) {

        printf("1: read line [%s], bytes before comment: %d\n", line, nread);

        // move the cursor
        offset += nread;
    }

    free(line);

    return 0;
}
