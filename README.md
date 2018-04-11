# timetask
This is an implementation of the C++ based timing task on the linux platform.
Minimum precision guaranteed to 10ms. You can add multiple tasks that start in same time.
It provides a simple test code that you can run it to learn how to use.

# make and build
mkdir build && cd build && cmake .. && make 

you can find bin/test has generate, just run it. 
It will first add 10 regular time tasks, each task will only log one information. These tasks will be completed soon. Then he will add 10 repetitive tasks with interval 1s and let them execute for 30s. Then he will delete all these repetitive tasks. Task execution log, NormalEvent.log and CycleEvent.log.
