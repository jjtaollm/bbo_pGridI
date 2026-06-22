//
// Created by juntao, at wuhan university   on 2020/10/22.
//

#ifndef BAMBOO_KVCONTAINER_H
#define BAMBOO_KVCONTAINER_H
#include <map>
#include <string>
namespace iono
{
    class KvContainer
    {
    public:
        static inline std::string getv(std::string key)
        {
            if (kvs.find(key) == kvs.end())
                return "";
            return kvs[key];
        }
        static inline void setv(std::string key, std::string v)
        {
            kvs[key] = v;
        }

    protected:
        static std::map<std::string, std::string> kvs;
    };
} // namespace bamboo
#endif // BAMBOO_KVCONTAINER_H
