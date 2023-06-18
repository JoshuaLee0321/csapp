#include "src/coro/sched.h"
#include "src/coro/switch.h"

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

    sched_policy_t policy;
};
static struct coro_schedule sched;

static struct coroutine *create_coroutine()
{
    if (unlikely(sched.curr_coro_size == sched.max_coro_size))
        return NULL;

    struct coroutine *coro = malloc(sizeof(struct coroutine));
    if (unlikely(!coro))
        return NULL;

    if (coro_stack_alloc(&coro->stack, sched.stack_bytes)) {
        free(coro);
        return NULL;
    }

    coro->coro_id = ++sched.next_coro_id;
    sched.curr_coro_size++;

    return coro;
}
static struct coroutine *get_coroutine()
{
    struct coroutine *coro;

    if (!list_empty(&sched.idle)) {
        coro = list_first_entry(&sched.idle, struct coroutine, list);
        list_del(&coro->list);
        coroutine_init(coro);

        return coro;
    }

    coro = create_coroutine();
    if (coro)
        coroutine_init(coro);

    return coro;
}
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
    sched.policy = event_cycle;
    if (!sched.cache) {
        printf("Failed to create cache for schedule timer node\n");
        exit(0);
    }
}
int main()
{
    schedule_init();
    coroutine_init();
    return 0;
}