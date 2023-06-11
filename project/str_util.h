/**
 * @file    str_util.h
 * 
 * @author  Petros Bimpiris (pbimpiris@tuc.gr)
 * 
 * @brief   String operations to make everything easier. Also a bit of file stuff. 
 * 
 * @version 0.1
 * 
 * @date 11-06-2023
 * 
 * @copyright Copyright (c) 2023
 * 
 */


#define NES 1   /**< Error # meaning not enough space. */
#define NARG -1 /**< Error # meaning null argument(s). */

#ifndef COMMENT_PREFIX
#define COMMENT_PREFIX "%%"         /**< The prefix of any comment line */
#endif

#ifndef KEYWORD_PREFIX
#define KEYWORD_PREFIX "**"         /**< The prefix of any keyword line */
#endif

/**
 * @brief   Write no more than n bytes from src into dest starting at offset
 * 
 * @details Assumes src is null terminated, overwrites potential contents of
 *          dest.
 * 
 *          No null termination is added to dest, unless there is one in the
 *          first n bytes of src.
 *          If NES is returned, contents of dest are undefined.
 * 
 * @param dest      The string to which the source data will be written
 * @param src       The data that will be written to the destination string
 * @param offset    The offset at which the data will be written to dest
 * @param n         The maximum number of bytes that will be written to dest
 * 
 * @retval 0 on success
 * @retval NARGS if any argument is null
 * @retval NES if there is not enough space in dest to fit the requested items
 */
int write_at(char* dest, char* src, int offset, int n);

/**
 * @brief   Write lc members of l (separated by delim) into dest, starting at offset. 
 *          Writes no more than n bytes, not null terminated.
 *          Sets b_w to the number of total bytes written.
 * 
 * @details Assumes that the delim and all members of l are null terminated.
 *          Does not null terminate.
 *          If NES is returned, contents of dest are undefined.
 * 
 * @param lc        The number of elements of l that will be written to dest
 * @param l         The array whose elements will be written to dest
 * @param delim     The delimiter that will separate the elements of l once written in dest    
 * @param dest      The string to which the data will be written
 * @param offset    The offset at which the data will be written in dest
 * @param n         The maximum number of bytes written to dest
 * @param b_w       A pointer to where the number of bytes written shall be stored
 * 
 * @retval 0 on sucess
 * @retval NARGS if any argument is null
 * @retval NES if there is not enough space in dest to fit the requested items
 */
int write_list_at(int lc, char** l, char* delim, char* dest, int offset, int n, int* b_w);

/**
 * @brief   Split str into tokens based on delim
 * 
 * @details If delim is found then the part before it will be returned and str
 *          will be set to the part after it.
 *
 *          If delim is not found the whole str is returned and str is set to
 *          NULL.
 * 
 *          The pointer arithmetic done here was inspired by glibc's implementation
 *          of strtok() and strsep().
 * 
 * @note    In order to not have any memory leaks, before passing a string to be
 *          split store its initial address in a pointer, so it can then be properly
 *          freed. Note that freeing in that way will also free the memory where
 *          all the tokens point to, so make sure to be done with them before freeing.
 * 
 * @param str   The (address of the) string that will be split
 * @param delim The delimiter at which the string will be split
 * 
 * @return (a pointer to) the start of the string that ends just before the delimiter 
 */
char *split(char **str, char *delim);

/**
 * @brief   Reads from the file with the given filename starting at offset and until a newline
 *          character is found. Stores the line in the buffer pointed to by line.
 * 
 * @details Ultimately a wrapper to getline() that allows us to pass an offset to it. Handles
 *          all file operations, will exit if there is an error when opening the file.
 * 
 *          Due to getline()'s magic, if line is null, a buffer of size len will be allocated.
 *          If it cannot fit the read line, it is extended until it can. This buffer NEEDS to
 *          be freed by the caller.
 * 
 *          See getline()'s manpage for info about the stored line (it is null terminated and
 *          does not include the newline character).
 * 
 *          If a line contains a COMMENT_PREFIX or a KEYWORD_PREFIX, the rest of the line is
 *          omited (a null byte is added where the prefix was).
 * 
 *          Ignores whitespace before and after the "meaningful" part of each line. For example,
 *          the following line:
 *          <code>"   COMP NOT ; IN: P        ** THE LINE WITH THE %% COMMENT"</code>
 *          Will store the following string in the given buffer:
 *          <code>"COMP NOT ; IN: P"</code>
 * 
 *          In that case, nread is the number of bytes read from the file, but the string contained
 *          in *line is not of the same length. Checking the length of each string is therefore a
 *          good way to make sure no segmentation faults or stack smashings occur.
 * 
 * @param line      The buffer where the read line will be stored
 * @param filename  The file from which the line will be read
 * @param len       A pointer to where the length of the read line shall be saved
 * @param offset    The offset at which the reading will start in the given file (if offset!=0, fseek()
 *                  will be used to get there)
 * @return  The number of bytes read, -1 on error
 */
int read_line_from_file(char **line, char *filename, size_t *len, int offset);

/**
 * @brief   Check if s1 starts with s2. Basically a wrapper to strncmp(s1, s2, strlen(s2)).
 * 
 * @param s1    The string that may start with the other
 * @param s2    The string with which the other may start
 * 
 * @retval 1 if true
 * @retval 0 if false
 */
int starts_with(char *s1, char *s2);

/**
 * @brief   Check if s1 starts with any of the strings in list (that has l_c
 *          items). If a match is found, return the index of the matching
 *          string, otherwise return -1.
 * 
 * @param s1    The string that may start with any of the elements of list
 * @param list  The list of things that s1 may start with
 * @param l_c   The number of elements in list
 * @return the index of the matching string in list, otherwise -1
 */
int index_starts_with(char *s1, char **list, int l_c);

/**
 * @brief   Split str into substrings separated by delim and store them in l.
 * 
 * @note    Allocates memory for each element of l and for l itself (meaning l
 *          can be NULL, and any contents will be overwriten and lost).
 * 
 * @note    Each element of l and l itself will eventually need to be freed
 *          by the caller.
 * 
 * @note    split() will be called on str, so it will be modified. Keeping a
 *          pointer to it (for example to free it later) is suggested.
 * 
 * @param str   The string that will be split into substrings that will be stored in l
 * @param l     The list where the substrings of str will be stored0
 * @param delim The delimiter on which str will be split
 * @return  Returns the number of items written.
 */
int str_to_list(char *str, char ***l, char *delim);

/**
 * @brief   Remove the starting and trailing whitespace from a line of text.
 * 
 * @details The given string is assumed to end with a null terminator, which is preserved,
 *          but any excess whitespace between it and the last non-whitespace character
 *          is omited.
 * 
 *          Any whitespace (" " or "\t") before the first non-whitespace character is
 *          also ommited.
 * 
 * @param line  The line from which the whitespace will be removed
 * @return 0 on success
 */
int trim_line(char **line);

/**
 * @brief       Check if a given string is contained in a given list of l_c strings.
 * 
 * @param l_c   The number of elements in list
 * @param list  The list in which str may be contained
 * @param str   The string that may be contained in list
 * 
 * @return      The index of the string in the list if it is found, -1 otherwise.
 */
int contains(int l_c, char **list, char *str);

/**
 * @brief   Returns the number of digits that x has.
 * 
 * @param x     The number whose number of digits will be returned
 * @return The number of digits that x has  
 */
int digits(int x);

/**
 * @brief   Append no more than l2_c contents of of l2 to l1.
 *          l1 is modified, l2 is left as is.
 *  
 * @note    any items of l1 after l1_c will be overwritten (but properly
 *          freed).
 * 
 * 
 * @param l1    The list to which l2 will be appended
 * @param l1_c  The offset at which l2 will be appened to l1
 * @param l2    The list that will be appended to l1
 * @param l2_c  The number of elements of l2 that will be appended to l1
 * 
 * @return Return the number of items written on success, NARG on failure
 * because of null args. 
 */
int list_concat(char*** l1, int l1_c, char **l2, int l2_c);


/**
 * @brief   Properly free a list of strings
 *
 * @param l     The list of strings to be freed
 * @param lc    The number of strings in the list
 */
void free_str_list(char **l, int lc);

/**
 * @brief   Allocate memory for n strings at dst, and copy the contents
 *          (not references) of src to dst.
 * 
 * @param dst   The address where the copied list will be stored
 * @param src   The list that will be copied
 * @param n     The number of elements of src that will be copied to dst
 * 
 * @return  the number of items copied over on success, NARG if
 *          there are null arguments. 
 */
int deepcopy_str_list(char ***dst, char **src, int n);

/**
 * @brief   Given a series of bits as a string (char array), return the decimal number
 *          that those bits represent.
 * 
 * @param bits  The series of bits to be converted to decimal.
 * @return The decimal representation of the number that the bits form.
 */
int decimal(char *bits);

/**
 * @brief   Convert a given integer to its binary represenation.
 * 
 * @note    The returned pointer is malloc()'ed in here, so remember to free it.
 * 
 * @param x     The integer to be converted to binary.
 * @return      The binary representation of the given integer.
 */
char* binary(int x);

/**
 * @brief   Create a bit string of the given size that has a '1' n places to the right of
 *          the MSB and all other bits are 0. The created bitstring can then be AND'ed with
 *          another to only preserve a specific bit of the latter.
 * 
 * @details That means n=0 will only have '1' at the MSB and n=(size-1) will only preserve
 *          the LSB.
 * 
 * @example one_at_index(5, 0) will return a bitstring of size 5, with a 1 in the MSB's place,
 *          so '10000' (which is 16 in decimal).
 * 
 * @example one_at_index(6, 3) will return a bitstring of size 6, with a three places to the
 *          right of the MSB, so '000100' (which is 4 in decimal).
 * 
 * @example one_at_index(4, 3) will return a bitstring of size 4, with a three places to the
 *          right of the MSB, so '0001' (which is 1 in decimal).
 * 
 * @param size  The size of the desired bitstring.
 * @param n     The number of places to the right of the MSB where the 1 should be.
 * @return      The resulting bitstring as an integer
 */
int one_at_index(int size, int n);
