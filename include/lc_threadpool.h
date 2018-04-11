#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include "lc_thread.h"
#include "lc_task.h"
#include <pthread.h>
#include <list>
#include <stdio.h>

class ThreadPool {
    public:
        //user api
        static ThreadPool* get_instance();
        int destroy_pool();
        int start_task(Task *task);
        string get_pool_info();

    public:
        static pthread_mutex_t pool_mutex;
        ~ThreadPool() {
            destroy();
        };


    private:

        Thread* get_idle_thread();
        int init();
        int destroy();
        int thread_destroy(Thread* thread);

    private:
        static ThreadPool      *m_instance;        
        list<Thread *>          m_thread_list;
        int                     m_thread_num;
        int                     m_busy_num;
        int                     m_idle_num;
};
#endif
