#include "lc_threadpool.h"
#include "lc_event.h"

pthread_mutex_t ThreadPool::pool_mutex = PTHREAD_MUTEX_INITIALIZER;
ThreadPool* ThreadPool::m_instance = NULL;

ThreadPool* ThreadPool::get_instance() {
    pthread_mutex_lock(&ThreadPool::pool_mutex);
    if (m_instance == NULL) {
        m_instance = new ThreadPool();
        if (m_instance) m_instance->init();
    }
    pthread_mutex_unlock(&ThreadPool::pool_mutex);
    return m_instance;
};

int ThreadPool::destroy_pool() {
    int ret = 0;
    pthread_mutex_lock(&ThreadPool::pool_mutex);
    ret = destroy();
    if (m_instance) {
        delete m_instance;
        m_instance = NULL;
    }
    pthread_mutex_unlock(&ThreadPool::pool_mutex);

    return ret;
};

int ThreadPool::init() {
    int i = 0;
    pthread_t   tid;
    Thread  *thread = NULL;

    m_thread_num = timetask::tnum;
    m_thread_list.clear();

    pthread_attr_t attr;
    if(pthread_attr_init(&attr) != 0)
    {
        printf("pthread_attr_init failed!");
        return -1;
    }
 
    if(pthread_attr_setstacksize(&attr, 2*1024*1024) != 0)
    {
        printf("pthread_attr_setstacksize failed!");
        return -1;
    }

    for (; i < m_thread_num; i++) {
        thread = new Thread();
        m_thread_list.push_back(thread);
        pthread_create(&tid, NULL, thread_func, thread);
        pthread_detach(tid);
    }

    return 0;
};

Thread* ThreadPool::get_idle_thread() {
    list<Thread*>::iterator it = m_thread_list.begin();
    Thread* thread = NULL;
    for (; it != m_thread_list.end(); it++) {
        thread = *it;
        pthread_mutex_lock(&thread->m_state_mtx);
        if ((*it)->get_thread_state() == THREAD_STATE_SUSPEND)
        {
            pthread_mutex_unlock(&thread->m_state_mtx);
            return thread;
        }
        pthread_mutex_unlock(&thread->m_state_mtx);
    }

    return NULL;
};

int ThreadPool::start_task(Task *task)
{
    Thread* thread = get_idle_thread();
    if (thread == NULL) {
        //printf("get_idle_thread failed for task %d!\n", task->m_id);
        //abort();
        return -1;
    }
    thread->set_task(task);
    thread->resume();

    return 0;
}

//获取线程池的信息
string ThreadPool::get_pool_info() {
    char    log[10240] = {0};
    string  info = "";

    pthread_mutex_lock(&pool_mutex);
    m_idle_num = m_busy_num = 0;

    for (list<Thread*>::iterator it = m_thread_list.begin(); it != m_thread_list.end(); it++) {
        thread_state state = (*it)->get_thread_state();
        Task *task = (*it)->get_task();
        if (state == THREAD_STATE_SUSPEND)
            m_idle_num++;
        else
            m_busy_num++;
        //info += (*it)->thread_info();
    }
    sprintf(log, "idle %d, busy %d, total %d\n", m_idle_num, m_busy_num, m_thread_num);
    pthread_mutex_unlock(&pool_mutex);

    info += log;

    return info;
};

//私有函数
int ThreadPool::thread_destroy(Thread* thread) {
    if (thread) {
        thread->set_thread_state(THREAD_STATE_DESTROY);
        return 0;
    } 

    return -1;
};

//销毁线程,释放资源。私有函数
int ThreadPool::destroy() {
    //把所有工作的线程全部置为删除状态
    Thread* thread = NULL;
    string  info = "";
    m_idle_num = m_busy_num = 0;

    for (list<Thread*>::iterator it = m_thread_list.begin(); it != m_thread_list.end(); it++) {
        thread_destroy(thread);
    }

    //删除线程队列的数据   
    m_thread_list.clear(); 

    //重置任务号和线程编号
    Thread::ms_id = 0;
    Task::ms_id = 0;

    return 0;
}
