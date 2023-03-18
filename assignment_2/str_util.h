/**
 * String operations to make everything easier.
 * 
 * Also a bit of file stuff.
*/

#include <stdio.h>
#include <string.h>

#define NES 1 /* Error # meaning not enough space. */
#define NARG -1 /* Error # meaning null argument(s). */

/**
 * Write no more than n bytes from src into dest starting at offset
 * 
 * Assumes src is null terminated, overwrites potential contents of
 * dest.
 * 
 * Returns:
 *  - NARGS if any argument is null
 *  - NES if there is not enough space in dest to fit the requested items
 * 
 * No null termination is added to dest, unless there is one in the
 * first n bytes of src.
 * If NES is returned, contents of dest are undefined.
*/
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

/**
 * Write lc members of l (separated by delim) into dest, starting at offset.
 * Writes no more than n bytes, not null terminated.
 * sets b_w to the number of total bytes written.
 * 
 * 
 * Assumes that the delim and all members of l are null terminated.
 * 
 * Returns:
 *  - NARGS if any argument is null
 *  - NES if there is not enough space in dest to fit the requested items
 * 
 * Does not null terminate.
 * If NES is returned, contents of dest are undefined.
*/
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
        if(_en=write_at(dest, l[i], _offset, strlen(l[i]))) return _en;
        _offset += strlen(l[i]);
        *b_w = _offset-offset;

        // check if we can write the delim
        if ((_offset-offset) + strlen(delim)> n) return NES;

        // write the delim if needed
        if (i != lc-1) {
            if(_en=write_at(dest, delim, _offset, strlen(delim))) return _en;
            _offset += strlen(delim);
            *b_w = _offset-offset;
        }
    }

    return 0;
}

/**
 * Split str into tokens based on delim.
 * 
 *  - If delim is found then the part before it will be returned and str
 *    will be set to the part after it.
 *  - If delim is not found the whole str is returned and str is set to
 *    NULL
 * 
 * In order to not have any memory leaks, before passing a string to be
 * split store its initial address in a pointer, so it can then be properly
 * freed. Note that freeing in that way will also free the memory where
 * all the tokens point to, so make sure to be done with them before freeing.
 * 
 * The pointer arithmetic done here was inspired by glibc's implementation
 * of strtok() and strsep().
*/
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

/**
 * Reads from the file with the given filename starting at offset and until a newline
 * character is found. Stores the line in the buffer pointed to by line.
 * 
 * Ultimately a wrapper to getline() that allows us to pass an offset to it. Handles
 * all file operations, will exit if there is an error when opening the file.
 * 
 * Due to getline()'s magic, if line is null, a buffer of size len will be allocated.
 * If it cannot fit the read line, it is extended until it can. This buffer NEEDS to
 * be freed by the caller.
 * 
 * See getline()'s manpage for info about the stored line (it is null terminated and
 * includes the newline character).
*/
int read_line_from_file(char **line, char *filename, size_t *len, int offset) {

    FILE *fp;
    int nread;

    // open the file
    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // seek to offset and read a line
    fseek(fp, offset, SEEK_SET);
    nread = getline(line, len, fp);

    fclose(fp);

    return nread;
}

/**
 * Check if s1 starts with s2. Basically a wrapper to strncmp(s1, s2, strlen(s2)).
 * 
 * If s1 is shorter than s2 this returns false.
 * 
*/
int starts_with(char *s1, char *s2) {

    if (strlen(s1) < strlen(s2)) return 0;

    if (strncmp(s1, s2, strlen(s2)) == 0) return 1;
    
    return 0;
}
