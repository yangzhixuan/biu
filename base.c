#include <stdio.h>
#include <math.h>

typedef struct closureType{
    void *func;
    void *env;
} closure;


static double add_func(void* env, double a, double b)
{
    return a+b;
}
closure add = {add_func, (void*)0};


static double sub_func(void* env, double a, double b)
{
    return a-b;
}
closure sub = {sub_func, (void*)0};


static double mul_func(void* env, double a, double b)
{
    return a*b;
}
closure mul = {mul_func, (void*)0};


static double div_func(void* env, double a, double b)
{
    return a/b;
}
closure div = {div_func, (void*)0};

static char equal_func(void* env, double a, double b)
{
    return a==b;
}
closure equal = {equal_func, (void*)0};


static double printnumber_func(void* env, double a)
{
    double ret = (double)printf("%f\n", a);
    fflush(stdout);
    return ret;

}
closure print = {printnumber_func, (void*)0};

static double sin_func(void* env, double a)
{
    return sin(a);
}
closure __sin = {sin_func, (void*)0};
