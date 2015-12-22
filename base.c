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
closure sinclosure = {sin_func, (void*)0};

static double cos_func(void* env, double a)
{
    return cos(a);
}
closure cosclosure = {cos_func, (void*)0};

static char getchar_func(void* env)
{
    return (char)getchar();
}
closure getcharclosure = {getchar_func, (void*)0};

static double putchar_func(void* env, char a)
{
    double ret = (double)putchar(a);
    fflush(stdout);
    return ret;
}
closure putcharclosure = {putchar_func, (void*)0};

static double chartonumber_func(void* env, char a)
{
    return (double)a;
}
closure chartonumberclosure = {chartonumber_func, (void*)0};

static char numbertochar_func(void* env, double a)
{
    int inta = (int)a;
    if (a == inta)
    {
        if (inta >= 0 && inta <= 255)
        {
            return (char)inta;
        }
        fprintf(stderr, "%d is not in 0~255\n", inta);
        exit(1);
    }
    else
    {
        fprintf(stderr, "%f is not int\n", a);
        exit(1);
    }
    fprintf(stderr, "unknown error\n");
    exit(1);
}
closure numbertocharclosure = {numbertochar_func, (void*)0};
