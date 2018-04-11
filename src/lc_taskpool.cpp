#include "lc_taskpool.h"
#include "lc_threadpool.h"
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <math.h>

void* manage_task_pool_thread(void *arg);
pthread_mutex_t TaskPool::m_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  TaskPool::m_cond = PTHREAD_COND_INITIALIZER;

TaskPool* TaskPool::m_instance = NULL;

void TaskPool::destroy_pool()
{
    Task* t = NULL;
    list<Task*>::iterator it , it1;
    it = m_tlist.begin();
    for (; it != m_tlist.end();)
    {
        it1 = it++;
        t = *it1;
	del_task_by_id(t->m_id);
    }
    m_instance = NULL; //not thread-safe
}

void TaskPool::show_task_list()
{
    const char *type_str[] = {"time  ", "normal"};
    Task* t = NULL;
    list<Task*>::iterator it = m_tlist.begin();
    for (; it != m_tlist.end(); it++)
    {
        t = *it;
        printf("task %d type %s start time %ld\n", t->m_id, type_str[t->get_type()], t->get_start_time());
    }
    printf("\n\n");
}

TaskPool* TaskPool::get_instance()
{
    pthread_mutex_lock(&TaskPool::m_mtx);
    if (m_instance == NULL)
        m_instance = new TaskPool();
    pthread_mutex_unlock(&TaskPool::m_mtx);
    return m_instance;
};

int TaskPool::init()
{
    m_tnum = m_num = m_total = 0;
    m_start_failed = 0;
    m_task_over_count = 0;
    m_cur_task = NULL;
    pthread_create(&m_tid, NULL, manage_task_pool_thread, NULL);
    pthread_detach(m_tid);

    return 0;
};

/*
    add task to list, and sort by start time
*/
int TaskPool::add_task_inter(Task *t)
{
    if (!t) return -1;
    if (m_tlist.empty())
    {
        m_tlist.push_back(t);
        return 0;
    }

    int64_t start_time = t->get_start_time();
    int64_t next_time = start_time;
    list<Task*>::iterator it = m_tlist.begin(); 
    int     count = 0;

    for (;it != m_tlist.end(); ++it)
    {
        if (!(*it)) 
        {
            //abort();
            return -2;
        }
        next_time = (*it)->get_start_time();
        if (next_time > start_time)
        {
            m_tlist.insert(it, t);
            return 0;;
        }
    }

    m_tlist.push_back(t);

    return 0;
};

int TaskPool::add_time_task(Event *e, int64_t start_ms, int64_t interval)
{
    Task* task = new Task(e, TASK_TYPE_TIME_TASK, start_ms, interval);
    pthread_mutex_lock(&TaskPool::m_mtx);
    int ret =  add_task_inter(task);
    if (ret == 0) update_cur_task(true);
    pthread_mutex_unlock(&TaskPool::m_mtx);
    if (ret == 0) 
    {
        m_total++;
        m_tnum++;
    }
    printf( "ADD TIME TASK %s! %s\n", (ret == 0) ? "  ok" : "fail", task->toString().c_str());
        
    return ret ? ret : task->m_id;
};

int TaskPool::add_normal_task(Event *e, int64_t start_ms)
{
    Task* task = new Task(e, TASK_TYPE_NORMAL, start_ms);
    pthread_mutex_lock(&TaskPool::m_mtx);
    int ret =  add_task_inter(task);
    if (ret == 0) update_cur_task(true);
    pthread_mutex_unlock(&TaskPool::m_mtx);
    if (ret == 0) 
    {
        m_total++;
        m_num++;
    }
    printf( "ADD NORMAL TASK %s! %s\n", (ret == 0) ? "  ok": "fail", task->toString().c_str());

    return ret ? ret : task->m_id;
};

/* 
    Simple Principle:
    1.IDLE: delete directly
    2.READY, IDLE, FIN: (handle by task type)
        cycle task : set status to DEL, manage thread will recycling resource
        normal task：work thread deal.
    3.DEL: just ignore.
*/
int TaskPool::delete_task_operate(Task *t, list<Task*>::iterator& it)
{
    int ret = 0;
    bool is_time_task = true;

    task_state s = t->set_state_for_del(is_time_task);
    switch (s)
    {
        case TASK_IDLE:
            m_tlist.erase(it);
            delete t;
            break;

        case TASK_READY:
        case TASK_RUN:
            break;

        case TASK_FIN:
            if (is_time_task)
            {
                t->modify_start_time(timetask::get_cur_start_time());
                m_tlist.erase(it);
                add_task_inter(t);
            }
            break;
        case TASK_DEL:
            ret = -3;
            break;
        default:
            printf("error! unkown task state!\n");
            //abort();
            ret = -3;
            break;
    }
    if (ret == 0) 
    {
        update_cur_task(true);
        m_total--;
        (is_time_task) ? m_tnum-- : m_num--;
    }

    return ret;
};

int TaskPool::del_task_by_id_inter(int id)
{
    int ret = -1;
    Task *t = NULL;
    list<Task*>::iterator it = m_tlist.begin();

    for (;it != m_tlist.end(); ++it)
    {
        t = *it;
        if (t == NULL)
        {
            //abort();
            ret = -2;
            break;
        }
        if (t->m_id == id) 
        {
            ret = 0;
            break;
        }
    }

    if (ret == 0)
    {
        ret = delete_task_operate(t, it);
    }

    return ret;
}

int TaskPool::del_task_by_id(int id)
{
    int ret = 0;
    const char *err_string[3] = {
        "task not found by id!",
        "found NULL pointer in m_tlist",
        "set task state failed!"
    };

    pthread_mutex_lock(&TaskPool::m_mtx); 
    ret = del_task_by_id_inter(id);
    pthread_mutex_unlock(&TaskPool::m_mtx); 

    if (ret != 0)
        printf( "Delete Fail. [task %04d] [%s]\n", id, err_string[-ret - 1]);

    return 0;
}

int TaskPool::del_task_by_event_inter(Event *e, int& id)
{
    int ret = -1;
    Task *t = NULL;
    list<Task*>::iterator it = m_tlist.begin();

    for (;it != m_tlist.end(); ++it)
    {
        t = *it;
        if (t == NULL)
        {
            ret = -2;
            break;
        }
        if (t->get_event() == e)
        {
            ret = 0;
            break;
        }
    }

    if (ret == 0)
    {
        id = t->m_id;
        ret = delete_task_operate(t, it);
    }

    return 0;
}

int TaskPool::del_task_by_event(Event *e)
{
    int ret = 0;
    const char *err_string[3] = {
        "task not found by event!",
        "found NULL pointer in m_tlist",
        "set task state failed!",
    };

    int id = -1;
    pthread_mutex_lock(&TaskPool::m_mtx); 
    ret = del_task_by_event_inter(e, id);
    pthread_mutex_unlock(&TaskPool::m_mtx); 

    if (ret != 0)
        printf( "DELETE fail, [task %04d] [%s]", id, err_string[-ret - 1]);

    return 0;
}

string TaskPool::get_task_pool_info()
{
    char buf[1024] = {0};
    sprintf(buf, "total task num:%d\ncycle task num:%d\nnormal task num:%d\n"
                "overtime task count:%d\n start failed count:%ld\n"
                ,m_total, m_tnum, m_num,
                m_task_over_count, m_start_failed);
                
    return buf;
}

/* update head task */
void TaskPool::update_cur_task(bool isresume)
{
    Task *old_task = m_cur_task;
    if (m_tlist.empty()) m_cur_task = NULL;
    m_cur_task = m_tlist.front();
    //show_task_list();
    if (old_task != m_cur_task)
        if (isresume) pthread_cond_signal(&TaskPool::m_cond);
}

Task* TaskPool::get_cur_task()
{
    return m_cur_task;
}

int64_t TaskPool::get_cur_time_us()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return (time.tv_sec * 1000000 + time.tv_usec);
}

void TaskPool::pop_front()
{
    m_tlist.pop_front();
    update_cur_task(false);
}

void get_wait_time(struct timespec &time, int64_t start_us)
{
    start_us = start_us;
    time.tv_sec = (start_us/1000000);
    time.tv_nsec = (start_us%1000000)*1000;
}


/*  The task execution time in the queue is ranked from small to large. 
    then：
    1.if get head task failed then suspend, wait user add task.
    2.if get head task success, supend before time to start.
    3.if head task changed, thread will be resumed. then jump to step 1st.
    4.pop head and check task status, and take diffrent operate for diffrent status.
        IDLE: run in an idle thread. if task type is cycle-task, then update next start time, push to task list again.
        READY,WAIT,FIN: it means cycle-task overtime, print information then update next start time, push to task list again.
        DEL:   pop head and delete it 
    5.jump to step 1st
*/
void* manage_task_pool_thread(void *arg)
{
    TaskPool *taskpool = TaskPool::get_instance();
    ThreadPool *threadpool = ThreadPool::get_instance();
    Task *task = NULL;

    int64_t last_time, cur_time, sleep_time = 20000;
    struct timespec wait_time;

    while (1)
    {
        pthread_mutex_lock(&TaskPool::m_mtx);
        task = taskpool->get_cur_task();
        if (task == NULL)
        {
            /* task list is empty, wait user add task */
            pthread_cond_wait(&TaskPool::m_cond, &TaskPool::m_mtx);
            pthread_mutex_unlock(&TaskPool::m_mtx);
            continue;
        }

        last_time = cur_time;
        cur_time = taskpool->get_cur_time_us();
        int64_t    start_us = task->get_start_time();

        /* get head success, wait to be resume */
        get_wait_time(wait_time, start_us);
        int ret = pthread_cond_timedwait(&TaskPool::m_cond, &TaskPool::m_mtx, &wait_time);
        if (ret == 0)
        {
            //head task has changed
            pthread_mutex_unlock(&TaskPool::m_mtx);
            continue;
        }
        if (ret != ETIMEDOUT)
        {
            printf("error! pthread_cond_timedwait retrun %d\n", ret);
            //abort();
        }
        
        /* update counter */
        if (!task->is_time_task())
        {
            taskpool->m_num--;
            taskpool->m_total--;
        }

        /* different process in different status */
        task_state s = task->get_task_state();
        switch (s)
        {
            case (TASK_DEL):
                taskpool->pop_front();
                delete task;
                break;
                
            case (TASK_IDLE):
                task->set_task_state(TASK_READY);
                if (threadpool->start_task(task))
                {
                    //abort();
                    printf( "task[%s]! start failed!", task->toString().c_str());
                    taskpool->m_start_failed++; 
                    task->set_task_state(TASK_IDLE);
                }
                taskpool->pop_front();
                if (task->is_time_task())
                {
                    task->update_start_time();
                    taskpool->add_task_inter(task);
                    taskpool->update_cur_task(false);
                }
                break;

            case (TASK_READY):
            case (TASK_RUN):
            case (TASK_FIN):
                if (task->is_time_task()) 
                {
                    // cycle-task overtime, it might need a long period.
                    //abort();
                    printf("task[%s]! cost more than %ldus\n", task->toString().c_str(), task->get_interval()); 
                    task->update_start_time();
                    taskpool->m_task_over_count++;
                    taskpool->pop_front();
                    taskpool->add_task_inter(task);
                    taskpool->update_cur_task(false);
                }
                else
                {
                    //abort();
                }
                break;
        }

        pthread_mutex_unlock(&TaskPool::m_mtx);
    }


    return (void *)0;
}

