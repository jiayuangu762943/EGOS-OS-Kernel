#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "earth.h"
#include "queue.h"
#include "context.h"
#include "inttypes.h"
#include "string.h"

/**** THREADS AND SEMAPHORES ****/

// Enum for thread states
enum state
{
	RUNNING,
	RUNNABLE,
	BLOCKED,
	TERMINATED
};

struct thread
{
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
int num_blocked_threads;

// your thread and semaphore code here
void thread_init();
void thread_create(void (*f)(void *arg), void *arg, unsigned int stack_size);
void scheduler();
void cleanup();
void thread_yield();
void thread_block();
void thread_exit();

void sema_init(struct sema *sema, unsigned int count);
void sema_inc(struct sema *sema);
void sema_dec(struct sema *sema);
bool sema_release(struct sema *sema);

static void producer(void *arg);
static void consumer(void *arg);

static void test_code(void *arg);
/**** TEST SUITE ****/
int dining_philosophers();
int producer_consumer();
int thread_tests();

// Dining philosophers
#define NSLOTS 3
static struct sema s_empty, s_full, s_lock;
static unsigned int in, out;
static char *slots[NSLOTS];

enum state_phil
{
	THINKING,
	HUNGRY,
	EATING
};

#define NPHILOSOPHERS 5
#define MAXITERATIONS 10
#define LEFT (phil_num + 4) % NPHILOSOPHERS
#define RIGHT (phil_num + 1) % NPHILOSOPHERS
enum state_phil status_phil[NPHILOSOPHERS];
int num_iterations[NPHILOSOPHERS] = {0};
int num_iterations_main = 0;
int phil[NPHILOSOPHERS] = {0, 1, 2, 3, 4};

struct sema waiter;
struct sema forks[NPHILOSOPHERS];
void dine(void *num);
void take_fork(int phil_num);
void put_fork(int phil_num);
void test(int phil_num);

int main(int argc, char **argv)
{
	if (argc <= 1)
	{
		thread_tests();
	}
	else if (strcmp(argv[1], "1") == 0)
	{
		dining_philosophers();
	}
	else if (strcmp(argv[1], "2") == 0)
	{
		producer_consumer();
	}
	else
	{
		thread_tests();
	}
	return 0;
}

// Method for running the dining philosophers implementation
int dining_philosophers()
{
	thread_init();
	sema_init(&waiter, 1);
	for (int i = 0; i < NPHILOSOPHERS; i++)
	{
		sema_init(&forks[i], 0);
	}
	for (int i = 0; i < NPHILOSOPHERS; i++)
	{
		thread_create(dine, &(phil[i]), 16 * 1024);
	}
	while (num_iterations_main < 200)
	{
		num_iterations_main++;
		thread_yield();
	}

	thread_exit();
	return 0;
}

// Method for running producer/consumer to test our implementations
int producer_consumer()
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

// tests for the four thread functions
int thread_tests()
{
	thread_init();
	thread_create(test_code, "thread 1", 16 * 1024);
	thread_create(test_code, "thread 2", 16 * 1024);
	test_code("main thread");
	thread_exit();
	free(queue_th);
	return 0;
}

void thread_init()
{
	// initialize the struct for the main thread and make it the running thread
	struct thread *first_th_p = malloc(sizeof(struct thread));
	first_th_p->sp = 0;
	first_th_p->fp = 0;
	running_th_p = first_th_p;

	// allocate a queue for storing runnable threads
	queue_th = malloc(sizeof(struct queue));
	queue_init(queue_th);
}

void thread_create(void (*f)(void *arg), void *arg, unsigned int stack_size)
{
	// When a new thread is created,
	// set the state of currently running thread to runnable
	struct thread *old_th_p = running_th_p;
	old_th_p->status = RUNNABLE;
	queue_add(queue_th, old_th_p);

	// Create and run the new thread
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

	// Set the state of current thread to runnable and add it to the runnable queue
	running_th_p->status = RUNNABLE;
	if (queue_empty(queue_th))
	{
		return;
	}

	queue_add(queue_th, running_th_p);
	scheduler();
}

void thread_exit()
{
	// clean zombie if there is any
	if (zb_th_p != 0)
	{
		cleanup();
	}

	// Existing thread is set as a zombie thread for the future clean up
	zb_th_p = running_th_p;
	if (queue_empty(queue_th))
	{
		exit(0);
	}
	scheduler();
}

// Test function that is used as a parameter for threads
static void test_code(void *arg)
{
	int i;
	for (i = 0; i < 10; i++)
	{
		printf("%s here: %d\n", (char *)arg, i);
		thread_yield();
	}
	printf("%s done\n", (char *)arg);
}

void ctx_entry()
{
	// run the function in the thread
	(running_th_p->f)(running_th_p->arg);
	// after running, mark the thread as terminated and exit
	running_th_p->status = TERMINATED;
	thread_exit();
}

// Cleans up zombie thread
void cleanup()
{
	free((void *)zb_th_p->fp);
	free((void *)zb_th_p);
	zb_th_p = 0;
}

// Switches from the current thread to the next runnable thread
void scheduler()
{
	struct thread *old_th_p = running_th_p;

	// Get the next runnable queue and set its state to running
	running_th_p = queue_get(queue_th);
	running_th_p->status = RUNNING;

	// Switch context from old thread to the new thread
	ctx_switch(&old_th_p->sp, running_th_p->sp);
}

// Blocks a thread by the given semaphore
void thread_block(struct sema *sema)
{
	// Sets state of the thread to blocked and adds it to the blocked queue of the given semaphore
	running_th_p->status = BLOCKED;
	queue_add(sema->queue_blocked, running_th_p);

	// If runnable queue is empty but there are still blocked threads left, it means that an unwanted situation occured.
	// That is why exit(1) is called
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
	num_blocked_threads++;
	scheduler();
}

/**** SEMAPHORES ****/

// Initializes a new semaphore by the given count
void sema_init(struct sema *sema, unsigned int count)
{

	// init queue
	struct queue *queue_blocked = malloc(sizeof(struct queue));
	queue_init(queue_blocked);
	sema->queue_blocked = queue_blocked;

	sema->count = count;
}

// Increments the count of the given semaphore if it doesn't have any blocked thread.
// If the semaphore has blocked threads the first one is set as runnable
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
			// If there are already blocked threads, the first one's state is changed from blocked to runnable
			// and it is added to the runnable queue
			struct thread *next_th_to_run;
			next_th_to_run = queue_get(sema->queue_blocked);

			next_th_to_run->status = RUNNABLE;
			queue_add(queue_th, next_th_to_run);
			num_blocked_threads--;
		}
	}
	else
	{
		sema->count = sema->count + 1;
	}
}

// Blocks the thread if the count is 0, if not decrements the count
void sema_dec(struct sema *sema)
{
	if (sema->count == 0)
	{
		thread_block(sema);
	}
	else
	{
		sema->count = sema->count - 1;
	}
}

// Releases the given semaphore if it doesn't block any thread
bool sema_release(struct sema *sema)
{
	// Check whether the blocked queue is empty. If not return false
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

void dine(void *num)
{
	while (1)
	{
		int *i = num;
		for (int j = 0; j < 100000000; j++)
		{
		}
		thread_yield();

		take_fork(*i);
		put_fork(*i);
		if (num_iterations[*(int *)num] > MAXITERATIONS)
		{
			printf("Philosopher %d left the restaurant after %d iterations\n", *(int *)num + 1, num_iterations[*(int *)num]);
			break;
		}
		num_iterations[*(int *)num] = num_iterations[*(int *)num] + 1;
	}
}

void take_fork(int phil_num)
{
	sema_dec(&waiter);

	// state that hungry
	status_phil[phil_num] = HUNGRY;

	printf("Philosopher %d is hungry\n", phil_num + 1);
	if (num_iterations[phil_num] == 0)
	{
		thread_yield();
	}
	// eat if neighbors are not eating
	test(phil_num);

	sema_inc(&waiter);

	// if unable to eat wait to be signalled
	sema_dec(&forks[phil_num]);

	for (int i = 0; i < 100000000; i++)
	{
	}
	thread_yield();
}

void put_fork(int phil_num)
{
	sema_dec(&waiter);

	// state that thinking
	status_phil[phil_num] = THINKING;

	printf("Philosopher %d putting fork %d and %d down\n",
				 phil_num + 1, LEFT + 1, phil_num + 1);
	printf("Philosopher %d is thinking\n", phil_num + 1);

	test(LEFT);
	test(RIGHT);

	sema_inc(&waiter);
}

void test(int phil_num)
{
	if (status_phil[phil_num] == HUNGRY && status_phil[LEFT] != EATING && status_phil[RIGHT] != EATING)
	{
		// state that eating
		status_phil[phil_num] = EATING;

		thread_yield();

		printf("Philosopher %d takes fork %d and %d\n",
					 phil_num + 1, LEFT + 1, phil_num + 1);

		printf("Philosopher %d is Eating\n", phil_num + 1);

		sema_inc(&forks[phil_num]);
	}
}