#include <stdio.h>

typedef struct closureType{
    void *func;
    void *env;
} closure;

double add_func(void* env, double a, double b)
{
    return a+b;
}

closure add = {add_func, (void*)0};

double printnumber_func(void* env, double a)
{
    double ret = (double)printf("%f\n", a);
    fflush(stdout);
    return ret;

}
closure print = {printnumber_func, (void*)0};
