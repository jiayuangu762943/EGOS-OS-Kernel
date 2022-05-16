#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

/*
    This is your test file.
    Write good tests!
*/

// Test 1 is given to you
void test1()
{
    queue_t new = queue_new();
    if (new == NULL)
    {
        fprintf(stderr, "test1: queue_new failed\n");
        exit(1);
    }
    int r = queue_free(new);
    if (r != 0)
    {
        fprintf(stderr, "test1: queue_free() returned %d instead of 0\n", r);
        exit(1);
    }
}

void test2()
{
    queue_t new = queue_new();
    int first_ele = 10;
    void *ptr = &first_ele;
    int res = queue_enqueue(new, ptr);
    if (res != 0)
    {
        printf("test2: No memory for this new element");
        exit(1);
    }
    int a = 5;
    void *poped_item = (void *)&a;
    int res_ = queue_dequeue(new, &poped_item);
    if (res_ != 0)
    {
        printf("test2: the queue is empty");
        exit(1);
    }
    if (*(int *)poped_item != 10)
    {
        printf("test2: the item popped is wrong");
        exit(1);
    }
    int second_ele = 20;
    void *ptr2 = &second_ele;
    int third_ele = 30;
    void *ptr3 = &third_ele;
    int fourth_ele = 40;
    void *ptr4 = &fourth_ele;
    int res1 = queue_insert(new, ptr2);
    if (res1 != 0)
    {
        printf("test2: no memory for the new item");
        exit(1);
    }
    int res2 = queue_enqueue(new, ptr3);
    int res3 = queue_enqueue(new, ptr4);
    if (queue_length(new) != 3)
    {
        printf("test2: the length of returned by queue_length is wrong");
    }

    int delete3 = queue_dequeue(new, &poped_item);
    if (*(int *)poped_item != 20)
    {
        printf("test2: the item dequeued from the queue isn't what's inserted");
        exit(1);
    }

    int delete1 = queue_delete(new, ptr2);
    if (delete1 != -1)
    {
        printf("test2: ptr2 should have been dequeued already");
        exit(1);
    }

    int delete2 = queue_delete(new, ptr3);
    if (delete2 == -1)
    {
        printf("test2: no item of ptr3 is found");
        exit(1);
    }

    int delete4 = queue_dequeue(new, &poped_item);
    if (*(int *)poped_item != 40)
    {
        printf("test2: the last item from the queue isn't what's enqueued last");
        exit(1);
    }
    if (queue_length(new) != 0)
    {
        printf("test2: length of queue now should be 0");
        exit(1);
    }

    char fifth_ele = 'a';
    void *fifth_ptr = &fifth_ele;
    int res4 = queue_enqueue(new, fifth_ptr);
    void **null_popped_item = NULL;
    int dequeued_with_no_return = queue_dequeue(new, null_popped_item);
    if (dequeued_with_no_return != 0)
    {
        printf("test2: dequeue function with *poped_item = null doesnt return correctly");
        exit(1);
    }
    if (null_popped_item != NULL)
    {
        printf("test2: the queue delete function shouldn't put the dequeued item in null_popped_item");
        exit(1);
    }
    if (queue_length(new) != 0)
    {
        printf("test2: the length of queue after dequeuing the fifth ele should be 0\n");
        exit(1);
    }

    int null_delete_1 = queue_dequeue(new, &poped_item);
    if (null_delete_1 != -1)
    {
        printf("test2: poping an item from an empty queue should return -1");
        exit(1);
    }
    if (*(int *)poped_item != 40)
    {
        printf("test2: the value pointed by [poped_item] shouldn't change");
        exit(1);
    }

    int null_delete_2 = queue_delete(new, ptr3); // the pointer to delete from the empty queue doesn't matter
    if (null_delete_2 != -1)
    {
        printf("test2: poping an item from an empty queue should return -1");
        exit(1);
    }

    int r = queue_free(new);
}

void seg_fault_iterate_func(void *item, void *context)
{
    void **ptr;
    int wrong = 10;
    int *wrong_ptr = &wrong;
    int **wrong_ptr_ptr = &wrong_ptr;
    ptr = (void **)wrong_ptr_ptr;
}

struct IterateContext
{
    int *arr;
    int idx;
};

void iterate_function(void *item, void *context)
{
    char *num = (char *)item;
    struct IterateContext *state = (struct IterateContext *)context;
    int ele = *num - '0';
    state->arr[state->idx++] = ele;
}

void test3()
{
    // using it over an empty queue shouldn't cause the seg_fault here
    queue_t new = queue_new();
    void *none_context;
    queue_iterate(new, seg_fault_iterate_func, none_context);

    char first_ele = '1';
    void *ptr1 = &first_ele;
    char second_ele = '2';
    void *ptr2 = &second_ele;
    char third_ele = '3';
    void *ptr3 = &third_ele;
    int res1 = queue_enqueue(new, ptr1);
    int res2 = queue_enqueue(new, ptr2);
    int res3 = queue_enqueue(new, ptr3);

    struct IterateContext *char_to_int_context = (struct IterateContext *)malloc(sizeof(struct IterateContext));
    char_to_int_context->arr = (int *)malloc(3 * sizeof(int));
    char_to_int_context->idx = 0;
    queue_iterate(new, iterate_function, char_to_int_context);
    for (int i = 0; i < 3; i++)
    {
        int n = char_to_int_context->arr[i];
        if (n != (i + 1))
        {
            printf("test3 iterate function: didn't correctly process each item in the queue with the passed-in function ");
        }
    }
    while (queue_length(new) != 0)
    {
        queue_dequeue(new, NULL);
    }
    queue_free(new);
    free(char_to_int_context->arr);
    free(char_to_int_context);
}

int main(void)
{
    test1();
    test2();
    test3();
    return 0;
}
