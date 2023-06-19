#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "process.h"
// #include "process.c"
#include "event.h"
#include "util/rbtree.h"
#include "util/memcache.h"
#include "util/rbtree.h"
#include "util/list.h"
#include "util/internal.h"
#include "util/str.h"
#include "env.h"
#include "coro/sched.h"
#include "coro/switch.h"
 

void NULL_func(void *ARGS __UNUSED)
{
    return;
}
int main()
{
    // schedule_init(1, 1024);
    // event_loop_init(1024);
    // dispatch_coro(NULL_func, NULL);

    // schedule_cycle();
    master_process_cycle();
    worker_process_cycle();
    return 0;
}