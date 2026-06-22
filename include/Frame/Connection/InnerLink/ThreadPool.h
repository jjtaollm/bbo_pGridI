#ifndef _THREADPOOL_H
#define _THREADPOOL_H
#include <vector>
#include <queue>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <condition_variable>
#include <memory> //unique_ptr
#include <functional>
namespace iono
{
	const int MAX_THREADS = 1000;

	typedef std::function<void(void)> Task_thread;

	class ThreadPool
	{
	public:
		ThreadPool(int number = 1);
		~ThreadPool();
		bool append(Task_thread task);
		void sync();
		int get_nthread() { return work_threads.size(); }

	private:
		static void *worker(void *arg);
		void run();

	private:
		std::vector<std::thread> work_threads;
		std::queue<Task_thread> tasks_queue;

		std::mutex queue_mutex;
		std::condition_variable condition;
		bool stop;

		std::mutex sync_mutex;
		std::condition_variable sync_condition;
		int ntask_num, ithread;
	};
}
#endif
