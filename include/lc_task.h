#ifndef __LC_TASK_H__
#define __LC_TASK_H__

#include <pthread.h>
#include <stdlib.h>
#include <iostream>
#include <queue>
#include <list>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "lc_event.h"

using namespace std;

#define STR(A)  #A

enum task_state {
    TASK_IDLE,
    TASK_READY,
    TASK_RUN,
    TASK_FIN,
    TASK_DEL,
};

enum task_type {
    TASK_TYPE_NORMAL,
    TASK_TYPE_TIME_TASK,
};


static string i2str(int64_t num) {
    char buf[22] = {0};
    sprintf(buf, "%lu", num);
    return buf;
};

class Task {
    public :
        Task(Event *event, task_type type, int64_t start_msec, int64_t  interval = 0, string name = "Task") {
            m_event = event;
            m_type = type;
            m_start = start_msec;
            m_id =  get_task_id(); 
            m_tid =  0;
            m_name = name + "_" + i2str(m_id);
            m_interval = interval;
            m_state = TASK_IDLE;
            pthread_mutex_init(&m_state_mtx, NULL);
        };

        int get_cur_time_us()
        {
            struct timeval time;
            gettimeofday(&time, NULL);
            return TV2USEC(time);
        };


        ~Task() {
            printf("Delete Success. task [%s]\n", toString().c_str());
            if (m_event) delete m_event;
            pthread_mutex_destroy(&m_state_mtx);
        };

        void set_event(Event *event) { m_event = event;};
        Event* get_event() { return m_event; };

        string toString() { char buf[1024] = {0}; sprintf(buf, "[task %04d] %s", m_id, m_event->toString().c_str()); return buf;};

        int run() {
            return m_result = m_event->run();
        };

        bool is_time_task() { return (m_type == TASK_TYPE_TIME_TASK); };

        task_state get_task_state() {
            task_state s = TASK_READY;
            pthread_mutex_lock(&m_state_mtx);
            s = m_state;
            pthread_mutex_unlock(&m_state_mtx);
            return s;
        };

        int64_t get_interval() { return m_interval;};
        int64_t get_start_time() { return m_start; };

        void update_start_time() { m_start = m_start + m_interval; };
        void modify_start_time(int64_t us) { m_start = us;};

        task_state set_task_state(task_state state) {
            task_state old_state = m_state;
            pthread_mutex_lock(&m_state_mtx);
            if (old_state != TASK_DEL)
                m_state = state;
            pthread_mutex_unlock(&m_state_mtx);
            return old_state;
        };

        //prevent coredump when call delete_task_operate to delete task which state in TASK_FIN
        task_state set_state_for_del(bool& ret_arg) {
            ret_arg = is_time_task();
            return set_task_state(TASK_DEL);
        };

        task_type get_type() { return m_type;};

    public:
        static int              ms_id;
        static pthread_mutex_t  m_mtx;
        int                     m_tid;
        int                     m_id;
        int                     m_result;

    private :
        int get_task_id() {
            int id = 0;

            pthread_mutex_lock(&m_mtx);
            id = ms_id++;
            pthread_mutex_unlock(&m_mtx);

            return id;
        };

    private :
        volatile task_state     m_state;
        string                  m_name;
        task_type               m_type;
        int64_t                 m_interval;
        int64_t                 m_start;
        Event                   *m_event;
        pthread_mutex_t         m_state_mtx;
};

#endif
