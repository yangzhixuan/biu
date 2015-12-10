#ifndef __COMMON_H
#define __COMMON_H

class Error{
    public:
        std::string msg;
        Error(std::string str) : msg(str) {}
};

#endif
