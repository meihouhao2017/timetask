#ifndef __LC_TASK_POOL_H__
#define __LC_TASK_POOL_H__
#include <iostream>
#include <list>
#include <pthread.h>
#include <unistd.h>
#include "lc_event.h"
#include "lc_task.h"

class TaskPool
{
    private:
        list<Task*>         m_tlist;        
        Task*               m_cur_task;     
        pthread_t           m_tid;
        static TaskPool*    m_instance;

        int init();
        int delete_task_operate(Task *t, list<Task*>::iterator& it);

    public:
        int                 m_tnum;       //cycle task number
        int                 m_num;        //normal task number
        int                 m_total;      //total task number
        int                 m_task_over_count;   //overtime count
        int64_t             m_start_failed;      //start failed count
        static pthread_mutex_t      m_mtx;
        static pthread_cond_t       m_cond;     

    public:
        TaskPool()
        {
            init();
        }

        void show_task_list();
        int add_task_inter(Task *t); 
        int del_task_by_id_inter(int id);
        int del_task_by_event_inter(Event *e, int& id);
        int64_t get_cur_time_us();
        string get_task_pool_info();
        Task* get_cur_task();
        void update_cur_task(bool isresume);
        void pop_front();
        void get_wait_time(struct timespec &time, int64_t start_us);

        //user api
        static TaskPool* get_instance();
	    void destroy_pool();
        int add_time_task(Event *e, int64_t start_ms, int64_t interval=20);
        int add_normal_task(Event *e, int64_t start_ms);
        int del_task_by_id(int id);
        int del_task_by_event(Event *e);

};

#endif
