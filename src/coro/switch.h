#ifndef CORO_SWITCH_H
#define CORO_SWITCH_H

struct coro_stack {
    void *ptr;
    size_t size_bytes;
};

struct context {
    void **sp; /* current coroutine's top stack. equivalent to %esp / %rsp */
};

typedef void (*coro_routine)(void *args) __attribute__((__regparm__(1)));
/* __regparm__ = register parameter，傳送參數 *args 的時候會用 register 傳送
*  也就是這個 function pointer 會吃一個 void* 參數，然後用暫存傳給 assigned 的
*  value
*/

void context_switch(struct context *prev, struct context *next)
    __attribute__((__noinline__, __regparm__(2)));
    /* __noinline__ 代表編譯器不會再這個 function 被呼叫時被展開
    *  __regparm__(2) 表示其中兩個 context* 都是使用 %eax %eai 傳送
    */


/* 初始化 coroutine */
void coro_stack_init(struct context *ctx,
                     struct coro_stack *stack,
                     coro_routine routine,
                     void *args);

int coro_stack_alloc(struct coro_stack *stack, size_t size_bytes);
void coro_stack_free(struct coro_stack *stack);

#endif
