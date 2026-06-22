//
// Created by juntao, at wuhan university   on 2020/10/18.
//

#ifndef BAMBOO_CALLABLE_H
#define BAMBOO_CALLABLE_H
#include <iostream>
#include <functional>
#include <map>
#include <list>
#include <cstdarg>
#include "CallBackCenter.h"
namespace iono
{
    class Callable
    {
    public:
        Callable(void *in) { m_ptr = in; }
        ~Callable();
        void m_register(std::string key, Task);
        void m_remove(std::string, Task);

    protected:
        void m_remove_native(std::string, Task);
        friend void CallBackCenter::m_remove(Callable *);
        std::map<std::string, std::list<Task>> m_tasks;
        void *m_ptr;
    };
}
#endif // BAMBOO_CALLABLE_H
