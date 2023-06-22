#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
void interrupt_handler()
{
    printf("hehe\n");
}
int main()
{
    signal(SIGINT, interrupt_handler);
    while(true)
    {
        continue;
    }
}