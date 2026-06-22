//
// Created by juntao, at wuhan university   on 2020/10/17.
//

#ifndef BAMBOO_CALLBACKCENTER_H
#define BAMBOO_CALLBACKCENTER_H
#include <functional>
#include <string>
#include <list>
#include <map>
#include <mutex>
#include <cstdarg>
namespace iono
{
    class Callable;
    using Task = std::function<void(std::map<std::string, std::string>)>;
    using Task_unit = std::map<void *, std::list<Task>>;

    using Task_new = std::function<void()>;
    using Task_unit_new = std::map<void *, std::list<Task_new>>;

    class CallBackCenter
    {
    public:
        static void m_remove(std::string, void *, Task); /* remove the pointed task */
        static void m_remove(Callable *);
        static void m_register(std::string key, void *, Task);
        static void m_evoke(std::string, std::map<std::string, std::string>);

    private:
        static std::map<std::string, Task_unit> m_task;

        /* function template here
        template<typename callable, typename... arguments>
        bool wrapped_function(callable&& fun, arguments&&... args) {
            std::function<typename std::result_of<callable(arguments...)>::type()> task(std::bind(std::forward<callable>(fun), std::forward<arguments>(args)...));
        }
        */

    public:
        static void m_remove_inner(std::string, void *, Task_new);
        template <typename t_fun, typename... t_arguments>
        static void m_remov_new(std::string key, void *ptr, t_fun &&fun, t_arguments &&...args)
        {
            std::function<typename std::result_of<t_fun(t_arguments...)>::type()> task(std::bind(std::forward<t_fun>(fun), std::forward<t_arguments>(args)...));
            m_remove_inner(key, ptr, task);
        }
    };
}
#endif // BAMBOO_CALLBACKCENTER_H