/*
 * ThreadPool.cpp
 *
 *  Created on: 2020/03/01 21:54:36
 *      Author: juntao, at wuhan university
 */
#include <unistd.h>
#include <pthread.h>
#include "include/Frame/Frame.h"
using namespace iono;

ThreadPool::ThreadPool(int number) : stop(false)
{
    ithread = 0;
    ntask_num = 0;
    if (number <= 0 || number > MAX_THREADS)
        throw std::exception();
    for (int i = 0; i < number; i++)
    {
        std::cout << "create" << i << " thread" << std::endl;
        work_threads.emplace_back(ThreadPool::worker, this);
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (auto &ww : work_threads)
        ww.join();
}

bool ThreadPool::append(Task_thread task)
{
    {
        std::unique_lock<std::mutex> lk(this->queue_mutex);
        tasks_queue.push(task);
    }
    {
        std::unique_lock<std::mutex> sync_lk(this->sync_mutex);
        this->ntask_num++;
    }
    condition.notify_all();
    return true;
}
void *ThreadPool::worker(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;
    pool->run();
    return pool;
}
void ThreadPool::sync()
{
    {
        std::unique_lock<std::mutex> sync_lk(this->sync_mutex);
        if (this->ntask_num != 0)
            this->sync_condition.wait(sync_lk, [this]
                                      { return this->ntask_num == 0; });
    }
}
void ThreadPool::run()
{
    int tid = ithread++;
    int num_cores = sysconf(_SC_NPROCESSORS_CONF);
    int cpu_bind = tid % num_cores;
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_bind, &mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
        perror("sched_setaffinity");
    else
        printf("binding threads %u into cpu %d, total num: %d\n", pthread_self(), cpu_bind, num_cores);
    while (!stop)
    {
        Task_thread task;
        {
            /// get a task from the queue
            std::unique_lock<std::mutex> lk(this->queue_mutex);
            this->condition.wait(lk, [this]
                                 { return !this->tasks_queue.empty() || this->stop; });
            if (this->tasks_queue.empty() && this->stop)
                return;
            task = tasks_queue.front();
            tasks_queue.pop();
        }
        task();
        /*** sync the task number **/
        {
            std::unique_lock<std::mutex> sync_lk(this->sync_mutex);
            this->ntask_num--;
        }

        sync_condition.notify_one();
    }
}