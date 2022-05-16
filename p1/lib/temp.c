#include <stdlib.h>
#include <stdio.h>
#include "earth.h"
#include "queue.h"
#include "context.h"

/**** THREADS AND SEMAPHORES ****/

enum state
{
    RUNNING,
    RUNNABLE,
    BLOCKED,
    TERMINATED
};

struct thread
{
    // your code here
    address_t sp;
    address_t fp;
    enum state status;
    void (*f)(void *arg);
    void *arg;
};

struct sema
{
    struct queue *queue_blocked;
    int count;
};

// declare global variables here
struct queue *queue_th;
struct thread *running_th_p = 0;
struct thread *zb_th_p = 0;

// your thread and semaphore code here
void thread_init();
void thread_create(void (*f)(void *arg), void *arg, unsigned int stack_size);
void scheduler();
void cleanup();
void thread_yield();
void thread_block();
void thread_exit();
int num_blocked_threads;

void sema_init(struct sema *sema, unsigned int count);
void sema_inc(struct sema *sema);
void sema_dec(struct sema *sema);
bool sema_release(struct sema *sema);

static void producer(void *arg);
static void consumer(void *arg);

static void test_code(void *arg);
/**** TEST SUITE ****/

/**/
#define NSLOTS 3
static struct sema s_empty, s_full, s_lock;
static unsigned int in, out;
static char *slots[NSLOTS];

int main(int argc, char **argv)
{
    thread_init();
    sema_init(&s_lock, 1);
    sema_init(&s_full, 0);
    sema_init(&s_empty, NSLOTS);
    printf("here  main \n");
    thread_create(consumer, "consumer 1", 16 * 1024);
    producer("producer 1");
    thread_exit();
    return 0;
}

// your test code here
// int main(int argc, char **argv)
// {
//     thread_init();
//     // your code here
//     thread_create(test_code, "thread 1", 16 * 1024);
//     thread_create(test_code, "thread 2", 16 * 1024);
//     test_code("main thread");
//     thread_exit();
//     free(queue_th);
//     return 0;
// }

void thread_init()
{
    // initialize the struct
    struct thread *first_th_p = malloc(sizeof(struct thread));
    // first_th_p->sp = malloc(sizeof(16 * 1024));
    first_th_p->sp = 0;
    first_th_p->fp = 0;
    running_th_p = first_th_p;

    queue_th = malloc(sizeof(struct queue));
    queue_init(queue_th);
}

void thread_create(void (*f)(void *arg), void *arg, unsigned int stack_size)
{
    struct thread *old_th_p = running_th_p;
    old_th_p->status = RUNNABLE;
    queue_add(queue_th, old_th_p);

    struct thread *th_p = malloc(sizeof(struct thread));
    th_p->f = f;
    th_p->arg = arg;
    th_p->fp = (address_t)malloc(stack_size);
    th_p->sp = (address_t)th_p->fp + stack_size;
    th_p->status = RUNNING;

    running_th_p = th_p;

    address_t new_sp = th_p->sp;
    ctx_start(&old_th_p->sp, new_sp);
}

void thread_yield()
{
    // clean zombie
    if (zb_th_p != 0)
    {
        cleanup();
    }
    running_th_p->status = RUNNABLE;
    scheduler();
}

void thread_exit()
{
    // clean zombie
    if (zb_th_p != 0)
    {
        cleanup();
    }
    scheduler();
}

static void test_code(void *arg)
{
    int i;
    for (i = 0; i < 10; i++)
    {
        printf("%s here: %d\n", (char *)arg, i);
        thread_exit();
    }
    printf("%s done\n", (char *)arg);
}

void ctx_entry()
{
    (running_th_p->f)(running_th_p->arg);
    running_th_p->status = TERMINATED;
    thread_exit();
}

void cleanup()
{
    free((void *)zb_th_p->fp);
    free((void *)zb_th_p);
    zb_th_p = 0;
}

/*
Taught in 2110: comment your function
*/
void scheduler()
{
    //
    if (running_th_p->status == TERMINATED)
    {
        zb_th_p = running_th_p;
        if (queue_empty(queue_th))
        {
            if (num_blocked_threads)
            {
                exit(1);
            }
            else
            {
                exit(0);
            }
        }
    }
    if (running_th_p->status == RUNNABLE)
    {
        if (queue_empty(queue_th))
        {
            return;
        }
        queue_add(queue_th, running_th_p);
    }
    if (running_th_p->status == RUNNING)
    {
        exit(0);
    }
    struct thread *old_th_p = running_th_p;
    running_th_p = queue_get(queue_th);
    running_th_p->status = RUNNING;
    ctx_switch(&old_th_p->sp, running_th_p->sp);
}

void thread_block(struct sema *sema)
{
    running_th_p->status = BLOCKED;
    queue_add(sema->queue_blocked, running_th_p);
    if (queue_empty(queue_th))
    {
        // when no runnable thread but blocked threads, return exit(1)?
        if (num_blocked_threads)
        {
            exit(1);
        }
        else
        {
            // if none block threads, keep running the thread that calls sema_dec
            exit(0);
        }
    }
    struct thread *old_th_p = running_th_p;
    running_th_p = queue_get(queue_th);
    running_th_p->status = RUNNING;

    ctx_switch(&old_th_p->sp, running_th_p->sp);
}

/**** SEMAPHORES ****/
void sema_init(struct sema *sema, unsigned int count)
{

    // init queue
    struct queue *queue_blocked = malloc(sizeof(struct queue));
    queue_init(queue_blocked);
    sema->queue_blocked = queue_blocked;

    sema->count = count;
}

void sema_inc(struct sema *sema)
{
    if (sema->count == 0)
    {
        // if there are no blocked threads, just increment the counter
        if (queue_empty(sema->queue_blocked))
        {
            sema->count++;
        }
        else
        {
            // should we just put the first blocked thread to runnable queue or make it the current running thread?
            struct thread *blocked;
            blocked = queue_get(sema->queue_blocked);

            blocked->status = RUNNABLE;
            queue_add(queue_th, blocked);
        }
    }
    else
    {
        sema->count = sema->count + 1;
    }
}

void sema_dec(struct sema *sema)
{
    if (sema->count == 0)
    {
        thread_block(sema);
        printf("here  sema_dec \n");
    }
    else
    {
        sema->count = sema->count - 1;
    }
}

bool sema_release(struct sema *sema)
{
    // check whether the blocked queue is empty
    if (!queue_empty(sema->queue_blocked))
    {
        return false;
    }

    free(sema->queue_blocked);
    free(sema);

    return true;
}

static void producer(void *arg)
{
    for (;;)
    {
        // first make sure there’s an empty slot.
        sema_dec(&s_empty);
        // now add an entry to the queue
        sema_dec(&s_lock);
        slots[in++] = arg;
        if (in == NSLOTS)
            in = 0;
        sema_inc(&s_lock);
        // finally, signal consumers
        sema_inc(&s_full);
    }
}
static void consumer(void *arg)
{
    unsigned int i;
    for (i = 0; i < 5; i++)
    {
        // first make sure there’s something in the buffer
        printf("here  consumer \n");
        sema_dec(&s_full);
        // now grab an entry to the queue
        sema_dec(&s_lock);
        void *x = slots[out++];
        printf("%s: got ’%s’\n", (char *)arg, (char *)x);
        if (out == NSLOTS)
            out = 0;
        sema_inc(&s_lock);
        // finally, signal producers
        sema_inc(&s_empty);
    }
}