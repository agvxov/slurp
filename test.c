// @BAKE gcc -o $*.out $@ -Wall -lcriterion -O0 -ggdb
#include "slurp.h"
#include <criterion/criterion.h>

Test(read_file, test) {
    char * s = read_file("test.txt");
    cr_assert(!strcmp(s, "This is my test input file.\n"));
}

Test(overwrite_file, test) {
    cr_assert(!overwrite_file("test.out.txt", "test"));
}

Test(write_file, test) {
    unlink("test.out.txt");
    cr_assert(!write_file("test.out.txt", "test"));
}

Test(write_stdout, test) {
    cr_assert(!overwrite_file("/dev/stdout", "test"));
}
