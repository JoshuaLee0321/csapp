#include <stdlib.h>
#include <sys/mman.h>

#include "coro/switch.h"
#include "util/system.h"

__asm__(
    ".text\n"
    ".globl context_switch\n"
    "context_switch:\n"
#if defined(__x86_64__) // 把 stack 中前六個元素與目前暫存的元素互相交換
#define NUM_SAVED 6
    "push %rbp\n"
    "push %rbx\n"
    "push %r12\n"
    "push %r13\n"
    "push %r14\n"
    "push %r15\n"
    "mov %rsp, (%rdi)\n"
    "mov (%rsi), %rsp\n"
    "pop %r15\n"
    "pop %r14\n"
    "pop %r13\n"
    "pop %r12\n"
    "pop %rbx\n"
    "pop %rbp\n"
    "pop %rcx\n"
    "jmp *%rcx\n"
#else
#error "unsupported architecture"
#endif
);

void coro_routine_entry();

__asm__(
    ".text\n"
    "coro_routine_entry:\n"
#if defined(__x86_64__)
    "pop %rdi\n"    // poped out from stack, save into rdi
    "pop %rcx\n"    // pop ** rcx
    "call *%rcx\n"  // call rcx function
#else
#error "unsupported architecture"
#endif
);

void coro_stack_init(struct context *ctx,
                     struct coro_stack *stack,
                     coro_routine routine,
                     void *args)
{
    ctx->sp = (void **) stack->ptr;
    *--ctx->sp = (void *) routine;
    *--ctx->sp = (void *) args;
    *--ctx->sp = (void *) coro_routine_entry;
    ctx->sp -= NUM_SAVED;
}

/* ensure size_bytes aligned by get_page_size() */
int coro_stack_alloc(struct coro_stack *stack, size_t size_bytes)
{
    stack->size_bytes = size_bytes;
    size_bytes += get_page_size();

    stack->ptr = mmap(NULL, size_bytes, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (stack->ptr == (void *) -1)
        return -1;

    mprotect(stack->ptr, get_page_size(), PROT_NONE); /*這邊就不能訪問了*/
    // mprotect(stack->ptr, get_page_size(), PROT_READ | PROT_WRITE);
    /* 更改成 PROT_READ 就可以讓別人讀取但不可以寫 */
    
    stack->ptr = (void *) ((char *) stack->ptr + size_bytes);

    return 0;
}

void coro_stack_free(struct coro_stack *stack)
{
    stack->ptr =
        (void *) ((char *) stack->ptr - stack->size_bytes - get_page_size());
    munmap(stack->ptr, stack->size_bytes + get_page_size());
}
