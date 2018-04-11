#ifndef __LC_EVENT__
#define __LC_EVENT__

#include <stdlib.h>
#include <sys/time.h>
#include <string>

#define TV2USEC(T)  ((T.tv_sec*1000000 + T.tv_usec))
#define TV2MSEC(T)  ((TV2USEC(T)/1000))
#define TIME_COST(start, end)   (TV2USEC(end) - TV2USEC(start))

#define MS          1000
#define S           1000000

class Event {
    public:
    virtual int run() = 0;
    virtual ~Event() {};
    virtual std::string toString() { return "Event";};
};

namespace timetask
{
        extern int     tnum;
        int64_t get_cur_start_time();
};
#endif
