#define _GNU_SOURCE

#include <stdint.h>
#include <string.h>

#define SPSC_QUEUE_SIZE (1024 * 8)

/* TODO: Customize your elements here */
typedef uint32_t element_t;
static element_t SPSC_QUEUE_ELEMENT_ZERO = 0;

#define SPSC_BATCH_SIZE (SPSC_QUEUE_SIZE / 16)
#define SPSC_BATCH_INCREAMENT (SPSC_BATCH_SIZE / 2)
#define SPSC_CONGESTION_PENALTY (1000) /* spin-cycles */

enum {
    SPSC_OK = 0,
    SPSC_Q_FULL = -1,
    SPSC_Q_EMPTY = -2,
};

typedef union {
    volatile uint32_t w;
    volatile const uint32_t r;
} counter_t;

#define __ALIGN __attribute__((aligned(64)))

typedef struct spsc_queue {
    counter_t head; /* Mostly accessed by producer */
    volatile uint32_t batch_head;
    counter_t tail __ALIGN; /* Mostly accessed by consumer */
    volatile uint32_t batch_tail;
    unsigned long batch_history;

    /* For testing purpose */
    uint64_t start_c __ALIGN;
    uint64_t stop_c;

    element_t data[SPSC_QUEUE_SIZE] __ALIGN; /* accessed by prod and coms */
} __ALIGN spsc_queue_t;

/* TODO: provide architecture-specific implementation */
static inline uint64_t read_tsc()
{
    uint64_t time;
    uint32_t msw, lsw;
    __asm__ __volatile__(
        "rdtsc\n\t"
        "movl %%edx, %0\n\t"
        "movl %%eax, %1\n\t"
        : "=r"(msw), "=r"(lsw)
        :
        : "%edx", "%eax");
    time = ((uint64_t) msw << 32) | lsw;
    return time;
}

static inline void wait_ticks(uint64_t ticks)
{
    uint64_t current_time;
    uint64_t time = read_tsc();
    time += ticks;
    do {
        current_time = read_tsc();
    } while (current_time < time);
}

static void queue_init(spsc_queue_t *self)
{
    memset(self, 0, sizeof(spsc_queue_t));
    self->batch_history = SPSC_BATCH_SIZE;
}

static int dequeue(spsc_queue_t *self, element_t *val_ptr)
{
    unsigned long batch_size = self->batch_history;
    *val_ptr = SPSC_QUEUE_ELEMENT_ZERO;

    /* Try to zero in on the next batch tail */
    if (self->tail.r == self->batch_tail) {
        uint32_t tmp_tail = self->tail.r + SPSC_BATCH_SIZE;
        if (tmp_tail >= SPSC_QUEUE_SIZE) {
            tmp_tail = 0;
            if (self->batch_history < SPSC_BATCH_SIZE) {
                self->batch_history =
                    (SPSC_BATCH_SIZE <
                     (self->batch_history + SPSC_BATCH_INCREAMENT))
                        ? SPSC_BATCH_SIZE
                        : (self->batch_history + SPSC_BATCH_INCREAMENT);
            }
        }

        batch_size = self->batch_history;
        while (!(self->data[tmp_tail])) {
            wait_ticks(SPSC_CONGESTION_PENALTY);

            batch_size >>= 1;
            if (batch_size == 0)
                return SPSC_Q_EMPTY;

            tmp_tail = self->tail.r + batch_size;
            if (tmp_tail >= SPSC_QUEUE_SIZE)
                tmp_tail = 0;
        }
        self->batch_history = batch_size;

        if (tmp_tail == self->tail.r)
            tmp_tail = (tmp_tail + 1) >= SPSC_QUEUE_SIZE ? 0 : tmp_tail + 1;
        self->batch_tail = tmp_tail;
    }

    /* Actually pull out the data element. */
    *val_ptr = self->data[self->tail.r];
    self->data[self->tail.r] = SPSC_QUEUE_ELEMENT_ZERO;
    self->tail.w++;
    /* for writer to write */
    if (self->tail.r >= SPSC_QUEUE_SIZE)
        self->tail.w = 0;

    return SPSC_OK;
}

static int enqueue(spsc_queue_t *self, element_t value)
{
    /* Try to zero in on the next batch head. */
    if (self->head.r == self->batch_head) {
        uint32_t tmp_head = self->head.r + SPSC_BATCH_SIZE;
        if (tmp_head >= SPSC_QUEUE_SIZE)
            tmp_head = 0;

        if (self->data[tmp_head]) {
            /* run spin cycle penality */
            wait_ticks(SPSC_CONGESTION_PENALTY);
            return SPSC_Q_FULL;
        }
        self->batch_head = tmp_head;
    }
    /* 壓根沒有做保護 */
    self->data[self->head.r] = value;
    self->head.w++;

    if (self->head.r >= SPSC_QUEUE_SIZE)
        self->head.w = 0;

    return SPSC_OK;
}

/* Test program */

#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#define TEST_SIZE 200000000

#define N_MAX_CORE 8
static spsc_queue_t queues[N_MAX_CORE];

typedef struct {
    uint32_t cpu_id;
    pthread_barrier_t *barrier;
} init_info_t;

init_info_t info[N_MAX_CORE];

#define INIT_CPU_ID(p) (info[p].cpu_id)
#define INIT_BARRIER(p) (info[p].barrier)
#define INIT_PTR(p) (&info[p])

void *consumer(void *arg)
{
    element_t value = 0, old_value = 0;

    init_info_t *init = (init_info_t *) arg;
    uint32_t cpu_id = init->cpu_id;
    pthread_barrier_t *barrier = init->barrier;

    /* user needs tune this according to their machine configurations. */
    cpu_set_t cur_mask;
    CPU_ZERO(&cur_mask);
    CPU_SET(cpu_id * 2, &cur_mask);

    printf("consumer %d:  ---%d----\n", cpu_id, 2 * cpu_id);
    if (sched_setaffinity(0, sizeof(cur_mask), &cur_mask) < 0) {
        printf("Error: sched_setaffinity\n");
        return NULL;
    }

    printf("Consumer created...\n");
    pthread_barrier_wait(barrier);

    queues[cpu_id].start_c = read_tsc();

    for (uint64_t i = 1; i <= TEST_SIZE; i++) {
        while (dequeue(&queues[cpu_id], &value) != 0)
            ;

        assert((old_value + 1) == value);
        old_value = value;
    }
    queues[cpu_id].stop_c = read_tsc();

    printf(
        "consumer: %ld cycles/op\n",
        ((queues[cpu_id].stop_c - queues[cpu_id].start_c) / (TEST_SIZE + 1)));

    pthread_barrier_wait(barrier);
    return NULL;
}

void producer(void *arg, uint32_t num)
{
    cpu_set_t cur_mask;
    init_info_t *init = (init_info_t *) arg;
    pthread_barrier_t *barrier = init->barrier;

    /* FIXME: tune this according to machine configurations */
    CPU_ZERO(&cur_mask);
    CPU_SET(0, &cur_mask);
    printf("producer %d:  ---%d----\n", 0, 1);
    if (sched_setaffinity(0, sizeof(cur_mask), &cur_mask) < 0) {
        printf("Error: sched_setaffinity\n");
        return;
    }

    pthread_barrier_wait(barrier);

    uint64_t start_p = read_tsc();

    for (uint64_t i = 1; i <= TEST_SIZE + SPSC_BATCH_SIZE; i++) {
        for (int32_t j = 1; j < num; j++) {
            element_t value = i;
            while (enqueue(&queues[j], value) != 0)
                ;
        }
    }
    uint64_t stop_p = read_tsc();

    printf("producer %ld cycles/op\n",
           (stop_p - start_p) / ((TEST_SIZE + 1) * (num - 1)));

    pthread_barrier_wait(barrier);
}

int main(int argc, char *argv[])
{
    pthread_t consumer_thread;
    pthread_attr_t consumer_attr;
    pthread_barrier_t barrier;

    int max_threads = 2;
    if (argc > 1)
        max_threads = atoi(argv[1]);

    if (max_threads < 2) {
        max_threads = 2;
        printf("Minimum core number is 2\n");
    }
    if (max_threads > N_MAX_CORE) {
        max_threads = N_MAX_CORE;
        printf("Maximum core number is %d\n", max_threads);
    }

    for (int i = 0; i < N_MAX_CORE; i++)
        queue_init(&queues[i]);

    int error = pthread_barrier_init(&barrier, NULL, max_threads);
    if (error != 0) {
        perror("BW");
        return 1;
    }
    error = pthread_attr_init(&consumer_attr);
    if (error != 0) {
        perror("BW");
        return 1;
    }

    /* For N cores, there are N-1 FIFOs. */
    for (int i = 1; i < max_threads; i++) {
        INIT_CPU_ID(i) = i;
        INIT_BARRIER(i) = &barrier;
        error = pthread_create(&consumer_thread, &consumer_attr, consumer,
                               INIT_PTR(i));
    }
    if (error != 0) {
        perror("BW");
        return 1;
    }

    INIT_CPU_ID(0) = 0;
    INIT_BARRIER(0) = &barrier;
    producer(INIT_PTR(0), max_threads);
    printf("Done!\n");

    return 0;
}