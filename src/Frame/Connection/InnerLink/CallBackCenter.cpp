//
// Created by juntao, at wuhan university   on 2020/10/17.
//
#include "include/Frame/Frame.h"
#include <iostream>
using namespace iono;
std::map<std::string, Task_unit> CallBackCenter::m_task;
void CallBackCenter::m_evoke(std::string key, std::map<std::string, std::string> args)
{
    if (m_task.find(key) == m_task.end())
        return;
    for (const auto &kvv : m_task[key])
    {
        for (const auto &tk : kvv.second)
            tk(args); /* evoke this function */
    }
}
void CallBackCenter::m_register(std::string key, void *ptr, Task task)
{
    bool bexist = false;
    if (m_task.find(key) == m_task.end())
    {
        m_task[key] = Task_unit();
    }
    if (m_task[key].find(ptr) == m_task[key].end())
    {
        m_task[key][ptr] = std::list<Task>{task};
        return;
    }
    for (auto &tt : m_task[key][ptr])
    {
        if (tt.target<void (*)(std::map<std::string, std::string>)>() == task.target<void (*)(std::map<std::string, std::string>)>())
        {
            bexist = true;
            break;
        }
    }
    if (!bexist)
        m_task[key][ptr].push_back(task); /* push it into the structure */
}
void CallBackCenter::m_remove(std::string key, void *ptr, Task task)
{
    std::list<Task>::iterator itr;
    if (m_task.find(key) == m_task.end())
        return;
    if (m_task[key].find(ptr) == m_task[key].end())
        return;
    std::list<Task> &l_tk = m_task[key][ptr];
    for (itr = l_tk.begin(); itr != l_tk.end();)
    {
        if (itr->target<void (*)(std::map<std::string, std::string>)>() == task.target<void (*)(std::map<std::string, std::string>)>())
        {
            itr = l_tk.erase(itr);
            break;
        }
        ++itr;
    }
    /* remove the empty key */
    if (m_task[key][ptr].empty())
        m_task[key].erase(m_task[key].find(ptr));
    if (m_task[key].empty())
        m_task.erase(m_task.find(key));
}
void CallBackCenter::m_remove(Callable *call)
{
    for (const auto &kv : call->m_tasks)
    {
        for (const auto &tk : kv.second)
        {
            m_remove(kv.first, call->m_ptr, tk);
        }
    }
}