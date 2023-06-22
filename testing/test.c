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
struct coro_schedule sched;

void NULL_func(void *ARGS __UNUSED)
{
    for(int i = 0; i< 100; i++)
        continue;

    return;
}
int main()
{
    schedule_init(10, 1024);
    event_loop_init(1024);
    int tmp = dispatch_coro(NULL_func, NULL);
    printf("dispatch Done errno: %d\n", tmp);
    struct coroutine *coro = create_coroutine();
    // move_to_active_list_head(coro);
    get_coroutine();

    run_active_coroutine();
    dispatch_coro(NULL_func, NULL);
    check_timeout_coroutine();
    // start treating tree
    // coroutine_switch(sched.current, &sched.main_coro);
    // schedule_cycle();
    // schedule_free_handler();

    // remove_from_inactive_tree(coro);

    return 0;
}