#include "lc_thread.h"
#include <sys/time.h>

pthread_mutex_t Task::m_mtx = PTHREAD_MUTEX_INITIALIZER;
int             Task::ms_id = 0;

pthread_mutex_t Thread::m_mtx = PTHREAD_MUTEX_INITIALIZER;
int             Thread::ms_id = 0;

void* thread_func(void *arg) {
    Thread *thread = (Thread *)arg;
    struct  timeval start, end;

    while (1) {
        if (thread->get_thread_state() == THREAD_STATE_DESTROY) {
            thread->destroy();
            break;
        }

        Task   *task = thread->get_task();
        if (task == NULL) {
            thread->suspend();
            continue;
        }
        thread->set_thread_state(THREAD_STATE_RUNNING);

        //delete task
        task_state s = task->get_task_state();
        if (s == TASK_DEL) {
            if (!task->is_time_task()) delete  task;
            thread->clear_task();
            continue;
        }

        //check task status
        if (s == TASK_RUN || s == TASK_FIN || s == TASK_IDLE)
        {
            printf("error! error task state!\n");
            //abort();
        }

        //start run
        task->set_task_state(TASK_RUN);
        task->run();
        task->set_task_state(TASK_FIN);

        
        //after run 
        if (task->is_time_task()) {
            task->set_task_state(TASK_IDLE);
            thread->clear_task();
        } else {
            delete  task;
            thread->clear_task();
        }
    }

    printf("thread %d exit!\n", thread->get_id());

    return (void *)0;
};

void Thread::clear_task()
{
    m_task = NULL;
};

int Thread::get_id()
{
    return m_id;
};

Task*& Thread::get_task()
{
    return m_task;
};

string Thread::thread_info()
{
    char buf[1024] = {0};

    const char* str_state[] = {
        "THREAD_STATE_SUSPEND",
        "THREAD_STATE_RUNNING",
        "THREAD_STATE_DESTROY",
    };

    sprintf(buf,  "thread %d  in %s state!\n", m_id, str_state[m_state]);
    return buf;
};

thread_state Thread::get_thread_state() {
    return m_state;
};

thread_state Thread::set_thread_state(thread_state state) {
    thread_state old_state;
    pthread_mutex_lock(&m_state_mtx);
    old_state = m_state;
    m_state = state;
    pthread_mutex_unlock(&m_state_mtx);
    return old_state;
};

int Thread::set_task(Task* task) {
    m_task = task;
    return 0;
};

void Thread::set_task_event(Event *e) {
    m_task->set_event(e);
};

int Thread::suspend() {
    int ret = 0;
    pthread_mutex_lock(&m_state_mtx);
    m_state = THREAD_STATE_SUSPEND;
    ret = pthread_cond_wait(&m_cond, &m_state_mtx);
    pthread_mutex_unlock(&m_state_mtx);

    return ret;
};

int Thread::resume() {
    pthread_mutex_lock(&m_state_mtx);
    m_state = THREAD_STATE_RUNNING;
    pthread_cond_signal(&m_cond);
    pthread_mutex_unlock(&m_state_mtx);
    return 0;
};

void Thread::destroy() {
    pthread_mutex_destroy(&m_state_mtx);
    pthread_cond_destroy(&m_cond);
    if (m_task) delete m_task;
}
