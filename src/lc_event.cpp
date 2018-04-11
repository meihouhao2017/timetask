#include "lc_event.h"

int timetask::tnum = 8;
int64_t timetask::get_cur_start_time()
{   
    struct timeval tv; 
    gettimeofday(&tv, NULL);
    int64_t us = ((int64_t)tv.tv_sec * 1000000 + tv.tv_usec);
    return us; 
};  

