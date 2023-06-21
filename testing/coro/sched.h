#ifndef CORO_SCHED_H
#define CORO_SCHED_H
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coro/sched.h"
#include "coro/switch.h"
#include "event.h"
#include "internal.h"
#include "util/list.h"
#include "util/memcache.h"
#include "util/rbtree.h"
#include "util/system.h"

#include <stdbool.h>
/* coroutine */

typedef void (*coro_func)(void *args);

struct timer_node {
    struct rb_node node; /* inactive */
    struct rb_root root; /* coroutines with the same timeout */
    long long timeout;   /* unit: milliseconds */
};

struct coroutine {
    struct list_head list; /* free_pool or active linked list */
    struct rb_node node;
    int coro_id;
    struct context ctx;
    struct coro_stack stack;
    coro_func func;
    void *args; /* associated with coroutine function */

    long long timeout; /* track the timeout of events */
    int active_by_timeout;
};

/* sched policy, if timeout == -1, sched policy can wait forever */
typedef void (*sched_policy_t)(int timeout /* in milliseconds */);

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

// extern struct coro_schedule sched;




// spinlock_t lock;
/* add other function as extern */
struct coroutine *find_in_timer(struct timer_node *tm_node, int coro_id);
void remove_from_timer_node(struct timer_node *tm_node, struct coroutine *coro);
struct timer_node *find_timer_node(long long timeout);
void remove_from_inactive_tree(struct coroutine *coro);
void add_to_timer_node(struct timer_node *tm_node, struct coroutine *coro);
void move_to_inactive_tree(struct coroutine *coro);
void move_to_idle_list_direct(struct coroutine *coro);
void move_to_active_list_tail_direct(struct coroutine *coro);
void move_to_active_list_tail(struct coroutine *coro);
void move_to_active_list_head(struct coroutine *coro);
struct coroutine *create_coroutine();
struct coroutine *get_coroutine();
void coroutine_switch(struct coroutine *curr, struct coroutine *next);
struct coroutine *get_active_coroutine();
void run_active_coroutine();
struct timer_node *get_recent_timer_node();
void timeout_coroutine_handler(struct timer_node *node);
void check_timeout_coroutine();
int get_recent_timespan();
void  __attribute__((__regparm__(1))) coro_routine_proxy(void *args);



void schedule_cycle();
int dispatch_coro(coro_func func, void *args);
void schedule_timeout(int milliseconds);
bool is_wakeup_by_timeout();

void wakeup_coro(void *args);
void wakeup_coro_priority(void *args);
void *current_coro();

void schedule_init(size_t stack_kbytes, size_t max_coro_size);



/* for testing only */

#endif
