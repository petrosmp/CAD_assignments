#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "str_util.h"
#include "netlist.h"


int main(int argc, char *argv[]) {


    //int a = atoi(argv[1]);
    //int b = atoi(argv[2]);
    //int c = atoi(argv[3]);

    char *_truth_table = "1, 1, 1, 1";

    int truth_table_bits = parse_truth_table(_truth_table);

    printf("the given truth table corresponds to the integer %d (%s)\n", truth_table_bits, binary(truth_table_bits));

    //printf("make_and(%d, %d) = %d\n", a, b, one_at_index(a, b));

    //printf("%d in binary is %s\n", c, binary(c));

    //printf("and in decimal it is %d\n", decimal(binary(c)));

    print_as_truth_table(truth_table_bits, 2);

    return 0;
}
