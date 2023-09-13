#pragma once

#include <future>
#include <iterator>
#include <queue>

using It = std::vector<int>::iterator;
typedef std::function<void()> task_type;
typedef void (*FuncType) (It begin, It end);
typedef std::future<void> res_type;

template<class T>
class BlockedQueue
{
public:
    void push(T& item)
    {
        std::lock_guard<std::mutex> l(m_locker);
        m_task_queue.push(std::move(item));
        m_event_holder.notify_one();
    }
    void pop(T& item)
    {
        std::unique_lock<std::mutex> l(m_locker);
        if (m_task_queue.empty())
        {
            m_event_holder.wait(l, [this] {return !m_task_queue.empty(); });
        }
        item = std::move(m_task_queue.front());
        m_task_queue.pop();
    }

    bool fast_pop(T& item)
    {
        std::lock_guard<std::mutex> l(m_locker);
        if (m_task_queue.empty())
        {
            return false;
        }
        item = std::move(m_task_queue.front());
        m_task_queue.pop();
        return true;
    }

private:
    std::mutex m_locker;
    std::queue<T> m_task_queue;
    std::condition_variable m_event_holder;
};

class OptimizedThreadPool
{
public:
    OptimizedThreadPool();
    void start();
    void stop();
    res_type push_task(FuncType, It begin, It end);
    void threadFunc(int qindex);

private:
    struct TaskWithPromise {
        task_type task;
        std::promise<void> prom;
    };
    std::mutex m_locker;
    int m_thread_count;
    std::vector<std::thread> m_threads;
    std::vector<BlockedQueue<TaskWithPromise>> m_thread_queues;
    unsigned m_qindex;
};

class RequestHandler {
public:
    RequestHandler();
    ~RequestHandler();
    res_type push_request(FuncType, It begin, It end);

private:
    OptimizedThreadPool m_tpool;
};