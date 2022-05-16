#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
// Define a singly-linked list node as underlying data structure
typedef struct Node
{
    void *ele;
    struct Node *next;
} * node_t;

typedef struct queue
{
    // head and tail stores the head and tail of the linked list respectively
    node_t head;
    node_t tail;
    int length;

} * queue_t;

queue_t queue_new()
{
    // your code here
    queue_t q = (queue_t)malloc(sizeof(struct queue));
    if (q == NULL)
    {
        return NULL;
    }
    q->head = NULL;
    q->tail = NULL;
    q->length = 0;
    return q;
}

// create a new node
node_t new_node(void *e)
{
    // create the memory space on heap for the new node
    node_t temp = (node_t)malloc(sizeof(struct Node));
    // if there is no memory left, return NULL for other functions to
    // receive and process
    if (temp == NULL)
    {
        return NULL;
    }
    temp->ele = e;
    temp->next = NULL;
    return temp;
}

int queue_enqueue(queue_t queue, void *item)
{
    // create the new node
    node_t node = new_node(item);
    // handle the case when there is no memory for the new node
    if (node == NULL)
    {
        return -1;
    }
    // when the queue is empty, let head and tail of the queue
    // point to this newly added and only element in the queue
    if (queue->head == NULL && queue->tail == NULL)
    {
        queue->head = queue->tail = node;
        queue->length++;
        return 0;
    }

    queue->tail->next = node;
    queue->tail = node;
    queue->length++;

    return 0;
}

int queue_insert(queue_t queue, void *item)
{
    node_t node = new_node(item);
    if (node == NULL)
    {
        return -1;
    }
    // when the queue is empty, let head and tail of the queue
    // point to this newly added and only element in the queue
    if (queue->head == NULL && queue->tail == NULL)
    {
        queue->head = node;
        queue->tail = node;
        queue->length++;
        return 0;
    }
    node->next = queue->head;
    queue->head = node;
    queue->length++;
    return 0;
}

int queue_dequeue(queue_t queue, void **pitem)
{
    // when the queue is empty don't dequeue and return -1
    if (queue->head == NULL && queue->tail == NULL)
    {
        return -1;
    }
    node_t temp = queue->head;
    if (pitem != NULL)
    {
        // put the dequeued item in the queue only if *pitem is not null
        *pitem = temp->ele;
    }
    // update the head of queue
    queue->head = queue->head->next;
    queue->length--;
    if (queue->head == NULL)
    {
        // if dequeuing the last item gives an empty queue, make the tail null as well
        queue->tail = NULL;
    }

    // free the memory allocated for the node that holds the dequeued item on the heap
    free(temp);
    return 0;
}

void queue_iterate(const queue_t queue, queue_func_t f, void *context)
{
    node_t cur = queue->head; // iterate from the head of the queue
    // as long as the element pointed by cur is not null, apply the function f
    while (cur != NULL)
    {
        f(cur->ele, context);
        cur = cur->next;
    }
}

int queue_free(queue_t queue)
{
    // when the queue is empty
    if (queue->head == NULL && queue->tail == NULL)
    {
        free(queue);
        return 0;
    }

    // the queue is not empty
    return -1;
}

int queue_length(const queue_t queue)
{
    return queue->length;
}

int queue_delete(queue_t queue, void *item)
{
    node_t cur = queue->head;
    node_t prev = NULL;
    // iterate over the queue
    while (cur != NULL)
    {
        // if find a match
        if (cur->ele == item)
        {
            // if the match is the head of the queue
            if (prev == NULL)
            {
                // update the head
                queue->head = cur->next;
                // if deleting the item makes the queue empty, make tail null as well
                if (queue->head == NULL)
                {
                    queue->tail = NULL;
                }
                queue->length--;
                free(cur);
                return 0;
            }
            prev->next = cur->next;
            if (cur == queue->tail)
            {
                queue->tail = prev;
            }
            queue->length--;
            free(cur);
            return 0;
        }
        prev = cur;
        cur = cur->next;
    }
    // if program runs into this stage,
    // it means it doesn't find item in the queue
    return -1;
}
