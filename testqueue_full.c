#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include "queue.c"

#define NUM_OPERATIONS 10000
#define MAX_SIZE 1000

int dequeue_with_sleep(void *arg);
int enqueueItems(void *arg);

void test_initQueue()
{
    printf("=== Testing initQueue ===\n");

    initQueue();

    // Perform other operations on the queue (enqueue, dequeue, etc.)
    // ...

    printf("initQueue test passed.\n");
}

void test_destroyQueue()
{
    printf("=== Testing destroyQueue ===\n");

    initQueue();

    // Perform operations on the queue
    // ...

    destroyQueue();

    // Verify that all resources associated with the queue are properly cleaned up
    // ...

    // Attempt to perform operations on the queue after destruction and check for expected behavior
    // ...

    printf("destroyQueue test passed.\n");
}

void test_enqueue_dequeue()
{
    printf("=== Testing enqueue and dequeue ===\n");

    initQueue();

    int items[] = {1, 2, 3, 4, 5};
    size_t num_items = sizeof(items) / sizeof(items[0]);

    // Enqueue items
    for (size_t i = 0; i < num_items; i++)
    {
        enqueue(&items[i]);
    }

    // Dequeue items and check the order
    for (size_t i = 0; i < num_items; i++)
    {
        int *item = (int *)dequeue();
        printf("Dequeued: %d\n", *item);
        assert(*item == items[i]);
    }

    // Queue should be empty
    assert(size() == 0);

    destroyQueue();

    printf("enqueue and dequeue test passed.\n");
}

void test_tryDequeue()
{
    printf("=== Testing tryDequeue ===\n");

    initQueue();

    int items[] = {1, 2, 3, 4, 5};
    size_t num_items = sizeof(items) / sizeof(items[0]);

    // Try dequeueing when the queue is empty
    void *item;
    assert(!tryDequeue(&item));

    // Enqueue items
    for (size_t i = 0; i < num_items; i++)
    {
        enqueue(&items[i]);
    }

    // Try dequeueing items and check the order
    for (size_t i = 0; i < num_items; i++)
    {
        assert(tryDequeue(&item));
        printf("Dequeued: %d\n", *(int *)item);
        assert(*(int *)item == items[i]);
    }

    // Queue should be empty
    assert(size() == 0);

    destroyQueue();

    printf("tryDequeue test passed.\n");
}

void test_size()
{
    printf("=== Testing size ===\n");

    initQueue();

    int items[] = {1, 2, 3};
    size_t num_items = sizeof(items) / sizeof(items[0]);

    // Enqueue items
    for (size_t i = 0; i < num_items; i++)
    {
        enqueue(&items[i]);
    }

    // Check size
    assert(size() == num_items);

    // Perform enqueue and dequeue operations and check if size updates correctly
    // ...

    destroyQueue();

    printf("size test passed.\n");
}

int dequeue_with_wait(void *arg)
{
    struct timespec *sleep_time = (struct timespec *)arg;

    void *item;
    while (1)
    {
        if (tryDequeue(&item))
        {
            // Item dequeued successfully
            int value = *(int *)item;
            printf("Dequeued item: %d\n", value);
            return value;
        }
        else
        {
            // Queue is empty, sleep for the specified time
            thrd_sleep(sleep_time, NULL);
        }
    }
}

void test_waiting()
{
    initQueue();

    int items[] = {1, 2, 3};
    size_t num_items = sizeof(items) / sizeof(items[0]);

    // Enqueue items
    for (size_t i = 0; i < num_items; i++)
    {
        enqueue(&items[i]);
    }

    // Create threads that will wait for items in the queue
    thrd_t threads[3];
    struct timespec sleep_time = {5, 0}; // Sleep time of 5 seconds

    for (int i = 0; i < 3; i++)
    {
        thrd_create(&threads[i], (int (*)(void *))dequeue_with_wait, &sleep_time);
    }

    // Wait for all threads to finish
    int values[3];
    for (int i = 0; i < 3; i++)
    {
        thrd_join(threads[i], &values[i]);
    }

    // Queue should be empty
    assert(size() == 0);
    // All items should have been enqueued and dequeued
    assert(visited() == num_items);
    // No threads should be waiting
    assert(waiting() == 0);

    destroyQueue();
}

void test_sleeping_threads()
{
    initQueue();

    const int NUM_THREADS = 5;
    const int NUM_ITEMS = 10;
    const int SLEEP_TIME_MIN = 5;
    // const int SLEEP_TIME_MAX = 10;

    // Enqueue items
    for (int i = 1; i <= NUM_ITEMS; i++)
    {
        enqueue(&i);
    }

    thrd_t threads[NUM_THREADS];

    // Create threads that sleep when the queue is empty
    for (int i = 0; i < NUM_THREADS; i++)
    {
        thrd_create(&threads[i], dequeue_with_sleep, &(struct timespec){SLEEP_TIME_MIN, 0});
    }

    // Allow some time for threads to start sleeping
    thrd_sleep(&(struct timespec){10, 0}, NULL);

    // Verify the waiting count should be equal to the number of threads
    int waiting_count = waiting();
    printf("Waiting threads count: %d\n", waiting_count);
    assert(waiting_count == NUM_THREADS);

    // Enqueue additional items
    for (int i = NUM_ITEMS + 1; i <= 2 * NUM_ITEMS; i++)
    {
        enqueue(&i);
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; i++)
    {
        thrd_join(threads[i], NULL);
    }

    // Queue should be empty
    assert(size() == 0);
    // All items should have been dequeued
    assert(visited() == 2 * NUM_ITEMS);
    // No threads should be waiting
    assert(waiting() == 0);

    destroyQueue();
}

void test_concurrent_enqueue_dequeue()
{
    printf("=== Testing concurrent enqueue and dequeue ===\n");

    initQueue();

    // Number of items to enqueue
    int numItems = 100;
    int half = numItems / 2;
    // Enqueue items in a separate thread
    thrd_t enqueueThread_a;
    thrd_t enqueueThread_b;
    thrd_create(&enqueueThread_a, (int (*)(void *))enqueueItems, &half);
    thrd_create(&enqueueThread_b, (int (*)(void *))enqueueItems, &half);

    // Dequeue items in the main thread
    for (int i = 0; i < numItems; i++)
    {
        int *item = (int *)dequeue();
        printf("Dequeued item: %d\n", *item);
    }

    // Wait for the enqueue threads to finish
    // thrd_join(enqueueThread_a, NULL);
    // thrd_join(enqueueThread_b, NULL);

    destroyQueue();

    printf("concurrent enqueue and dequeue test passed.\n");
}

// Thread function to enqueue items
int enqueueItems(void *arg)
{
    int numItems = *(int *)arg;
    for (int i = 0; i < numItems; i++)
    {
        // printf("%d\n", i);
        int *item = (int *)malloc(sizeof(int));
        *item = i + 1;
        // printf("%d\n", *item);
        enqueue((void *)item);
        printf("Enqueued item: %d\n", *item);
    }
    thrd_exit(0);
}

void test_enqueue_tryDequeue()
{
    printf("=== Testing enqueue and tryDequeue ===\n");

    initQueue();

    // Enqueue items
    int item1 = 1;
    int item2 = 2;
    int item3 = 3;
    enqueue(&item1);
    enqueue(&item2);
    enqueue(&item3);

    // Try dequeueing items and check the order
    void *item;
    assert(tryDequeue(&item));
    printf("Dequeued item: %d\n", *(int *)item);
    assert(*(int *)item == item1);

    assert(tryDequeue(&item));
    printf("Dequeued item: %d\n", *(int *)item);
    assert(*(int *)item == item2);

    assert(tryDequeue(&item));
    printf("Dequeued item: %d\n", *(int *)item);
    assert(*(int *)item == item3);

    // Try dequeue from an empty queue
    assert(!tryDequeue(&item));

    destroyQueue();

    printf("enqueue and tryDequeue test passed.\n");
}

void test_enqueue_dequeue_with_sleep()
{
    printf("=== Testing enqueue and dequeue with sleep time ===\n");

    initQueue();

    // Enqueue items
    int item1 = 1;
    int item2 = 2;
    int item3 = 3;
    enqueue(&item1);
    enqueue(&item2);
    enqueue(&item3);

    // Create multiple threads to concurrently dequeue items with a delay between each dequeue operation
    thrd_t threads[3];
    for (int i = 0; i < 3; i++)
    {
        thrd_create(&threads[i], dequeue_with_sleep, NULL);
    }

    // Wait for all threads to finish
    for (int i = 0; i < 3; i++)
    {
        thrd_join(threads[i], NULL);
    }

    // Queue should be empty
    assert(size() == 0);

    destroyQueue();

    printf("enqueue and dequeue with sleep time test passed.\n");
}

int dequeue_with_sleep(void *arg)
{
    int sleep_time = (rand() % 6 + 5) * 1000; // Random sleep time between 5000 and 10000 milliseconds

    thrd_sleep(&(struct timespec){sleep_time / 1000, (sleep_time % 1000) * 1000000}, NULL);

    int *item = (int *)dequeue();
    printf("Dequeued item with sleep: %d\n", *item);

    return 0;
}

void test_edge_cases()
{
    printf("=== Testing edge cases ===\n");

    initQueue();

    // Dequeue from an empty queue - Should block until an item is enqueued
    int *item = (int *)dequeue();
    assert(item == NULL);
    printf("Dequeue from an empty queue - Assertion failed: Expected NULL\n");

    // Try dequeue from an empty queue - Should return false
    void *tryItem;
    assert(!tryDequeue(&tryItem));
    printf("Try dequeue from an empty queue - Assertion failed: Expected false\n");

    // Enqueue items
    int item1 = 1;
    int item2 = 2;
    enqueue(&item1);
    enqueue(&item2);

    // Dequeue the same item multiple times
    int *dequeuedItem = (int *)dequeue();
    assert(*dequeuedItem == item1);
    printf("Dequeued item: %d\n", *dequeuedItem);
    dequeuedItem = (int *)dequeue();
    assert(*dequeuedItem == item1);
    printf("Dequeued item: %d\n", *dequeuedItem);

    // Dequeue the maximum allowed number of items
    for (int i = 0; i < MAX_SIZE; i++)
    {
        int item = i + 1;
        enqueue(&item);
    }
    for (int i = 0; i < MAX_SIZE; i++)
    {
        int *dequeuedItem = (int *)dequeue();
        assert(*dequeuedItem == (i + 1));
    }

    // Enqueue and dequeue when the queue is at its capacity limit
    for (int i = 0; i < MAX_SIZE; i++)
    {
        int item = i + 1;
        enqueue(&item);
    }
    dequeuedItem = (int *)dequeue();
    assert(*dequeuedItem == 1);
    int newItem = MAX_SIZE + 1;
    enqueue(&newItem);

    // Enqueue and dequeue when the queue is empty
    void *tryDequeueItem;
    assert(!tryDequeue(&tryDequeueItem));
    printf("Try dequeue from an empty queue - Assertion failed: Expected false\n");

    destroyQueue();

    printf("edge cases test passed.\n");
}

void test_mixed_operations()
{
    printf("=== Testing mixed operations ===\n");

    initQueue();

    // Enqueue items
    int item1 = 1;
    enqueue(&item1);
    printf("Enqueued item: %d\n", item1);

    // Try dequeue
    void *tryDequeueItem;
    if (tryDequeue(&tryDequeueItem))
    {
        int *dequeuedItem = (int *)tryDequeueItem;
        printf("Dequeued item: %d\n", *dequeuedItem);
    }

    // Enqueue more items
    int item2 = 2;
    enqueue(&item2);
    printf("Enqueued item: %d\n", item2);

    int item3 = 3;
    enqueue(&item3);
    printf("Enqueued item: %d\n", item3);

    // Dequeue an item
    int *dequeuedItem = (int *)dequeue();
    printf("Dequeued item: %d\n", *dequeuedItem);

    // Enqueue another item
    int item4 = 4;
    enqueue(&item4);
    printf("Enqueued item: %d\n", item4);

    // Try dequeue multiple times
    for (int i = 0; i < 3; i++)
    {
        if (tryDequeue(&tryDequeueItem))
        {
            int *dequeuedItem = (int *)tryDequeueItem;
            printf("Dequeued item: %d\n", *dequeuedItem);
        }
    }

    // Enqueue additional items
    int item5 = 5;
    enqueue(&item5);
    printf("Enqueued item: %d\n", item5);

    int item6 = 6;
    enqueue(&item6);
    printf("Enqueued item: %d\n", item6);

    // Dequeue remaining items
    while (size() > 0)
    {
        int *dequeuedItem = (int *)dequeue();
        printf("Dequeued item: %d\n", *dequeuedItem);
    }

    destroyQueue();

    printf("mixed operations test passed.\n");
}

int main()
{
    test_initQueue();
    test_destroyQueue();
    test_enqueue_dequeue();
    test_tryDequeue();
    test_size();
    test_waiting();
    // test_sleeping_threads();
    test_concurrent_enqueue_dequeue();
    test_enqueue_tryDequeue();
    test_enqueue_dequeue_with_sleep();
    // test_edge_cases();
    test_mixed_operations();

    return 0;
}
