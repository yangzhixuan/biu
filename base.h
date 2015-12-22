#ifndef __BASE_H
#define __BASE_H

typedef struct closureType{
    void *func;
    void *env;
} closure;

#endif
