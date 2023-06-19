#include <stddef.h>
#include <assert.h>
#include <stdio.h>

#include "util/str.h"
#include "util/rbtree.h"
#include "util/memcache.h"
#include "util/list.h"
#include "util/rbtree.h"

#include "coro/sched.h"
#include "coro/switch.h"
struct timer_node {
    struct rb_node node; /* inactive */
    struct rb_root root; /* coroutines with the same timeout */
    long long timeout;   /* unit: milliseconds */
};

struct coroutine {
    struct list_head list; /* free_pool or active linked list */
    int coro_id;
    struct context ctx;
    struct coro_stack stack;
    void *args; /* associated with coroutine function */
    long long timeout; /* track the timeout of events */
    int active_by_timeout;
};

struct coro_schedule {
    size_t max_coro_size;
    size_t curr_coro_size;

    int next_coro_id;

    size_t stack_bytes;
    struct coroutine main_coro;
    struct coroutine *current;

    struct list_head idle, active;
    struct rb_root inactive; /* waiting coroutines */
    struct memcache *cache;  /* caching timer node */
};

static struct coro_schedule sched;
/* @stack_bytes should be aligned to PAGE_SIZE */
void schedule_init(size_t stack_kbytes, size_t max_coro_size)
{
    assert(max_coro_size >= 2);

    sched.max_coro_size = max_coro_size;
    sched.curr_coro_size = 0;
    sched.next_coro_id = 0;

    sched.stack_bytes = stack_kbytes * 1024;
    sched.current = NULL;

    INIT_LIST_HEAD(&sched.idle);
    INIT_LIST_HEAD(&sched.active);


    sched.inactive = RB_ROOT;
    
    sched.cache = memcache_create(sizeof(struct timer_node), max_coro_size / 2);

    if (!sched.cache) {
        printf("Failed to create cache for schedule timer node\n");
        exit(0);
    }
}
int main()
{
    schedule_init(1024 >> 10, 1);
    return 0;
}