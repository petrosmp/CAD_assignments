#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int one_at_index(int size, int n);
char* binary(int x);
int decimal(char *bits);
int parse_truth_table(char *_tt);
void print_as_truth_table(int tt, int inputs);


int main(int argc, char *argv[]) {


    int a = atoi(argv[1]);
    int b = atoi(argv[2]);
    int c = atoi(argv[3]);

    int inp1 = 0;
    int inp2 = 1;

    char inps[] = {inp1, inp2, '\0'};

    char *_truth_table = "1, 1, 1, 0";

    int truth_table_bits = parse_truth_table(_truth_table);

    printf("the given truth table corresponds to the integer %d (%s)\n", truth_table_bits, binary(truth_table_bits));

    printf("make_and(%d, %d) = %d\n", a, b, one_at_index(a, b));

    printf("%d in binary is %s\n", c, binary(c));

    printf("and in decimal it is %d\n", decimal(binary(c)));

    print_as_truth_table(truth_table_bits, 2);

    return 0;
}

/**
 * @brief   Given a series of bits as a string (char array), return the decimal number
 *          that those bits represent.
 * 
 * @param bits  The series of bits to be converted to decimal.
 * @return The decimal representation of the number that the bits form.
 */
int decimal(char *bits) {
    int res = 0;

    for (int i=0; i<strlen(bits); i++) {
        res <<= 1;
        res += (bits[i] - '0');
    }

    return res;
}

/**
 * @brief   Convert a given integer to its binary represenation.
 * 
 * @note    The returned pointer is malloc()'ed in here, so remember to free it.
 * 
 * @param x     The integer to be converted to binary.
 * @return      The binary representation of the given integer.
 */
char* binary(int x) {

    int n = sizeof(int) * 8;
    char* bits = (char*)malloc(n + 1);
    bits[n] = '\0';

    for (int i = n - 1; i >= 0; i--) {
        bits[i] = (x & 1) ? '1' : '0';
        x = x >> 1;
    }

    return bits;
}

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
int one_at_index(int size, int n) {
    int res = 0;
    res = (1<<((size-1)-n));
    return res;
}

/**
 * @brief   Given a string representing a truth table of a gate (in the format described in the project
 *          specification), make it a bitstring and return it (in the form of an integer).
 * 
 * @note    Any non-bit characters (not '1' or '0') will be skipped.
 * 
 * @example     Suppose the input is '0, 1, 1, 0'. The bitstring would then be '0110', which means that
 *              the number 6 will be returned.
 * 
 * @example     Suppose that the input is '0, 1'. The bitstring would then be '01', which means that the
 *              number 1 will be returned.
 * 
 * @param _tt   The string containing the truth table.
 * @return      The bitstring representing the given truth table.
 */
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

/**
 * @brief   Given a truth table in bitstring (integer) form and a set of inputs
 *          as an array of characters, return a truth value (as an integer).
 * 
 * @note    The inputs array must be null terminated.
 * 
 * @param tt        The truth table.
 * @param inputs    The inputs.
 * @return The truth value of the table with the given inputs (1 or 0)
 */
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

    int filter = one_at_index(pow(2, n), index);

    int res = tt & filter;

    return res>>((int)pow(2, n)-1-index);
}

void print_as_truth_table(int tt, int inputs) {

    int max_n = pow(2, inputs);

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

        int filter = one_at_index(pow(2, inputs), i);

        int res = tt & filter;

        fprintf(stderr, " %d\t\n", eval_at(tt, inps));

    }

}
