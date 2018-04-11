#ifndef __LC_THREAD_H__
#define __LC_THREAD_H__

#include <pthread.h>
#include "lc_task.h"

void *thread_func(void *);
       
enum thread_state {
    THREAD_STATE_SUSPEND,
    THREAD_STATE_RUNNING,
    THREAD_STATE_DESTROY,
    THREAD_STATE_FATAL,
};


class Thread {
    public :

        Thread(Task *task=NULL) {
            m_id = get_thread_id();
            m_task = task;
            m_state = THREAD_STATE_SUSPEND;
            pthread_mutex_init(&m_state_mtx, NULL);
            pthread_cond_init(&m_cond, NULL);
        };

        ~Thread() {
            destroy();
        };

        void clear_task();

        string thread_info();

        int suspend();

        int resume();

        thread_state get_thread_state();

        thread_state set_thread_state(thread_state state);

        int set_task(Task* task);

        Task*& get_task();

        int get_id();

        void set_task_event(Event *e);

        void destroy();

    public :
        static int              ms_id;
        static pthread_mutex_t  m_mtx; 
        pthread_mutex_t         m_state_mtx; 

    private:
        int get_thread_id() {
            int id = 0;

            pthread_mutex_lock(&m_mtx);
            id = ms_id++;
            pthread_mutex_unlock(&m_mtx);

            return id;
        };
        
    private:
        int             m_id;
        int             m_run_counts;
        Task*           m_task;
        volatile thread_state    m_state;
        pthread_cond_t  m_cond; 
};

#endif

