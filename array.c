#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "base.h"

typedef struct array_tag{
    void *mem;
    size_t num_ele;
    size_t type_size;
}array;

void __make_array_func(array* a, double num_ele_d, size_t type_size)
{
    if(num_ele_d < 0 || (size_t)num_ele_d != num_ele_d) {
        fprintf(stderr, "invalid array length: %f\n", num_ele_d);
        exit(1);
    }
    a->num_ele = (size_t)num_ele_d;
    a->type_size = type_size;

    a->mem = malloc(a->num_ele * a->type_size);
    if(a->mem == NULL) {
        perror("make_array");
    }
}

void* __getelement_func(array* a, double n_d)
{
    long n = (long) n_d;
    if(n != n_d || (n >= 0 && n >= (long)a->num_ele) 
                || (n < 0  && n <  (long)a->num_ele)) {
        fprintf(stderr, "invalid array index: %f\n", n_d);
        exit(1);
    }

    if(n < 0) {
        n += a->num_ele;
    }
    return a->mem + (n * a->type_size);
}

