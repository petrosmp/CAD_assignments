#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "str_util.h"

int write_at(char* dest, char* src, int offset, int n) {
    int i=0;

    if (dest==NULL || src==NULL) {
        return NARG;
    }

    while (1) {

        if (i > n) {
            return NES;
        } 
        if (src[i] == '\0') {
            break;
        }

        dest[offset+i] = src[i];
        i++;
    }

    return 0;
}

int write_list_at(int lc, char** l, char* delim, char* dest, int offset, int n, int* b_w) {
    
    int _offset = offset;
    int _en;

    if (l==NULL || delim==NULL || dest==NULL) {
        return NARG;
    }

    for(int i=0; i<lc; i++) {

        // check if the element exists
        if (l[i] == NULL) return NARG;

        // check if we can write the next element
        if ((_offset-offset) + strlen(l[i]) > n) return NES;

        // write the element
        if( (_en=write_at(dest, l[i], _offset, strlen(l[i]))) ) return _en;
        _offset += strlen(l[i]);
        *b_w = _offset-offset;


        // check if a delim is needed
        if (i != lc-1) {

            // check if we can write the delim
            if ((_offset-offset) + strlen(delim)> n) return NES;

            // write the delim
            if( (_en=write_at(dest, delim, _offset, strlen(delim))) ) return _en;
            _offset += strlen(delim);
            *b_w = _offset-offset;
        }
    }

    return 0;
}

char *split(char **str, char *delim) {

    char *begin, *end;

    begin = *str;
    if (begin == NULL) return NULL;

    if((end = strstr(*str, delim))){
        /* if delim is found, insert a null byte there and advance str */
        *end = '\0';
        *str = end+strlen(delim);
    } else {
        /* if this is the last token set str to NULL */
        *str = NULL;
    }

    return begin;
}

int read_line_from_file(char **line, char *filename, size_t *len, int offset) {

    FILE *fp;
    int nread;

    // open the file
    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        exit(-1);
    }

    // seek to offset and read a line
    fseek(fp, offset, SEEK_SET);
    nread = getline(line, len, fp);

    fclose(fp);

    return nread;
}

int starts_with(char *s1, char *s2) {

    if (strlen(s1) < strlen(s2)) return 0;

    if (strncmp(s1, s2, strlen(s2)) == 0) return 1;
    
    return 0;
}

int str_to_list(char *str, char ***l, char *delim) {

    char *cur;
    int i=0;

    while(str != NULL) {

        // get the list item
        cur = split(&str, delim);
        
        // allocate memory for the list entry
        (*l) = realloc((*l), sizeof(char*)*(i+1));
        if ((*l) == NULL) {
            fprintf(stderr, "malloc() error! not enough memory!\n");
            exit(-1);
        }

        // allocate memory for the item (plus a null byte)
        (*l)[i] = malloc(strlen(cur)+1);
        if ((*l)[i] == NULL) {
            fprintf(stderr, "malloc() error! not enough memory!\n");
            exit(-1);
        }

        // copy the item into the table
        strncpy((*l)[i], cur, strlen(cur)+1);  // strncpy will insert null bytes in empty space

        i++;
    }

    return i;
}
