#include <stdio.h>
#include <string.h>
#include "lc_taskpool.h"
#include "lc_threadpool.h"
#include "lc_event.h"

string now_time()
{
    struct timeval tt;
    gettimeofday(&tt, NULL);
    struct tm ttm;
    localtime_r(&tt.tv_sec, &ttm);
    char buf[64] = {0};
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ttm);
    sprintf(buf + strlen(buf), ":%03dms", (tt.tv_usec/1000));
    return buf;
}

class NormalEvent : public Event
{
    public:
        int run()
        {
            fprintf(fp, "%s NormalEvent %d is running!\n", now_time().c_str(), id);
            return 0;
        };

        NormalEvent(FILE* f, int i)
        {
            fp = f;
            id = i;
        };
    private:
        int     id;
        FILE*   fp;
};

class CycleEvent : public Event
{
    public:
        int run()
        {
            fprintf(fp, "%s CycleEvent %d is running!\n", now_time().c_str(), id);
            return 0;
        };

        CycleEvent(FILE* f, int i)
        {
            fp = f;
            id = i;
        };

    private:
        int     id;
        FILE*   fp;
};


void test_show_info(ThreadPool* threadp, TaskPool* taskp)
{
    string taskinfo = taskp->get_task_pool_info();
    string threadinfo = threadp->get_pool_info();
    printf("thread_info:\n%s\n", threadinfo.c_str());
    printf("task_info:\n%s\n", taskinfo.c_str());

    return ;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s thread_num\ntip: max thread num is 128 in this demo!\n", argv[0]);
        return -1;
    }

    timetask::tnum = atoi(argv[1]);
    timetask::tnum = (timetask::tnum > 128) ? 128 : timetask::tnum;

    FILE *fpn = fopen("NormalTask.log", "w+");
    FILE *fpc = fopen("CycleTask.log", "w+");

    if (!fpn || !fpc)
    {
        printf("fopen failed!\n");
        return -1;
    }

    TaskPool* taskp = TaskPool::get_instance();
    ThreadPool* threadp = ThreadPool::get_instance();

    if (!taskp || !threadp)
    {
        printf("get_instance failed!\n");
        return -1;
    }


    int i, j, k;

    printf("I will add ten normal task, each start interval is 1ms! \n");
    //sleep(1);
    //add ten normal task
    for (i = 0; i < 10; i++)
    {
        taskp->add_normal_task(new NormalEvent(fpn, i), timetask::get_cur_start_time()+i*MS);
    }
    test_show_info(threadp, taskp);

    printf("I will add ten Cycle task, each start interval is 1ms and eacho period is 1s!\n");
    //sleep(1);
    for (j = i; j < 20; j++)
    {
        //on success, add_time_task return taskid and it's equal to j
        taskp->add_time_task(new CycleEvent(fpc, j), timetask::get_cur_start_time()+j*MS, 1*S);
    }
    test_show_info(threadp, taskp);
    sleep(30);

    printf("normal task might have done, then I will delete all cycle task!\n");
    for (k = i; k < j; k++)
    {
        taskp->del_task_by_id(k);
    }
    sleep(1);
    test_show_info(threadp, taskp);
    
    fclose(fpn);
    fclose(fpc);
    taskp->destroy_pool();
    threadp->destroy_pool();

    return 0;
}
