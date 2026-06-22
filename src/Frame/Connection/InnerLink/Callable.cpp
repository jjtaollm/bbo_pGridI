//
// Created by juntao, at wuhan university   on 2020/10/18.
//
#include "include/Frame/Frame.h"
using namespace iono;
Callable::~Callable()
{
    /* delete from the callback center */
    CallBackCenter::m_remove(this);
}
void Callable::m_register(std::string key, Task task)
{
    bool bexist = false;
    if (m_tasks.find(key) == m_tasks.end())
    {
        m_tasks[key] = std::list<Task>();
    }
    for (const auto &kv : m_tasks[key])
    {
        if (kv.target<void (*)(std::map<std::string, std::string>)>() ==
            task.target<void (*)(std::map<std::string, std::string>)>())
        {
            bexist = true;
            break;
        }
    }
    if (!bexist)
    {
        m_tasks[key].push_back(task);
        /* register to the callback center */
        CallBackCenter::m_register(key, m_ptr, task);
    }
}
void Callable::m_remove_native(std::string key, Task task)
{
    std::list<Task>::iterator itr;
    if (m_tasks.find(key) == m_tasks.end())
        return;
    std::list<Task> &l_tk = m_tasks[key];
    for (itr = l_tk.begin(); itr != l_tk.end();)
    {
        if (itr->target<void (*)(std::map<std::string, std::string>)>() ==
            task.target<void (*)(std::map<std::string, std::string>)>())
        {
            itr = l_tk.erase(itr);
            break;
        }
        ++itr;
    }
    /* remove the empty key */
    if (m_tasks[key].empty())
        m_tasks.erase(m_tasks.find(key));
}
void Callable::m_remove(std::string key, Task task)
{
    m_remove_native(key, task);
    CallBackCenter::m_remove(key, m_ptr, task);
}