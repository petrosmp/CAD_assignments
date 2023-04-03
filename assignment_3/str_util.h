/**
 * String operations to make everything easier.
 * 
 * Also a bit of file stuff.
*/
#define NES 1   /* Error # meaning not enough space. */
#define NARG -1 /* Error # meaning null argument(s). */

#ifndef COMMENT_PREFIX
#define COMMENT_PREFIX "%%"         /* The prefix of any comment line */
#endif

#ifndef KEYWORD_PREFIX
#define KEYWORD_PREFIX "**"         /* The prefix of any keyword line */
#endif

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
int write_at(char* dest, char* src, int offset, int n);


/**
 * Write lc members of l (separated by delim) into dest, starting at offset. 
 * Writes no more than n bytes, not null terminated.
 * Sets b_w to the number of total bytes written.
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
int write_list_at(int lc, char** l, char* delim, char* dest, int offset, int n, int* b_w);

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
char *split(char **str, char *delim);

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
 * does not include the newline character).
 * 
 * If a line contains a COMMENT_PREFIX or a KEYWORD_PREFIX, the rest of the line is
 * omited (a null byte is added where the prefix was).
 * 
 * Ignores whitespace before and after the "meaningful" part of each line. For example,
 * the following line:
 * "   COMP NOT ; IN: P        ** THE LINE WITH THE %% COMMENT"
 * Will store the following string in the given buffer:
 * "COMP NOT ; IN: P"
 * 
 * In that case, nread is the number of bytes read from the file, but the string contained
 * in *line is not of the same length. Checking the length of each string is therefore a
 * good way to make sure no segmentation faults or stack smashings occur.
 * 
*/
int read_line_from_file(char **line, char *filename, size_t *len, int offset);

/**
 * Check if s1 starts with s2. Basically a wrapper to strncmp(s1, s2, strlen(s2)).
 * 
 * If s1 is shorter than s2 this returns false.
 * 
*/
int starts_with(char *s1, char *s2);

/**
 * Check if s1 starts with any of the strings in list (that has l_c
 * items). If a match is found, return the index of the matching
 * string, otherwise return -1.
*/
int index_starts_with(char *s1, char **list, int l_c);

/**
 * Split str into substrings separated by delim and store them in l.
 * Returns the number of items written. Allocates memory for each
 * element of l and for l itself (meaning l can be NULL, and any
 * contents will be overwriten and lost).
 * 
 * Each element of l and l itself will eventually need to be freed
 * by the caller.
 * 
 * split() will be called on str, so it will be modified. Keeping a
 * pointer to it (for example to free it later) is suggested.
*/
int str_to_list(char *str, char ***l, char *delim);

/**
 * Remove the starting and trailing whitespace from a line of text.
 * 
 * The given string is assumed to end with a null terminator, which is preserved,
 * but any excess whitespace between it and the last non-whitespace character
 * is omited.
 * 
 * Any whitespace (" " or "\t") before the first non-whitespace character is
 * also ommited.
*/
int trim_line(char **line);

/**
 * Check if a given string is contained in a given list of l_c strings.
 * 
 * Returns the index of the string in the list if it is found, -1
 * otherwise.
*/
int contains(int l_c, char **list, char *str);

/**
 * Returns the number of digits that x has.
*/
int digits(int x);
