#include "optimized_thread.h"

OptimizedThreadPool::OptimizedThreadPool() :
	m_thread_count(std::thread::hardware_concurrency() != 0 ? std::thread::hardware_concurrency() : 4),
	m_thread_queues(m_thread_count)
{
}

void OptimizedThreadPool::start() 
{
	for (int i = 0; i < m_thread_count; i++) {
		m_threads.emplace_back(&OptimizedThreadPool::threadFunc, this, i);
	}
}

void OptimizedThreadPool::threadFunc(int qindex) 
{
    while (true) {

        TaskWithPromise task_to_do;
        {
            bool res;
            int i = 0;
            for (; i < m_thread_count; i++)
            {
                if (res = m_thread_queues[(qindex + i) % m_thread_count].fast_pop(task_to_do))
                {
                    break;
                }
            }
            if (!res)
            {
                m_thread_queues[qindex].pop(task_to_do);
            }
            else if (!task_to_do.task)
            {
                m_thread_queues[(qindex + i) % m_thread_count].push(task_to_do);
            }
            if (!task_to_do.task)
            {
                return;
            }
            task_to_do.task;
            task_to_do.prom.set_value();
        }
    }
}

void OptimizedThreadPool::stop()
{
    for (int i = 0; i < m_thread_count; i++)
    {
        TaskWithPromise empty_task;
        m_thread_queues[i].push(empty_task);
    }
    for (auto& t : m_threads)
    {
        t.join();
    }
}

res_type OptimizedThreadPool::push_task(FuncType f, It begin, It end)
{
    std::lock_guard<std::mutex> l(m_locker);
    int queue_to_push = m_qindex++ % m_thread_count;
    TaskWithPromise twp;
    twp.task = [begin, end, f]
    {
        f(begin, end);
    };
    res_type res = twp.prom.get_future();
    m_thread_queues[queue_to_push].push(twp);
    return res;
}

RequestHandler::RequestHandler()
{
    m_tpool.start();
}


RequestHandler::~RequestHandler()
{
    m_tpool.stop();
}

res_type RequestHandler::push_request(FuncType f, It begin, It end)
{
    return m_tpool.push_task(f, begin, end);
}